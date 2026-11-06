// veil: our own GL pass for blur/pixel covers (Hyprland has no pixelate, and its blur path
// can't both blur at full strength and keep the covered surface invisible in a share buffer).
// Include after the Hyprland headers.
#pragma once

namespace Shade {
    struct SDraw {
        CBox                 box;      // target, share buffer px
        SP<Render::ITexture> src;      // blurred screen (blur) or mirror (pixel), monitor-sized
        Vector2D             uv0, uv1; // box corners in src UVs
        SP<Render::ITexture> mask;     // optional: draw only where mask alpha > 0 (same box)
        CRegion              clip;     // optional: buffer px, empty = whole box
        float                radius = 0.F; // rounded box when there is no mask, buffer px
        float                block  = 0.F; // > 0 = pixelate with cells this size, buffer px
    };

    // False when it couldn't draw (shader failed, unsupported mask texture): caller hides as black.
    bool draw(Render::CRenderContext& ctx, const SDraw& d);
    void destroy();
}
