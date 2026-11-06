// veil: paints the cover for hidden windows/layers into a share frame.
// Mirrors Hyprland 0.57 CScreenshareFrame::renderMonitor's black boxes, with these fixes:
//  - layers/popups are covered even when they don't accept input (click-through or closing),
//  - popup boxes subtract the monitor position (native adds it: wrong off the 0,0 monitor),
//  - closing windows/layers stay covered while they fade/slide out (native forgets them),
//  - blur/pixel on layers follow the surface's own pixels (exact shape), not its whole box,
//  - pixelate mode (own shader, shade.cpp).

// std + Hyprland headers first: `private public` below must not touch libstdc++ internals.
#include <src/includes.hpp>
#include <algorithm>
#include <any>
#include <chrono>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#define private   public
#define protected public
#include <src/managers/screenshare/ScreenshareManager.hpp>
#include <src/render/Renderer.hpp>
#include <src/render/Framebuffer.hpp>
#include <src/render/pass/RectPassElement.hpp>
#include <src/render/pass/TexPassElement.hpp>
#include <src/render/OpenGL.hpp>
#include <src/output/MonitorResources.hpp>
#include <src/output/Monitor.hpp>
#include <src/workspace/HLWorkspace.hpp>
#include <src/desktop/view/window/Window.hpp>
#include <src/desktop/view/window/WindowPresentation.hpp>
#include <src/desktop/view/LayerSurface.hpp>
#include <src/desktop/view/Popup.hpp>
#include <src/desktop/state/WindowState.hpp>
#include <src/desktop/state/LayerState.hpp>
#include <src/desktop/state/LayerFadeout.hpp>
#include <src/desktop/rule/windowRule/WindowRuleEffectContainer.hpp>
#include <src/desktop/rule/layerRule/LayerRuleEffectContainer.hpp>
#include <src/desktop/rule/windowRule/WindowRuleApplicator.hpp>
#include <src/desktop/rule/layerRule/LayerRuleApplicator.hpp>
#include <src/managers/fullscreen/FullscreenController.hpp>
#include <src/protocols/core/Compositor.hpp>
#undef private
#undef protected

#include <src/config/ConfigValue.hpp>

#include "shade.hpp"
#include "veil.hpp"

namespace {
    constexpr const char* RULE = "veil";
    // Pixelate cell size, logical px (times monitor scale).
    constexpr float PIXEL_BLOCK = 16.F;

    Desktop::Rule::CWindowRuleEffectContainer::storageType g_windowIdx = 0;
    Desktop::Rule::CLayerRuleEffectContainer::storageType  g_layerIdx  = 0;

    struct SFade {
        WP<Desktop::IFadeout> fade;
        Veil::eMode           mode;
    };
    std::vector<SFade> g_fades; // closing veiled windows/layers, until their animation ends

    // Shared state for one frame's paint.
    struct SPaint {
        Render::CRenderContext&      ctx;
        PHLMONITOR                   mon;
        CBox                         capture;         // capture box in buffer px
        mutable SP<Render::ITexture> blurred;         // blurred screen, made on first use
        mutable bool                 blurTried = false;
    };

    // The screen frame the share copies from (mirror, everything on screen), blurred at full
    // strength. Not Hyprland's "live" blur: that reads the share buffer, which has no texture,
    // so it silently fell back to the screen's background blur (the blur sat
    // behind the card). shortcut: blurs the whole monitor once per shared frame; blur only
    // the covered boxes if the GPU cost shows up when measured.
    SP<Render::ITexture> blurredScreen(const SPaint& p) {
        if (p.blurTried)
            return p.blurred;
        p.blurTried    = true;
        const auto src = p.mon->resources()->mirrorFB();
        if (!src || !src->getTexture())
            return nullptr;
        const CRegion all{0, 0, p.mon->m_transformedSize.x, p.mon->m_transformedSize.y};
        auto          guard = g_pHyprRenderer->bindTempFB(p.ctx, p.ctx.m_data.currentFB); // blurring rebinds FBs
        const auto    fb    = g_pHyprRenderer->blurFramebuffer(p.ctx, src, 1.F, all, {});
        p.blurred           = fb ? fb->getTexture() : nullptr;
        return p.blurred;
    }

    // Share buffer px -> UVs of a monitor-sized screen texture (adds the region offset back).
    std::pair<Vector2D, Vector2D> screenUV(const SPaint& p, const CBox& box) {
        const auto size = p.mon->m_transformedSize;
        const auto pos  = box.pos() + p.capture.pos();
        return {pos / size, (pos + box.size()) / size};
    }

    // Blur that draws nothing would leak the window, so without working blur hide as black.
    Veil::eMode effective(Veil::eMode mode) {
        static auto PENABLED = CConfigValue<Config::INTEGER>("decoration:blur:enabled");
        if (mode == Veil::eMode::BLUR && !*PENABLED)
            return Veil::eMode::BLACK;
        return mode;
    }

    template <class Applicator, class Idx>
    Veil::eMode modeOf(const Applicator& rules, Idx idx) {
        if (!rules || idx == 0)
            return Veil::eMode::NONE;
        const auto it = rules->m_otherProps.props.find(idx);
        if (it == rules->m_otherProps.props.end() || !it->second)
            return Veil::eMode::NONE;
        return effective(Veil::parseMode(it->second->effect));
    }

    // Global layout coords -> share buffer px.
    CBox toBuffer(const SPaint& p, CBox box) {
        return box.translate(-p.mon->m_position).scale(p.mon->m_scale).translate(-p.capture.pos());
    }

    void coverBlack(const SPaint& p, const CBox& box, int round, float power) {
        g_pHyprRenderer->draw(p.ctx, CRectPassElement::SRectData{.box = box, .color = CHyprColor{0, 0, 0, 1.F}, .round = round, .roundingPower = power}, box);
    }

    // Blur/pixel through our shader (shade.cpp). False = couldn't; caller paints black.
    bool shade(const SPaint& p, Veil::eMode mode, const CBox& box, const SP<Render::ITexture>& mask, const CRegion& clip, float radius) {
        const bool pixel = mode == Veil::eMode::PIXEL;
        const auto src   = pixel ? p.mon->resources()->getMirrorTexture() : blurredScreen(p);
        if (!src)
            return false;
        const auto [uv0, uv1] = screenUV(p, box);
        return Shade::draw(p.ctx,
                           {.box    = box,
                            .src    = src,
                            .uv0    = uv0,
                            .uv1    = uv1,
                            .mask   = mask,
                            .clip   = clip.copy(),
                            .radius = radius,
                            .block  = pixel ? PIXEL_BLOCK * sc<float>(p.mon->m_scale) : 0.F});
    }

    // Box cover (windows, layers in black mode): rounded box.
    void cover(const SPaint& p, const CBox& box, Veil::eMode mode, int round, float power) {
        if (box.w <= 0 || box.h <= 0)
            return;
        if (mode == Veil::eMode::BLACK || !shade(p, mode, box, nullptr, {}, round))
            coverBlack(p, box, round, power); // never leak
    }

    // Cover cut by `tex`'s own alpha: everything the surface draws, nothing else.
    // `clip` (buffer px, may be empty = none) limits it further.
    void coverShaped(const SPaint& p, Veil::eMode mode, const SP<Render::ITexture>& tex, const CBox& box, const CRegion& clip = {}) {
        if (!tex || !tex->ok() || box.w <= 0 || box.h <= 0)
            return;
        if (!shade(p, mode, box, tex, clip, 0.F))
            coverBlack(p, box, 0, 2.F); // never leak
    }

    // Rows of `box` the surface takes input on, full width. Drops the drop-shadow above/below a
    // card (else the blur spills past the card and gets cut flat at the layer top) while a card
    // sliding in sideways stays covered. Infinite or empty input region -> no clip.
    CRegion inputRows(const SP<CWLSurfaceResource>& surf, const CBox& box) {
        auto       input = surf->m_current.input.copy();
        const auto ext   = input.getExtents();
        const auto size  = surf->m_current.size;
        if (input.empty() || size.y <= 0 || (ext.y <= 0 && ext.y + ext.h >= size.y))
            return {};
        const double top = std::max(0.0, ext.y), bottom = std::min(size.y, ext.y + ext.h);
        const double k   = box.h / size.y; // surface px -> buffer px (scale + animation)
        return CRegion{box.x, box.y + top * k, box.w, (bottom - top) * k};
    }

    // Every surface of a tree (`root` + subsurfaces), at global origin `base`, scaled by `scale`
    // (layer open/close animations may resize).
    void coverSurfaces(const SPaint& p, SP<CWLSurfaceResource> root, Vector2D base, Vector2D scale, Veil::eMode mode) {
        if (!root)
            return;
        root->breadthfirst(
            [&](SP<CWLSurfaceResource> surf, const Vector2D& off, void*) {
                const auto box = toBuffer(p, CBox{base + off * scale, surf->m_current.size * scale});
                if (mode != Veil::eMode::BLACK)
                    coverShaped(p, mode, surf->m_current.texture, box, inputRows(surf, box));
                else
                    cover(p, box, mode, 0, 2.F);
            },
            nullptr);
    }

    // Covers every mapped popup under `head`. `base` = global origin of the popup tree.
    void coverPopups(const SPaint& p, WP<Desktop::View::CPopup> head, Vector2D base, Veil::eMode mode) {
        if (!head)
            return;
        head->breadthfirst(
            [&](WP<Desktop::View::CPopup> popup, void*) {
                if (!popup->wlSurface() || !popup->mapped())
                    return;
                coverSurfaces(p, popup->wlSurface()->resource(), base + popup->coordsRelativeToParent(), {1, 1}, mode);
            },
            nullptr);
    }

    void paintLayer(const SPaint& p, const PHLLS& l, Veil::eMode mode) {
        const auto pos  = l->position(Desktop::View::IGeometric::GEOMETRIC_CURRENT);
        const auto size = l->size(Desktop::View::IGeometric::GEOMETRIC_CURRENT);
        const auto geom = l->m_geometry;

        if (mode != Veil::eMode::BLACK && l->wlSurface()) {
            const Vector2D scale = geom.w > 0 && geom.h > 0 ? Vector2D{size.x / geom.w, size.y / geom.h} : Vector2D{1, 1};
            coverSurfaces(p, l->wlSurface()->resource(), pos, scale, mode);
        } else
            cover(p, toBuffer(p, CBox{pos, Vector2D{std::max(size.x, 5.0), std::max(size.y, 5.0)}}), mode, 0, 2.F);

        coverPopups(p, l->popupHead(), pos - geom.pos(), mode);
    }

    void paintLayers(const SPaint& p) {
        for (auto const& l : Desktop::layerState()->layers()) {
            const auto mode = modeOf(l->m_ruleApplicator, g_layerIdx);
            if (mode == Veil::eMode::NONE || !l->mapped() || !l->alphaNonZero())
                continue;
            paintLayer(p, l, mode);
        }
    }

    // Closing windows/layers: their snapshot is a full-monitor texture at an animated,
    // monitor-local box. No snapshot (or black mode) -> cover the animated box itself.
    void paintFades(const SPaint& p) {
        std::erase_if(g_fades, [](const SFade& f) { return !f.fade || f.fade->done(); });
        for (const auto& f : g_fades) {
            const auto fade = f.fade.lock();
            const auto fb   = fade->framebuffer();
            if (fade->monitor() != p.mon || fade->alpha() <= 0.F)
                continue;
            if (f.mode != Veil::eMode::BLACK && fb)
                coverShaped(p, f.mode, fb->getTexture(), fade->renderBox().translate(-p.capture.pos()));
            else
                cover(p, toBuffer(p, fade->geometricBox(Desktop::View::IGeometric::GEOMETRIC_CURRENT)), f.mode, 0, 2.F);
        }
    }

    void paintWindow(const SPaint& p, const PHLWINDOW& w, Veil::eMode mode) {
        const auto ws     = w->m_workspace;
        const auto offset = ws && !(w->m_state & Desktop::View::WINDOW_STATE_PINNED) ? ws->m_renderOffset->value() : Vector2D{};
        const auto pos    = w->position(Desktop::View::IGeometric::GEOMETRIC_CURRENT) + offset;
        const auto size   = w->size(Desktop::View::IGeometric::GEOMETRIC_CURRENT);
        const auto box    = toBuffer(p, CBox{pos, Vector2D{std::max(size.x, 5.0), std::max(size.y, 5.0)}});

        // Same as native: rounding leaks on region captures and fullscreen, so drop it there.
        const bool square = p.capture.pos() != Vector2D{} || Fullscreen::controller()->isFullscreen(w, Fullscreen::FSMODE_FULLSCREEN);
        const int  round  = square ? 0 : w->presentation().rounding() * p.mon->m_scale;
        cover(p, box, mode, round, square ? 2.F : w->presentation().roundingPower());

        if (!w->backend().isX11())
            coverPopups(p, w->popupHead(), pos - w->backend().geometry().box.pos(), mode);
    }

    // A window that just lost its veil stays covered a bit longer: Firefox renames the tab
    // (title rule stops matching) 1-2 frames before it draws the new page.
    constexpr auto LINGER = std::chrono::milliseconds(35);

    struct SLinger {
        PHLWINDOWREF                   window;
        Veil::eMode                    mode;
        std::optional<Time::steady_tp> lostAt; // first frame the rule no longer matched
    };
    std::vector<SLinger> g_lingers;

    // The mode to paint `w` with: its rule's, or the last one for LINGER after the rule drops.
    Veil::eMode lingered(const PHLWINDOW& w, Veil::eMode mode, Time::steady_tp now) {
        const auto it = std::ranges::find_if(g_lingers, [&](const SLinger& l) { return l.window == w; });
        if (mode != Veil::eMode::NONE) {
            if (it == g_lingers.end())
                g_lingers.push_back({.window = w, .mode = mode});
            else
                *it = {.window = w, .mode = mode};
            return mode;
        }
        if (it == g_lingers.end())
            return Veil::eMode::NONE;
        if (!it->lostAt)
            it->lostAt = now;
        if (now - *it->lostAt < LINGER)
            return it->mode;
        g_lingers.erase(it);
        return Veil::eMode::NONE;
    }

    void paintWindows(const SPaint& p) {
        std::erase_if(g_lingers, [](const SLinger& l) { return !l.window; });
        const auto now = Time::steadyNow();
        for (auto const& w : Desktop::windowState()->windows()) {
            const auto mode = lingered(w, modeOf(w->m_ruleApplicator, g_windowIdx), now);
            if (mode == Veil::eMode::NONE || w->isHidden() || !g_pHyprRenderer->shouldRenderWindow(w, p.mon))
                continue;
            paintWindow(p, w, mode);
        }
    }
}

Veil::eMode Veil::parseMode(std::string_view v) {
    if (v.empty() || v == "none" || v == "off" || v == "false" || v == "0")
        return eMode::NONE;
    if (v == "blur")
        return eMode::BLUR;
    if (v == "pixel" || v == "pixelate")
        return eMode::PIXEL;
    return eMode::BLACK;
}

bool Veil::registerRules() {
    g_windowIdx = Desktop::Rule::windowEffects()->registerEffect(RULE);
    g_layerIdx  = Desktop::Rule::layerEffects()->registerEffect(RULE);
    return g_windowIdx != 0 && g_layerIdx != 0;
}

void Veil::unregisterRules() {
    Desktop::Rule::windowEffects()->unregisterEffect(std::string{RULE});
    Desktop::Rule::layerEffects()->unregisterEffect(std::string{RULE});
    g_windowIdx = 0;
    g_layerIdx  = 0;
    g_fades.clear();
    g_lingers.clear();
}

Veil::eMode Veil::windowMode(const PHLWINDOW& window) {
    return window ? modeOf(window->m_ruleApplicator, g_windowIdx) : eMode::NONE;
}

Veil::eMode Veil::layerMode(const PHLLS& layer) {
    return layer ? modeOf(layer->m_ruleApplicator, g_layerIdx) : eMode::NONE;
}

void Veil::trackFadeout(eMode mode, const SP<Desktop::IFadeout>& fade) {
    std::erase_if(g_fades, [](const SFade& f) { return !f.fade || f.fade->done(); });
    if (fade && mode != eMode::NONE)
        g_fades.push_back({.fade = fade, .mode = mode});
}

void Veil::paint(Screenshare::CScreenshareFrame& frame, Render::CRenderContext& ctx) {
    const SPaint p{.ctx = ctx, .mon = frame.m_session->monitor(), .capture = frame.m_session->m_captureBox};
    if (!p.mon)
        return;
    // Layers first: windows on top of a veiled layer still get their own cover after.
    paintFades(p);
    paintLayers(p);
    paintWindows(p);
}
