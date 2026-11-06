// veil: Hyprland plugin entry. Hooks the monitor/region screenshare render (paint covers) and
// window/layer unmap (keep covering closing windows/layers while they animate out).

// std + Hyprland headers first: `private public` below must not touch libstdc++ internals.
#include <src/includes.hpp>
#include <any>
#include <sstream>
#include <string>
#include <vector>

#define private   public
#define protected public
#include <src/plugins/PluginAPI.hpp>
#include <src/managers/screenshare/ScreenshareManager.hpp>
#include <src/pointer/PointerManager.hpp>
#include <src/output/Monitor.hpp>
#include <src/desktop/state/LayerFadeout.hpp>
#include <src/desktop/state/FadingOutState.hpp>
#include <src/desktop/view/LayerSurface.hpp>
#include <src/desktop/view/window/Window.hpp>
#undef private
#undef protected

extern "C" {
#include <lua.h>
}

#include "shade.hpp"
#include "veil.hpp"

namespace {
    constexpr const char* VERSION = "0.9";
    HANDLE         g_handle    = nullptr;
    CFunctionHook* g_hookFrame = nullptr;
    CFunctionHook* g_hookFade  = nullptr;
    CFunctionHook* g_hookWinFade = nullptr;

    using FRenderMonitor = void (*)(Screenshare::CScreenshareFrame*, Render::CRenderContext&);
    using FLayerUnmap    = void (*)(Desktop::View::CLayerSurface*);
    using FWindowUnmap   = void (*)(Desktop::View::CWindow*);

    // Native draws the cursor last; we turn that off, paint our covers, then draw it ourselves
    // so the cursor stays visible on top of a cover (same as native).
    void drawCursor(Screenshare::CScreenshareFrame* f, Render::CRenderContext& ctx) {
        const auto mon  = f->m_session->monitor();
        CRegion    dmg  = {0, 0, f->m_bufferSize.x, f->m_bufferSize.y};
        Vector2D   cpos = Pointer::mgr()->untransformedPosition() - mon->m_position - f->m_session->m_captureBox.pos() / mon->m_scale;
        Pointer::mgr()->renderSoftwareCursorsFor(ctx, mon, Time::steadyNow(), dmg, cpos, true);
    }

    void hkRenderMonitor(Screenshare::CScreenshareFrame* f, Render::CRenderContext& ctx) {
        const auto type = f->m_session->m_type;
        // Same early-outs as native: nothing was drawn, so nothing to cover.
        const bool ours = (type == Screenshare::SHARE_MONITOR || type == Screenshare::SHARE_REGION) && !f->done() && f->m_session->monitor() &&
            ctx.m_data.pMonitor && ctx.m_data.pMonitor->resources()->getMirrorTexture();

        const bool cursor  = f->m_overlayCursor;
        f->m_overlayCursor = false;
        reinterpret_cast<FRenderMonitor>(g_hookFrame->m_original)(f, ctx);
        f->m_overlayCursor = cursor;

        if (!ours)
            return;
        Veil::paint(*f, ctx);
        if (cursor)
            drawCursor(f, ctx);
    }

    // Runs an unmap and tracks every fade it appended (walks back to the old last one; if that
    // got cleaned up meanwhile it tracks all: over-covering is safe). Mode is read before the
    // unmap, while the rules still apply.
    template <class F>
    void trackNewFades(Veil::eMode mode, F&& unmap) {
        const auto& fades  = Desktop::fadingOutState()->fadeouts();
        const auto* before = fades.empty() ? nullptr : fades.back().get();
        unmap();
        if (mode == Veil::eMode::NONE)
            return;
        for (auto it = fades.rbegin(); it != fades.rend() && it->get() != before; ++it)
            Veil::trackFadeout(mode, *it);
    }

    // onUnmap snapshots the layer and appends a CLayerFadeout (CLayerFadeout::create itself
    // can't be hooked: its prologue isn't patchable).
    void hkLayerUnmap(Desktop::View::CLayerSurface* self) {
        trackNewFades(Veil::layerMode(self->m_self.lock()), [&] { reinterpret_cast<FLayerUnmap>(g_hookFade->m_original)(self); });
    }

    // unmapWindow is the only caller of CWindowFadeout::create (checked in the 0.57 binary).
    void hkWindowUnmap(Desktop::View::CWindow* self) {
        trackNewFades(Veil::windowMode(self->m_self.lock()), [&] { reinterpret_cast<FWindowUnmap>(g_hookWinFade->m_original)(self); });
    }

    // Hooks the function whose demangled name contains `full` (searched by short `name`).
    CFunctionHook* hookFn(const std::string& name, std::string_view full, void* dest) {
        for (const auto& fn : HyprlandAPI::findFunctionsByName(g_handle, name)) {
            if (!fn.demangled.contains(full))
                continue;
            auto* h = HyprlandAPI::createFunctionHook(g_handle, fn.address, dest);
            return h && h->hook() ? h : nullptr;
        }
        return nullptr;
    }

    // `hl.plugin.veil.version()`. Its existence tells the config the plugin is loaded, so
    // `veil` rules can sit behind `if hl.plugin.veil then` and never error without it.
    int luaVersion(lua_State* L) {
        lua_pushstring(L, VERSION);
        return 1;
    }

    // Same string as the header's __hyprland_api_get_client_hash(), built here: that one keeps it
    // in a process-wide (GNU-unique) static, so an old veil that failed to load and is still mapped
    // answers with ITS hash and a fresh build refuses to load until Hyprland restarts.
    std::string clientHash() {
        const auto noPatch = [](std::string_view v) { return std::string{v.substr(0, v.find_last_of('.'))}; };
        return std::format("{}_aq_{}_hu_{}_hg_{}_hc_{}_hlg_{}", GIT_COMMIT_HASH, noPatch(AQUAMARINE_VERSION), noPatch(HYPRUTILS_VERSION), noPatch(HYPRGRAPHICS_VERSION),
                           noPatch(HYPRCURSOR_VERSION), noPatch(HYPRLANG_VERSION));
    }

    [[noreturn]] void fail(const std::string& why) {
        HyprlandAPI::addNotification(g_handle, "[veil] " + why, CHyprColor{0.75F, 0.29F, 0.29F, 1.F}, 8000);
        throw std::runtime_error("[veil] " + why);
    }
}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    g_handle = handle;

    // Server hash (running Hyprland) vs client hash (headers we built with). Hyprland doesn't
    // check this for us; a mismatch means wrong struct layouts -> crash.
    if (std::string_view{__hyprland_api_get_hash()} != clientHash())
        fail("built for a different Hyprland, rebuild it");
    if (!Veil::registerRules())
        fail("could not register the veil rule");

    // Hyprland removes created hooks itself when init throws or the plugin unloads.
    g_hookFrame = hookFn("renderMonitor", "CScreenshareFrame::renderMonitor", reinterpret_cast<void*>(&hkRenderMonitor));
    g_hookFade    = hookFn("onUnmap", "CLayerSurface::onUnmap", reinterpret_cast<void*>(&hkLayerUnmap));
    g_hookWinFade = hookFn("unmapWindow", "CWindow::unmapWindow", reinterpret_cast<void*>(&hkWindowUnmap));
    if (!g_hookFrame || !g_hookFade || !g_hookWinFade) {
        Veil::unregisterRules();
        fail(std::string{"could not hook "} + (!g_hookFrame ? "renderMonitor" : !g_hookFade ? "CLayerSurface::onUnmap" : "CWindow::unmapWindow"));
    }

    if (!HyprlandAPI::addLuaFunction(handle, "veil", "version", luaVersion))
        HyprlandAPI::addNotification(handle, "[veil] could not add hl.plugin.veil (rules behind it stay off)", CHyprColor{0.75F, 0.29F, 0.29F, 1.F}, 8000);

    // Plugins load after the first config parse, so `veil` rules were skipped then.
    HyprlandAPI::reloadConfig();
    return {"veil", "Hide windows/layers from screen shares: black, blur or pixel", "gabe", VERSION};
}

APICALL EXPORT void PLUGIN_EXIT() {
    Veil::unregisterRules();
    Shade::destroy();
}
