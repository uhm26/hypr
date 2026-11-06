// veil: the cover shader. One program, two looks:
//  - blur:  samples the pre-blurred screen + ±0.5/255 dither (kills the 8-bit banding that
//           looked blocky; far below visible grain),
//  - pixel: samples the sharp screen in block cells, 4 taps averaged per cell (no shimmer).
// Cut by the covered surface's own alpha (mask) or a rounded box. Hyprland caches GL state
// (program, texture unit, scissor), so we go through its helpers and keep its cache truthful.

#include <src/includes.hpp>
#include <any>
#include <sstream>
#include <string>
#include <vector>

#define private   public
#define protected public
#include <src/render/OpenGL.hpp>
#include <src/render/Renderer.hpp>
#include <src/render/Texture.hpp>
#include <src/output/Monitor.hpp>
#undef private
#undef protected

#include "shade.hpp"

namespace {
    constexpr const char* VERT = R"(#version 300 es
uniform mat3 proj;
in vec2 pos;
out vec2 uv;
void main() {
    uv = pos;
    gl_Position = vec4((proj * vec3(pos, 1.0)).xy, 0.0, 1.0);
})";

    constexpr const char* FRAG = R"(#version 300 es
precision highp float;
uniform sampler2D src;
uniform sampler2D mask;
uniform int   useMask; // 0 none, 1 alpha, 2 opaque texture (RGBX: whole box)
uniform vec2  uv0;
uniform vec2  uv1;
uniform vec2  boxPx;
uniform float radius;
uniform float block;
in vec2 uv;
out vec4 color;

vec3 at(vec2 t) { return texture(src, mix(uv0, uv1, clamp(t, 0.0, 1.0))).rgb; }

void main() {
    if (useMask == 1 && texture(mask, uv).a <= 0.0)
        discard;
    if (useMask == 0 && radius > 0.0) {
        vec2 q = abs(uv * boxPx - boxPx * 0.5) - (boxPx * 0.5 - radius);
        if (length(max(q, 0.0)) > radius)
            discard;
    }
    vec3 c;
    if (block > 0.0) {
        vec2 cell = floor(uv * boxPx / block) * block; // cells anchored to the box: move with it
        vec2 q    = block / boxPx;
        vec2 o    = cell / boxPx;
        c = (at(o + q * vec2(0.25, 0.25)) + at(o + q * vec2(0.75, 0.25)) + at(o + q * vec2(0.25, 0.75)) + at(o + q * vec2(0.75, 0.75))) * 0.25;
    } else {
        float n = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
        c = at(uv) + (n - 0.5) / 255.0;
    }
    color = vec4(c, 1.0);
})";

    struct SProgram {
        GLuint prog = 0, vao = 0, vbo = 0;
        GLint  proj = -1, src = -1, mask = -1, useMask = -1, uv0 = -1, uv1 = -1, boxPx = -1, radius = -1, block = -1, pos = -1;
        bool   failed = false;
    } g_p;

    GLuint compile(GLenum type, const char* code) {
        const GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &code, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            glDeleteShader(s);
            return 0;
        }
        return s;
    }

    bool link() {
        const GLuint v = compile(GL_VERTEX_SHADER, VERT), f = compile(GL_FRAGMENT_SHADER, FRAG);
        if (v && f) {
            g_p.prog = glCreateProgram();
            glAttachShader(g_p.prog, v);
            glAttachShader(g_p.prog, f);
            glLinkProgram(g_p.prog);
        }
        glDeleteShader(v);
        glDeleteShader(f);
        GLint ok = 0;
        if (g_p.prog)
            glGetProgramiv(g_p.prog, GL_LINK_STATUS, &ok);
        return ok;
    }

    // Unit quad as a triangle strip; uv = position, same convention as Hyprland's own quads.
    void makeQuad() {
        static constexpr GLfloat QUAD[] = {0, 0, 1, 0, 0, 1, 1, 1};
        glGenVertexArrays(1, &g_p.vao);
        glGenBuffers(1, &g_p.vbo);
        glBindVertexArray(g_p.vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_p.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), QUAD, GL_STATIC_DRAW);
        glEnableVertexAttribArray(g_p.pos);
        glVertexAttribPointer(g_p.pos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    bool ready() {
        if (g_p.prog || g_p.failed)
            return !g_p.failed;
        if (!link()) {
            g_p.failed = true;
            return false;
        }
        const auto u = [](const char* n) { return glGetUniformLocation(g_p.prog, n); };
        g_p.proj = u("proj"), g_p.src = u("src"), g_p.mask = u("mask"), g_p.useMask = u("useMask");
        g_p.uv0 = u("uv0"), g_p.uv1 = u("uv1"), g_p.boxPx = u("boxPx"), g_p.radius = u("radius"), g_p.block = u("block");
        g_p.pos = glGetAttribLocation(g_p.prog, "pos");
        makeQuad();
        return true;
    }

    int maskKind(const SP<Render::ITexture>& mask) {
        if (!mask)
            return 0;
        return mask->m_type == Render::TEXTURE_RGBX ? 2 : 1;
    }

    void bindTextures(const Shade::SDraw& d) {
        auto* gl = Render::GL::g_pHyprOpenGL.get();
        gl->setActiveTexture(GL_TEXTURE0);
        d.src->bind();
        if (d.mask) {
            gl->setActiveTexture(GL_TEXTURE1);
            d.mask->bind();
            gl->setActiveTexture(GL_TEXTURE0);
        }
    }

    void setUniforms(Render::CRenderContext& ctx, const Shade::SDraw& d) {
        const auto m = g_pHyprRenderer->projectBoxToTarget(ctx, d.box);
        glUniformMatrix3fv(g_p.proj, 1, GL_TRUE, m.getMatrix().data());
        glUniform1i(g_p.src, 0);
        glUniform1i(g_p.mask, 1);
        glUniform1i(g_p.useMask, maskKind(d.mask));
        glUniform2f(g_p.uv0, d.uv0.x, d.uv0.y);
        glUniform2f(g_p.uv1, d.uv1.x, d.uv1.y);
        glUniform2f(g_p.boxPx, d.box.w, d.box.h);
        glUniform1f(g_p.radius, d.radius);
        glUniform1f(g_p.block, d.block);
    }
}

bool Shade::draw(Render::CRenderContext& ctx, const SDraw& d) {
    if (!d.src || !d.src->ok() || d.src->m_type == Render::TEXTURE_EXTERNAL || d.box.w <= 0 || d.box.h <= 0)
        return false;
    if (d.mask && (!d.mask->ok() || d.mask->m_type == Render::TEXTURE_EXTERNAL))
        return false; // sampler2D can't read external images
    if (!ready())
        return false;

    auto* gl = Render::GL::g_pHyprOpenGL.get();
    glUseProgram(g_p.prog);
    gl->m_currentProgram = g_p.prog; // keep Hyprland's useShader() cache truthful
    bindTextures(d);
    setUniforms(ctx, d);
    glBindVertexArray(g_p.vao);

    CRegion area{d.box.x, d.box.y, d.box.w, d.box.h};
    if (!d.clip.empty())
        area.intersect(d.clip);
    area.forEachRect([&](const auto& r) {
        gl->scissor(ctx, &r, false);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    });
    gl->scissor(ctx, nullptr, false);
    glBindVertexArray(0);
    return true;
}

void Shade::destroy() {
    if (g_p.vbo)
        glDeleteBuffers(1, &g_p.vbo);
    if (g_p.vao)
        glDeleteVertexArrays(1, &g_p.vao);
    if (g_p.prog)
        glDeleteProgram(g_p.prog);
    g_p = {};
}
