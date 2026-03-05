#!/bin/bash
# Build/install FFmpeg for OpenMW Vita port
# Uses vitasdk/packages if available, otherwise builds from source
# Minimal config: only decoders needed for Morrowind audio/video
set -e

VITASDK="${VITASDK:-/usr/local/vitasdk}"
SYSROOT="${VITASDK}/arm-vita-eabi"

echo "=== Installing FFmpeg for Vita ==="

# Check if vdpm is available
if command -v vdpm &> /dev/null; then
    echo "Using vdpm to install FFmpeg..."
    vdpm ffmpeg
    echo "=== FFmpeg installed via vdpm ==="
    exit 0
fi

# Manual build fallback
WORKDIR="${1:-$(pwd)/vita-deps-build/ffmpeg}"
FFMPEG_VERSION="6.1"

echo "Building FFmpeg ${FFMPEG_VERSION} from source..."
echo "Work dir: ${WORKDIR}"

mkdir -p "${WORKDIR}"
cd "${WORKDIR}"

if [ ! -f ffmpeg_done ]; then
    if [ ! -f "ffmpeg-${FFMPEG_VERSION}.tar.bz2" ]; then
        wget -q "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.bz2"
    fi

    tar xf "ffmpeg-${FFMPEG_VERSION}.tar.bz2"
    cd "ffmpeg-${FFMPEG_VERSION}"

    ./configure \
        --prefix="${SYSROOT}" \
        --cross-prefix="${VITASDK}/bin/arm-vita-eabi-" \
        --arch=armv7-a \
        --cpu=cortex-a9 \
        --target-os=none \
        --enable-cross-compile \
        --enable-static \
        --disable-shared \
        --disable-programs \
        --disable-doc \
        --disable-network \
        --disable-everything \
        --disable-asm \
        --enable-avcodec \
        --enable-avformat \
        --enable-avutil \
        --enable-swscale \
        --enable-swresample \
        --enable-decoder=mp3 \
        --enable-decoder=aac \
        --enable-decoder=pcm_s16le \
        --enable-decoder=pcm_u8 \
        --enable-decoder=vorbis \
        --enable-decoder=bink \
        --enable-decoder=binkaudio_rdft \
        --enable-decoder=binkaudio_dct \
        --enable-decoder=wav \
        --enable-demuxer=mp3 \
        --enable-demuxer=aac \
        --enable-demuxer=ogg \
        --enable-demuxer=wav \
        --enable-demuxer=pcm_s16le \
        --enable-demuxer=bink \
        --enable-parser=mpegaudio \
        --enable-parser=aac \
        --enable-parser=vorbis \
        --enable-protocol=file \
        --extra-cflags="-Os -ftree-vectorize -fomit-frame-pointer -ffast-math -ffunction-sections -fdata-sections -fvisibility=hidden"

    make -j$(nproc)
    make install

    cd ..
    touch ffmpeg_done
fi

echo "=== FFmpeg installation complete ==="
