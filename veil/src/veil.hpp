// veil: what to paint over a hidden window/layer in screen shares, and painting it.
// Include after the Hyprland headers (it uses their types; see the .cpp files).
#pragma once

#include <cstdint>
#include <string_view>

namespace Veil {
    enum class eMode : uint8_t {
        NONE,
        BLACK,
        BLUR,
        PIXEL,
    };

    // Rule value -> mode. Anything unknown hides as BLACK: a typo must never leak a window.
    eMode parseMode(std::string_view value);

    // Registers the `veil` window + layer rule. Returns false if Hyprland refused.
    bool registerRules();
    void unregisterRules();

    // A window's / layer's veil mode (NONE if not veiled or null).
    eMode windowMode(const PHLWINDOW& window);
    eMode layerMode(const PHLLS& layer);

    // Called when Hyprland snapshots a closing window/layer: keep covering it while it animates out.
    void trackFadeout(eMode mode, const SP<Desktop::IFadeout>& fade);

    // Paints every veiled window, layer, popup and closing layer into a monitor/region share frame.
    void paint(Screenshare::CScreenshareFrame& frame, Render::CRenderContext& ctx);
}
