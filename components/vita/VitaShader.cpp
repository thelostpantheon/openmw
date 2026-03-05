#ifdef __vita__

#include "VitaShader.h"

#include <string>

#include <osg/Node>
#include <osg/Shader>
#include <osg/Texture2D>
#include <osg/Uniform>

namespace Vita
{

    // ==================== VitaLit Shaders ====================
    // Handles objects, actors, sky, batched statics.
    // Per-vertex (Gouraud) lighting with sun + ambient, linear fog, alpha test, single diffuse texture.

    static const char* s_litVertSource =
        "uniform mat4 osg_ModelViewProjectionMatrix;\n"
        "uniform mat4 osg_ModelViewMatrix;\n"
        "uniform mat3 osg_NormalMatrix;\n"
        "\n"
        "uniform vec3 u_sunDirView;\n"
        "uniform vec3 u_sunColor;\n"
        "uniform vec3 u_ambient;\n"
        "uniform float u_fogStart;\n"
        "uniform float u_fogEnd;\n"
        "\n"
        "uniform int colorMode;\n"
        "uniform vec4 u_materialDiffuse;\n"
        "uniform vec4 u_materialAmbient;\n"
        "uniform vec4 u_materialEmission;\n"
        "\n"
        "uniform vec4 u_lightPos0;\n"
        "uniform vec4 u_lightDiffuse0;\n"
        "uniform vec4 u_lightAtten0;\n"
        "uniform vec4 u_lightPos1;\n"
        "uniform vec4 u_lightDiffuse1;\n"
        "uniform vec4 u_lightAtten1;\n"
        "uniform vec4 u_lightPos2;\n"
        "uniform vec4 u_lightDiffuse2;\n"
        "uniform vec4 u_lightAtten2;\n"
        "uniform vec4 u_lightPos3;\n"
        "uniform vec4 u_lightDiffuse3;\n"
        "uniform vec4 u_lightAtten3;\n"
        "uniform vec4 u_lightPos4;\n"
        "uniform vec4 u_lightDiffuse4;\n"
        "uniform vec4 u_lightAtten4;\n"
        "uniform vec4 u_lightPos5;\n"
        "uniform vec4 u_lightDiffuse5;\n"
        "uniform vec4 u_lightAtten5;\n"
        "uniform vec4 u_lightPos6;\n"
        "uniform vec4 u_lightDiffuse6;\n"
        "uniform vec4 u_lightAtten6;\n"
        "\n"
        "attribute vec4 osg_Vertex;\n"
        "attribute vec3 osg_Normal;\n"
        "attribute vec4 osg_Color;\n"
        "attribute vec2 osg_MultiTexCoord0;\n"
        "\n"
        "varying vec2 v_texCoord;\n"
        "varying vec4 v_color;\n"
        "varying float v_fogFactor;\n"
        "\n"
        "vec3 calcPointLight(vec3 pos, vec4 lightPos, vec4 lightDiff, vec4 lightAtt, vec3 normal, vec3 matDiff) {\n"
        "    // Early exit if light is inactive (alpha = 0 means no light)\n"
        "    if (lightDiff.a < 0.001) return vec3(0.0);\n"
        "    \n"
        "    vec3 pl = lightPos.xyz - pos;\n"
        "    float pd = length(pl) + 0.001;\n"
        "    float pa = 1.0 / (lightAtt.x + lightAtt.y * pd + lightAtt.z * pd * pd);\n"
        "    return lightDiff.rgb * matDiff * max(dot(normal, pl / pd), 0.0) * pa;\n"
        "}\n"
        "\n"
        "void main() {\n"
        "    gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;\n"
        "    vec3 viewPos = (osg_ModelViewMatrix * osg_Vertex).xyz;\n"
        "    vec3 viewNormal = normalize(osg_NormalMatrix * osg_Normal);\n"
        "\n"
        "    vec3 emission = u_materialEmission.rgb;\n"
        "    vec4 matDiffuse = u_materialDiffuse;\n"
        "    vec4 matAmbient = u_materialAmbient;\n"
        "\n"
        "    if (colorMode == 1) {\n"
        "        emission = osg_Color.rgb;\n"
        "    } else if (colorMode == 2) {\n"
        "        matDiffuse = osg_Color;\n"
        "        matAmbient = osg_Color;\n"
        "    } else if (colorMode == 3) {\n"
        "        matAmbient = osg_Color;\n"
        "    } else if (colorMode == 4) {\n"
        "        matDiffuse = osg_Color;\n"
        "    }\n"
        "\n"
        "    float NdotL = max(dot(viewNormal, u_sunDirView), 0.0);\n"
        "    vec3 lighting = emission\n"
        "                  + u_ambient * matAmbient.rgb\n"
        "                  + u_sunColor * matDiffuse.rgb * NdotL;\n"
        "\n"
        "    lighting += calcPointLight(viewPos, u_lightPos0, u_lightDiffuse0, u_lightAtten0, viewNormal, matDiffuse.rgb);\n"
        "    lighting += calcPointLight(viewPos, u_lightPos1, u_lightDiffuse1, u_lightAtten1, viewNormal, matDiffuse.rgb);\n"
        "    lighting += calcPointLight(viewPos, u_lightPos2, u_lightDiffuse2, u_lightAtten2, viewNormal, matDiffuse.rgb);\n"
        "    lighting += calcPointLight(viewPos, u_lightPos3, u_lightDiffuse3, u_lightAtten3, viewNormal, matDiffuse.rgb);\n"
        "    lighting += calcPointLight(viewPos, u_lightPos4, u_lightDiffuse4, u_lightAtten4, viewNormal, matDiffuse.rgb);\n"
        "    lighting += calcPointLight(viewPos, u_lightPos5, u_lightDiffuse5, u_lightAtten5, viewNormal, matDiffuse.rgb);\n"
        "    lighting += calcPointLight(viewPos, u_lightPos6, u_lightDiffuse6, u_lightAtten6, viewNormal, matDiffuse.rgb);\n"
        "\n"
        "    v_color = vec4(min(lighting, vec3(1.0)), matDiffuse.a);\n"
        "    v_texCoord = osg_MultiTexCoord0;\n"
        "\n"
        "    float dist = length(viewPos);\n"
        "    v_fogFactor = clamp((u_fogEnd - dist) / (u_fogEnd - u_fogStart), 0.0, 1.0);\n"
        "}\n";

    static const char* s_litFragSource =
        "uniform sampler2D diffuseMap;\n"
        "uniform vec4 u_fogColor;\n"
        "uniform float alphaRef;\n"
        "\n"
        "varying vec2 v_texCoord;\n"
        "varying vec4 v_color;\n"
        "varying float v_fogFactor;\n"
        "\n"
        "void main() {\n"
        "    vec4 tex = texture2D(diffuseMap, v_texCoord);\n"
        "    vec4 color = vec4(tex.rgb * v_color.rgb, tex.a * v_color.a);\n"
        "    if (color.a < alphaRef) discard;\n"
        "    gl_FragColor = mix(u_fogColor, color, v_fogFactor);\n"
        "}\n";

    // ==================== VitaTerrain Shaders ====================
    // Handles terrain chunks. Same lighting as VitaLit but adds blend map sampling
    // on texture unit 1 and texture matrix uniforms for UV scaling.

    static const char* s_terrainVertSource =
        "uniform mat4 osg_ModelViewProjectionMatrix;\n"
        "uniform mat4 osg_ModelViewMatrix;\n"
        "uniform mat3 osg_NormalMatrix;\n"
        "\n"
        "uniform vec3 u_sunDirView;\n"
        "uniform vec3 u_sunColor;\n"
        "uniform vec3 u_ambient;\n"
        "uniform float u_fogStart;\n"
        "uniform float u_fogEnd;\n"
        "\n"
        "uniform int colorMode;\n"
        "uniform vec4 u_materialDiffuse;\n"
        "uniform vec4 u_materialAmbient;\n"
        "uniform vec4 u_materialEmission;\n"
        "\n"
        "uniform mat4 u_texMat0;\n"
        "uniform mat4 u_texMat1;\n"
        "\n"
        "attribute vec4 osg_Vertex;\n"
        "attribute vec3 osg_Normal;\n"
        "attribute vec4 osg_Color;\n"
        "attribute vec2 osg_MultiTexCoord0;\n"
        "attribute vec2 osg_MultiTexCoord1;\n"
        "\n"
        "varying vec2 v_texCoord;\n"
        "varying vec2 v_texCoord2;\n"
        "varying vec4 v_color;\n"
        "varying float v_fogFactor;\n"
        "\n"
        "void main() {\n"
        "    gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;\n"
        "    vec3 viewPos = (osg_ModelViewMatrix * osg_Vertex).xyz;\n"
        "    vec3 viewNormal = normalize(osg_NormalMatrix * osg_Normal);\n"
        "\n"
        "    vec3 emission = u_materialEmission.rgb;\n"
        "    vec4 matDiffuse = u_materialDiffuse;\n"
        "    vec4 matAmbient = u_materialAmbient;\n"
        "\n"
        "    if (colorMode == 1) {\n"
        "        emission = osg_Color.rgb;\n"
        "    } else if (colorMode == 2) {\n"
        "        matDiffuse = osg_Color;\n"
        "        matAmbient = osg_Color;\n"
        "    } else if (colorMode == 3) {\n"
        "        matAmbient = osg_Color;\n"
        "    } else if (colorMode == 4) {\n"
        "        matDiffuse = osg_Color;\n"
        "    }\n"
        "\n"
        "    float NdotL = max(dot(viewNormal, u_sunDirView), 0.0);\n"
        "    vec3 lighting = emission\n"
        "                  + u_ambient * matAmbient.rgb\n"
        "                  + u_sunColor * matDiffuse.rgb * NdotL;\n"
        "\n"
        "    v_color = vec4(min(lighting, vec3(1.0)), matDiffuse.a);\n"
        "    v_texCoord = (u_texMat0 * vec4(osg_MultiTexCoord0, 0.0, 1.0)).xy;\n"
        "    v_texCoord2 = (u_texMat1 * vec4(osg_MultiTexCoord1, 0.0, 1.0)).xy;\n"
        "\n"
        "    float dist = length(viewPos);\n"
        "    v_fogFactor = clamp((u_fogEnd - dist) / (u_fogEnd - u_fogStart), 0.0, 1.0);\n"
        "}\n";

    static const char* s_terrainFragSource =
        "uniform sampler2D diffuseMap;\n"
        "uniform sampler2D blendMap;\n"
        "uniform int u_hasBlendMap;\n"
        "uniform vec4 u_fogColor;\n"
        "\n"
        "varying vec2 v_texCoord;\n"
        "varying vec2 v_texCoord2;\n"
        "varying vec4 v_color;\n"
        "varying float v_fogFactor;\n"
        "\n"
        "void main() {\n"
        "    vec4 tex = texture2D(diffuseMap, v_texCoord);\n"
        "    float alpha = 1.0;\n"
        "    if (u_hasBlendMap != 0)\n"
        "        alpha = texture2D(blendMap, v_texCoord2).a;\n"
        "    vec4 color = vec4(tex.rgb * v_color.rgb, alpha);\n"
        "    gl_FragColor = mix(u_fogColor, color, v_fogFactor);\n"
        "}\n";

    // ==================== VitaTerrainMulti Shaders ====================
    // Single-pass terrain: samples up to 4 diffuse layers + 3 blendmaps.
    // Reduces terrain draw calls from N-per-chunk to 1-per-chunk.
    // Vertex shader is same as VitaTerrain (one diffuse UV + one blend UV).

    static const char* s_terrainMultiFragSource =
        "uniform sampler2D u_layer0;\n"
        "uniform sampler2D u_layer1;\n"
        "uniform sampler2D u_layer2;\n"
        "uniform sampler2D u_layer3;\n"
        "uniform sampler2D u_blend1;\n"
        "uniform sampler2D u_blend2;\n"
        "uniform sampler2D u_blend3;\n"
        "uniform int u_numLayers;\n"
        "uniform vec4 u_fogColor;\n"
        "\n"
        "varying vec2 v_texCoord;\n"
        "varying vec2 v_texCoord2;\n"
        "varying vec4 v_color;\n"
        "varying float v_fogFactor;\n"
        "\n"
        "void main() {\n"
        "    vec3 color = texture2D(u_layer0, v_texCoord).rgb;\n"
        "    if (u_numLayers > 1) {\n"
        "        float b1 = texture2D(u_blend1, v_texCoord2).a;\n"
        "        color = mix(color, texture2D(u_layer1, v_texCoord).rgb, b1);\n"
        "    }\n"
        "    if (u_numLayers > 2) {\n"
        "        float b2 = texture2D(u_blend2, v_texCoord2).a;\n"
        "        color = mix(color, texture2D(u_layer2, v_texCoord).rgb, b2);\n"
        "    }\n"
        "    if (u_numLayers > 3) {\n"
        "        float b3 = texture2D(u_blend3, v_texCoord2).a;\n"
        "        color = mix(color, texture2D(u_layer3, v_texCoord).rgb, b3);\n"
        "    }\n"
        "    color *= v_color.rgb;\n"
        "    gl_FragColor = mix(u_fogColor, vec4(color, 1.0), v_fogFactor);\n"
        "}\n";

    // ==================== Program Creation ====================

    osg::ref_ptr<osg::Program> createVitaLitProgram()
    {
        static osg::ref_ptr<osg::Program> s_program;
        if (s_program)
            return s_program;

        s_program = new osg::Program;
        s_program->setName("VitaLit");
        s_program->addShader(new osg::Shader(osg::Shader::VERTEX, s_litVertSource));
        s_program->addShader(new osg::Shader(osg::Shader::FRAGMENT, s_litFragSource));

        // Do NOT add explicit addBindAttribLocation() calls here.
        // OSG's State already provides all attribute bindings when
        // setUseVertexAttributeAliasing(true) is enabled, and vitaGL's
        // glsl_attr_map only holds 16 entries — our explicit bindings
        // combined with State's 13 bindings would overflow that buffer.

        return s_program;
    }

    osg::ref_ptr<osg::Program> createVitaTerrainProgram()
    {
        static osg::ref_ptr<osg::Program> s_program;
        if (s_program)
            return s_program;

        s_program = new osg::Program;
        s_program->setName("VitaTerrain");
        s_program->addShader(new osg::Shader(osg::Shader::VERTEX, s_terrainVertSource));
        s_program->addShader(new osg::Shader(osg::Shader::FRAGMENT, s_terrainFragSource));

        // Do NOT add explicit addBindAttribLocation() calls here.
        // OSG's State provides all attribute bindings via vertex attribute aliasing.
        // See VitaLit comment above for details on the vitaGL buffer overflow.

        return s_program;
    }

    osg::ref_ptr<osg::Program> createVitaTerrainMultiProgram()
    {
        static osg::ref_ptr<osg::Program> s_program;
        if (s_program)
            return s_program;

        s_program = new osg::Program;
        s_program->setName("VitaTerrainMulti");
        // Reuse VitaTerrain vertex shader — same UV handling
        s_program->addShader(new osg::Shader(osg::Shader::VERTEX, s_terrainVertSource));
        s_program->addShader(new osg::Shader(osg::Shader::FRAGMENT, s_terrainMultiFragSource));

        return s_program;
    }

    // ==================== Shader Application ====================

    osg::ref_ptr<osg::Texture2D> getWhiteFallbackTexture()
    {
        static osg::ref_ptr<osg::Texture2D> s_white;
        if (s_white)
            return s_white;
        osg::ref_ptr<osg::Image> img = new osg::Image;
        static unsigned char white[] = { 255, 255, 255, 255 };
        img->setImage(1, 1, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, white, osg::Image::NO_DELETE);
        s_white = new osg::Texture2D(img);
        s_white->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        s_white->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        s_white->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        s_white->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        return s_white;
    }

    void applyVitaShader(osg::Node& node, int colorMode, float alphaRef, const osg::Material* mat)
    {
        osg::StateSet* ss = node.getOrCreateStateSet();
        ss->setAttributeAndModes(createVitaLitProgram(),
            osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        ss->addUniform(new osg::Uniform("colorMode", colorMode));
        ss->addUniform(new osg::Uniform("alphaRef", alphaRef));
        ss->addUniform(new osg::Uniform("diffuseMap", 0));
        if (mat)
        {
            ss->addUniform(new osg::Uniform("u_materialDiffuse", mat->getDiffuse(osg::Material::FRONT)));
            ss->addUniform(new osg::Uniform("u_materialAmbient", mat->getAmbient(osg::Material::FRONT)));
            ss->addUniform(new osg::Uniform("u_materialEmission", mat->getEmission(osg::Material::FRONT)));
        }
        else
        {
            ss->addUniform(new osg::Uniform("u_materialDiffuse", osg::Vec4f(1, 1, 1, 1)));
            ss->addUniform(new osg::Uniform("u_materialAmbient", osg::Vec4f(1, 1, 1, 1)));
            ss->addUniform(new osg::Uniform("u_materialEmission", osg::Vec4f(0, 0, 0, 1)));
        }
    }

    // ==================== Scene Uniforms ====================

    void setupSceneUniforms(osg::StateSet* ss)
    {
        ss->addUniform(new osg::Uniform("u_sunDirView", osg::Vec3f(0.f, 0.f, 1.f)));
        ss->addUniform(new osg::Uniform("u_sunColor", osg::Vec3f(1.f, 1.f, 1.f)));
        ss->addUniform(new osg::Uniform("u_ambient", osg::Vec3f(0.3f, 0.3f, 0.3f)));
        ss->addUniform(new osg::Uniform("u_fogStart", 0.f));
        ss->addUniform(new osg::Uniform("u_fogEnd", 2048.f));
        ss->addUniform(new osg::Uniform("u_fogColor", osg::Vec4f(0.7f, 0.7f, 0.7f, 1.f)));

        // Default material uniforms (overridden per-node by applyVitaShader)
        ss->addUniform(new osg::Uniform("u_materialDiffuse", osg::Vec4f(1, 1, 1, 1)));
        ss->addUniform(new osg::Uniform("u_materialAmbient", osg::Vec4f(1, 1, 1, 1)));
        ss->addUniform(new osg::Uniform("u_materialEmission", osg::Vec4f(0, 0, 0, 1)));

        // Default point light uniforms (zero diffuse = no contribution)
        for (int i = 0; i < 7; ++i)
        {
            std::string idx = std::to_string(i);
            ss->addUniform(new osg::Uniform(("u_lightPos" + idx).c_str(), osg::Vec4f(0, 0, 0, 0)));
            ss->addUniform(new osg::Uniform(("u_lightDiffuse" + idx).c_str(), osg::Vec4f(0, 0, 0, 0)));
            ss->addUniform(new osg::Uniform(("u_lightAtten" + idx).c_str(), osg::Vec4f(1, 0, 0, 0)));
        }

        // Default terrain uniforms
        ss->addUniform(new osg::Uniform("u_texMat0", osg::Matrixf::identity()));
        ss->addUniform(new osg::Uniform("u_texMat1", osg::Matrixf::identity()));
        ss->addUniform(new osg::Uniform("u_hasBlendMap", 0));
        ss->addUniform(new osg::Uniform("u_numLayers", 1));
    }

    void updateSceneUniforms(osg::StateSet* ss, const osg::Vec3f& sunDirView, const osg::Vec3f& sunColor,
        const osg::Vec3f& ambient, float fogStart, float fogEnd, const osg::Vec4f& fogColor)
    {
        if (osg::Uniform* u = ss->getUniform("u_sunDirView"))
            u->set(sunDirView);
        if (osg::Uniform* u = ss->getUniform("u_sunColor"))
            u->set(sunColor);
        if (osg::Uniform* u = ss->getUniform("u_ambient"))
            u->set(ambient);
        if (osg::Uniform* u = ss->getUniform("u_fogStart"))
            u->set(fogStart);
        if (osg::Uniform* u = ss->getUniform("u_fogEnd"))
            u->set(fogEnd);
        if (osg::Uniform* u = ss->getUniform("u_fogColor"))
            u->set(fogColor);
    }

} // namespace Vita

#endif // __vita__
