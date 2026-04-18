#include "imagemanager.hpp"

#include <cassert>
#include <cstdint>
#include <osgDB/Registry>

#include <components/debug/debuglog.hpp>
#include <components/misc/pathhelpers.hpp>
#include <components/sceneutil/glextensions.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

#include "objectcache.hpp"

#ifdef OSG_LIBRARY_STATIC
// This list of plugins should match with the list in the top-level CMakelists.txt.
USE_OSGPLUGIN(png)
USE_OSGPLUGIN(tga)
USE_OSGPLUGIN(dds)
USE_OSGPLUGIN(jpeg)
USE_OSGPLUGIN(bmp)
#ifndef __vita__
USE_OSGPLUGIN(osg)
USE_SERIALIZER_WRAPPER_LIBRARY(osg)
#endif
#endif

namespace
{

    osg::ref_ptr<osg::Image> createWarningImage()
    {
        osg::ref_ptr<osg::Image> warningImage = new osg::Image;

        int width = 8, height = 8;
        warningImage->allocateImage(width, height, 1, GL_RGB, GL_UNSIGNED_BYTE);
        assert(warningImage->isDataContiguous());
        unsigned char* data = warningImage->data();
        for (int i = 0; i < width * height; ++i)
        {
            data[3 * i] = (255);
            data[3 * i + 1] = (0);
            data[3 * i + 2] = (255);
        }
        return warningImage;
    }

#ifdef __vita__
    struct DXT1Block
    {
        uint16_t color_0, color_1;
        uint32_t texels;
    };

    struct DXT3Block
    {
        uint16_t alpha[4];
        uint16_t color_0, color_1;
        uint32_t texels;
    };

    struct DXT5Block
    {
        uint8_t alpha_0, alpha_1;
        uint8_t alphaIdx[6];
        uint16_t color_0, color_1;
        uint32_t texels;
    };

    inline void rgb565to888(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b)
    {
        r = ((c >> 11) & 0x1F) * 255 / 31;
        g = ((c >> 5) & 0x3F) * 255 / 63;
        b = (c & 0x1F) * 255 / 31;
    }

    void decompressDXT(const osg::Image* src, osg::Image* dst)
    {
        const int w = src->s();
        const int h = src->t();
        const GLenum fmt = src->getPixelFormat();
        const bool isDXT1 = (fmt == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || fmt == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
        const bool isDXT3 = (fmt == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
        // isDXT5 implied otherwise
        const bool hasAlpha = (dst->getPixelFormat() == GL_RGBA);
        const int bpp = hasAlpha ? 4 : 3;
        const int rowBytes = w * bpp;

        const unsigned char* srcData = src->data();
        unsigned char* dstData = dst->data();

        const int blocksX = (w + 3) / 4;
        const int blocksY = (h + 3) / 4;

        for (int by = 0; by < blocksY; ++by)
        {
            for (int bx = 0; bx < blocksX; ++bx)
            {
                uint8_t colors[4][4]; // [index][r,g,b,a]
                uint8_t blockAlpha[16]; // per-pixel alpha for DXT3/5

                if (isDXT1)
                {
                    const auto* blk = reinterpret_cast<const DXT1Block*>(srcData);
                    srcData += sizeof(DXT1Block);

                    rgb565to888(blk->color_0, colors[0][0], colors[0][1], colors[0][2]);
                    colors[0][3] = 255;
                    rgb565to888(blk->color_1, colors[1][0], colors[1][1], colors[1][2]);
                    colors[1][3] = 255;

                    if (blk->color_0 > blk->color_1)
                    {
                        for (int c = 0; c < 3; ++c)
                        {
                            colors[2][c] = (2 * colors[0][c] + colors[1][c] + 1) / 3;
                            colors[3][c] = (colors[0][c] + 2 * colors[1][c] + 1) / 3;
                        }
                        colors[2][3] = 255;
                        colors[3][3] = 255;
                    }
                    else
                    {
                        for (int c = 0; c < 3; ++c)
                            colors[2][c] = (colors[0][c] + colors[1][c]) / 2;
                        colors[2][3] = 255;
                        colors[3][0] = colors[3][1] = colors[3][2] = 0;
                        colors[3][3] = 0; // transparent black
                    }

                    uint32_t bits = blk->texels;
                    const int maxPy = std::min(4, h - by * 4);
                    const int maxPx = std::min(4, w - bx * 4);
                    for (int py = 0; py < maxPy; ++py)
                    {
                        unsigned char* row = dstData + (by * 4 + py) * rowBytes + bx * 4 * bpp;
                        for (int px = 0; px < maxPx; ++px)
                        {
                            int idx = bits & 3;
                            bits >>= 2;
                            row[px * bpp + 0] = colors[idx][0];
                            row[px * bpp + 1] = colors[idx][1];
                            row[px * bpp + 2] = colors[idx][2];
                            if (hasAlpha)
                                row[px * bpp + 3] = colors[idx][3];
                        }
                        // skip remaining bits in this row if width not multiple of 4
                        for (int px = maxPx; px < 4; ++px)
                            bits >>= 2;
                    }
                }
                else if (isDXT3)
                {
                    const auto* blk = reinterpret_cast<const DXT3Block*>(srcData);
                    srcData += sizeof(DXT3Block);

                    // Decode 4-bit explicit alpha
                    for (int i = 0; i < 4; ++i)
                    {
                        uint16_t a = blk->alpha[i];
                        blockAlpha[i * 4 + 0] = ((a >> 0) & 0xF) * 17;
                        blockAlpha[i * 4 + 1] = ((a >> 4) & 0xF) * 17;
                        blockAlpha[i * 4 + 2] = ((a >> 8) & 0xF) * 17;
                        blockAlpha[i * 4 + 3] = ((a >> 12) & 0xF) * 17;
                    }

                    rgb565to888(blk->color_0, colors[0][0], colors[0][1], colors[0][2]);
                    rgb565to888(blk->color_1, colors[1][0], colors[1][1], colors[1][2]);
                    for (int c = 0; c < 3; ++c)
                    {
                        colors[2][c] = (2 * colors[0][c] + colors[1][c] + 1) / 3;
                        colors[3][c] = (colors[0][c] + 2 * colors[1][c] + 1) / 3;
                    }

                    uint32_t bits = blk->texels;
                    const int maxPy = std::min(4, h - by * 4);
                    const int maxPx = std::min(4, w - bx * 4);
                    for (int py = 0; py < maxPy; ++py)
                    {
                        unsigned char* row = dstData + (by * 4 + py) * rowBytes + bx * 4 * bpp;
                        for (int px = 0; px < maxPx; ++px)
                        {
                            int idx = bits & 3;
                            bits >>= 2;
                            row[px * bpp + 0] = colors[idx][0];
                            row[px * bpp + 1] = colors[idx][1];
                            row[px * bpp + 2] = colors[idx][2];
                            if (hasAlpha)
                                row[px * bpp + 3] = blockAlpha[py * 4 + px];
                        }
                        for (int px = maxPx; px < 4; ++px)
                            bits >>= 2;
                    }
                }
                else // DXT5
                {
                    const auto* blk = reinterpret_cast<const DXT5Block*>(srcData);
                    srcData += sizeof(DXT5Block);

                    // Decode interpolated alpha
                    uint8_t alphaTable[8];
                    alphaTable[0] = blk->alpha_0;
                    alphaTable[1] = blk->alpha_1;
                    if (blk->alpha_0 > blk->alpha_1)
                    {
                        alphaTable[2] = (6 * alphaTable[0] + 1 * alphaTable[1] + 3) / 7;
                        alphaTable[3] = (5 * alphaTable[0] + 2 * alphaTable[1] + 3) / 7;
                        alphaTable[4] = (4 * alphaTable[0] + 3 * alphaTable[1] + 3) / 7;
                        alphaTable[5] = (3 * alphaTable[0] + 4 * alphaTable[1] + 3) / 7;
                        alphaTable[6] = (2 * alphaTable[0] + 5 * alphaTable[1] + 3) / 7;
                        alphaTable[7] = (1 * alphaTable[0] + 6 * alphaTable[1] + 3) / 7;
                    }
                    else
                    {
                        alphaTable[2] = (4 * alphaTable[0] + 1 * alphaTable[1] + 2) / 5;
                        alphaTable[3] = (3 * alphaTable[0] + 2 * alphaTable[1] + 2) / 5;
                        alphaTable[4] = (2 * alphaTable[0] + 3 * alphaTable[1] + 2) / 5;
                        alphaTable[5] = (1 * alphaTable[0] + 4 * alphaTable[1] + 2) / 5;
                        alphaTable[6] = 0;
                        alphaTable[7] = 255;
                    }

                    // 48 bits of alpha indices (3 bits each, 16 pixels)
                    uint64_t alphaBits = 0;
                    for (int i = 0; i < 6; ++i)
                        alphaBits |= static_cast<uint64_t>(blk->alphaIdx[i]) << (8 * i);
                    for (int i = 0; i < 16; ++i)
                    {
                        blockAlpha[i] = alphaTable[(alphaBits >> (3 * i)) & 7];
                    }

                    rgb565to888(blk->color_0, colors[0][0], colors[0][1], colors[0][2]);
                    rgb565to888(blk->color_1, colors[1][0], colors[1][1], colors[1][2]);
                    for (int c = 0; c < 3; ++c)
                    {
                        colors[2][c] = (2 * colors[0][c] + colors[1][c] + 1) / 3;
                        colors[3][c] = (colors[0][c] + 2 * colors[1][c] + 1) / 3;
                    }

                    uint32_t bits = blk->texels;
                    const int maxPy = std::min(4, h - by * 4);
                    const int maxPx = std::min(4, w - bx * 4);
                    for (int py = 0; py < maxPy; ++py)
                    {
                        unsigned char* row = dstData + (by * 4 + py) * rowBytes + bx * 4 * bpp;
                        for (int px = 0; px < maxPx; ++px)
                        {
                            int idx = bits & 3;
                            bits >>= 2;
                            row[px * bpp + 0] = colors[idx][0];
                            row[px * bpp + 1] = colors[idx][1];
                            row[px * bpp + 2] = colors[idx][2];
                            if (hasAlpha)
                                row[px * bpp + 3] = blockAlpha[py * 4 + px];
                        }
                        for (int px = maxPx; px < 4; ++px)
                            bits >>= 2;
                    }
                }
            }
        }
    }
#endif

}

namespace Resource
{

    ImageManager::ImageManager(const VFS::Manager* vfs, double expiryDelay)
        : ResourceManager(vfs, expiryDelay)
        , mWarningImage(createWarningImage())
        , mOptions(new osgDB::Options("dds_flip dds_dxt1_detect_rgba ignoreTga2Fields"))
        , mOptionsNoFlip(new osgDB::Options("dds_dxt1_detect_rgba ignoreTga2Fields"))
    {
    }

    ImageManager::~ImageManager() {}

    bool checkSupported(osg::Image* image)
    {
        switch (image->getPixelFormat())
        {
            case (GL_COMPRESSED_RGB_S3TC_DXT1_EXT):
            case (GL_COMPRESSED_RGBA_S3TC_DXT1_EXT):
            case (GL_COMPRESSED_RGBA_S3TC_DXT3_EXT):
            case (GL_COMPRESSED_RGBA_S3TC_DXT5_EXT):
            {
                if (!SceneUtil::glExtensionsReady())
                    return true; // hashtag yolo (CS might not have context when loading assets)
                osg::GLExtensions& exts = SceneUtil::getGLExtensions();
                if (!exts.isTextureCompressionS3TCSupported
                    // This one works too. Should it be included in isTextureCompressionS3TCSupported()? Submitted as a
                    // patch to OSG.
                    && !osg::isGLExtensionSupported(exts.contextID, "GL_S3_s3tc"))
                {
                    return false;
                }
                break;
            }
            // not bothering with checks for other compression formats right now
            default:
                return true;
        }
        return true;
    }

    osg::ref_ptr<osg::Image> ImageManager::getImage(VFS::Path::NormalizedView path, bool disableFlip)
    {
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(path);
        if (obj)
            return osg::ref_ptr<osg::Image>(static_cast<osg::Image*>(obj.get()));
        else
        {
            Files::IStreamPtr stream;
            try
            {
                stream = mVFS->get(path);
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << "Failed to open image: " << e.what();
                mCache->addEntryToObjectCache(path.value(), mWarningImage);
                return mWarningImage;
            }

            const std::string ext(Misc::getFileExtension(path.value()));
            osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
            if (!reader)
            {
                Log(Debug::Error) << "Error loading " << path << ": no readerwriter for '" << ext << "' found";
                mCache->addEntryToObjectCache(path.value(), mWarningImage);
                return mWarningImage;
            }

            bool killAlpha = false;
            if (reader->supportedExtensions().count("tga"))
            {
                // Morrowind ignores the alpha channel of 16bpp TGA files even when the header says not to
                unsigned char header[18];
                stream->read((char*)header, 18);
                if (stream->gcount() != 18)
                {
                    Log(Debug::Error) << "Error loading " << path << ": couldn't read TGA header";
                    mCache->addEntryToObjectCache(path.value(), mWarningImage);
                    return mWarningImage;
                }
                int type = header[2];
                int depth;
                if (type == 1 || type == 9)
                    depth = header[7];
                else
                    depth = header[16];
                int alphaBPP = header[17] & 0x0F;
                killAlpha = depth == 16 && alphaBPP == 1;
                stream->seekg(0);
            }

            osgDB::ReaderWriter::ReadResult result
                = reader->readImage(*stream, disableFlip ? mOptionsNoFlip : mOptions);
            if (!result.success())
            {
                Log(Debug::Error) << "Error loading " << path << ": " << result.message() << " code "
                                  << result.status();
                mCache->addEntryToObjectCache(path.value(), mWarningImage);
                return mWarningImage;
            }

            osg::ref_ptr<osg::Image> image = result.getImage();

#ifdef __vita__
            // Decompress DXT textures to RGBA on Vita.
            // vitaGL's glCompressedTexImage2D has partial DXT support and our
            // glCompressedTexSubImage2D is a no-op stub — some textures render
            // as solid green (uninitialized VRAM). CPU decompression is safe.
            if (image->isCompressed())
            {
                GLenum fmt = image->getPixelFormat();
                bool isDXT = (fmt == GL_COMPRESSED_RGB_S3TC_DXT1_EXT
                           || fmt == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
                           || fmt == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
                           || fmt == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);

                osg::ref_ptr<osg::Image> newImage = new osg::Image;
                newImage->setFileName(image->getFileName());
                GLenum outFmt = image->isImageTranslucent() ? GL_RGBA : GL_RGB;
                newImage->allocateImage(image->s(), image->t(), image->r(), outFmt, GL_UNSIGNED_BYTE);

                if (isDXT && image->r() == 1)
                    decompressDXT(image.get(), newImage.get());
                else
                {
                    // Fallback for non-DXT compressed or 3D textures
                    for (int s = 0; s < image->s(); ++s)
                        for (int t = 0; t < image->t(); ++t)
                            for (int r = 0; r < image->r(); ++r)
                                newImage->setColor(image->getColor(s, t, r), s, t, r);
                }
                image = newImage;
            }

            // Cap world textures at 128px max edge. Cuts VRAM, texture-cache
            // pressure, and upload bandwidth. Skip UI/fonts/book art so text
            // and menu icons stay crisp. Skip normal/spec maps defensively
            // (usually disabled on Vita anyway).
            {
                std::string_view p = path.value();
                bool isUI = p.find("menu") != std::string_view::npos
                    || p.find("Menu") != std::string_view::npos
                    || p.find("cursor") != std::string_view::npos
                    || p.find("font") != std::string_view::npos
                    || p.find("Font") != std::string_view::npos
                    || p.find("bookart") != std::string_view::npos
                    || p.find("levelup") != std::string_view::npos
                    || p.find("scroll") != std::string_view::npos
                    || p.find("splash") != std::string_view::npos;

                bool isNormalSpec = p.find("_n.") != std::string_view::npos
                    || p.find("_nm.") != std::string_view::npos
                    || p.find("_s.") != std::string_view::npos;

                constexpr int kMaxEdge = 128;
                if (!isUI && !isNormalSpec && !image->isCompressed()
                    && image->s() > 1 && image->t() > 1)
                {
                    int s = image->s(), t = image->t();
                    while ((s > kMaxEdge || t > kMaxEdge) && s > 1 && t > 1)
                    {
                        s = std::max(s / 2, 1);
                        t = std::max(t / 2, 1);
                    }
                    if (s != image->s() || t != image->t())
                        image->scaleImage(s, t, image->r());
                }
            }
#endif

            image->setFileName(std::string(path.value()));
            if (!checkSupported(image))
            {
                static bool uncompress = (getenv("OPENMW_DECOMPRESS_TEXTURES") != nullptr);
                if (!uncompress)
                {
                    Log(Debug::Error) << "Error loading " << path << ": no S3TC texture compression support installed";
                    mCache->addEntryToObjectCache(path.value(), mWarningImage);
                    return mWarningImage;
                }
                else
                {
                    // decompress texture in software if not supported by GPU
                    // requires update to getColor() to be released with OSG 3.6
                    osg::ref_ptr<osg::Image> newImage = new osg::Image;
                    newImage->setFileName(image->getFileName());
                    newImage->allocateImage(image->s(), image->t(), image->r(),
                        image->isImageTranslucent() ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE);
                    for (int s = 0; s < image->s(); ++s)
                        for (int t = 0; t < image->t(); ++t)
                            for (int r = 0; r < image->r(); ++r)
                                newImage->setColor(image->getColor(s, t, r), s, t, r);
                    image = newImage;
                }
            }
            else if (killAlpha)
            {
                osg::ref_ptr<osg::Image> newImage = new osg::Image;
                newImage->setFileName(image->getFileName());
                newImage->allocateImage(image->s(), image->t(), image->r(), GL_RGB, GL_UNSIGNED_BYTE);
                // OSG just won't write the alpha as there's nowhere to put it.
                for (int s = 0; s < image->s(); ++s)
                    for (int t = 0; t < image->t(); ++t)
                        for (int r = 0; r < image->r(); ++r)
                            newImage->setColor(image->getColor(s, t, r), s, t, r);
                image = newImage;
            }

            mCache->addEntryToObjectCache(path.value(), image);
            return image;
        }
    }

    osg::Image* ImageManager::getWarningImage()
    {
        return mWarningImage;
    }

    void ImageManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("Image", frameNumber, mCache->getStats(), *stats);
    }

}
