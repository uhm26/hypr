-- Look and feel: gaps, borders, rounding, blur, animations, tiling layout.
-- Options: https://wiki.hypr.land/configuring/core/config-options/
-- Colors and times come from palette.lua (monochrome glass).

local p = require("palette")

hl.config({
    general = {
        gaps_in     = 5,
        gaps_out    = 20,
        border_size = p.frame,
        col = {
            active_border   = p.rgba("bright", 0.30), -- monochrome hairline
            inactive_border = p.rgba("bright", 0.08),
        },
        resize_on_border = false, -- true: resize by dragging borders/gaps
        allow_tearing    = false,
        layout           = "dwindle",
    },

    decoration = {
        rounding       = 10,
        rounding_power = 2,
        -- Soft, wide shadow for depth instead of a hard dark edge.
        shadow = { enabled = true, range = 24, render_power = 3, color = p.rgba("black", 0.45) },
        -- Glass: wide smooth blur, slightly darkened so text stays readable on any
        -- wallpaper. No noise (grain) and no vibrancy (it adds color). Only shows on
        -- see-through surfaces (bar, menus, transparent apps).
        blur = {
            enabled    = true,
            size       = 6,
            passes     = 3,
            noise      = 0,
            contrast   = 1,
            brightness = 0.8,
            vibrancy   = 0,
            popups     = true, -- right-click menus get the glass too
        },
    },

    dwindle = {
        preserve_split = true,
    },

    animations = {
        enabled = true,
    },
})

-- Animation curves (Hyprland defaults)
hl.curve("easeOutQuint", { type = "bezier", points = { { 0.23, 1 },    { 0.32, 1 } } })
hl.curve("linear",       { type = "bezier", points = { { 0, 0 },       { 1, 1 } } })
hl.curve("almostLinear", { type = "bezier", points = { { 0.5, 0.5 },   { 0.75, 1 } } })
hl.curve("quick",        { type = "bezier", points = { { 0.15, 0 },    { 0.1, 1 } } })

-- Springs by feel, like boltsnap (github.com/drvcvt/boltsnap, src/ui/theme.rs):
-- `duration` = seconds to (almost) settle, `bounce` 0 = never overshoots.
-- Same model as Apple's Spring(duration:bounce:). Hyprland ignores `speed` for springs.
local function spring(name, duration, bounce)
    local omega = 2 * math.pi / duration
    hl.curve(name, { type = "spring", mass = 1, stiffness = omega * omega, damping = 2 * (1 - bounce) * omega })
end
spring("snap",  p.motion.snap,  0) -- small UI: menus and popups coming in
spring("glide", p.motion.glide, 0) -- windows and workspaces moving

hl.animation({ leaf = "global",        enabled = true, speed = 10,   bezier = "default" })
hl.animation({ leaf = "border",        enabled = true, speed = 5.39, bezier = "easeOutQuint" })
hl.animation({ leaf = "windows",       enabled = true, speed = 1,    spring = "glide" })
hl.animation({ leaf = "windowsIn",     enabled = true, speed = 1,    spring = "glide",        style = "popin 87%" })
hl.animation({ leaf = "windowsOut",    enabled = true, speed = 1.49, bezier = "linear",       style = "popin 87%" })
hl.animation({ leaf = "fadeIn",        enabled = true, speed = 1.73, bezier = "almostLinear" })
hl.animation({ leaf = "fadeOut",       enabled = true, speed = 1.46, bezier = "almostLinear" })
hl.animation({ leaf = "fade",          enabled = true, speed = 3.03, bezier = "quick" })
hl.animation({ leaf = "layers",        enabled = true, speed = 3.81, bezier = "easeOutQuint" })
hl.animation({ leaf = "layersIn",      enabled = true, speed = 1,    spring = "snap",         style = "fade" })
hl.animation({ leaf = "layersOut",     enabled = true, speed = 1.5,  bezier = "linear",       style = "fade" })
hl.animation({ leaf = "fadeLayersIn",  enabled = true, speed = 1.79, bezier = "almostLinear" })
hl.animation({ leaf = "fadeLayersOut", enabled = true, speed = 1.39, bezier = "almostLinear" })
hl.animation({ leaf = "workspaces",    enabled = true, speed = 1,    spring = "glide",        style = "fade" })
hl.animation({ leaf = "workspacesIn",  enabled = true, speed = 1,    spring = "glide",        style = "fade" })
hl.animation({ leaf = "workspacesOut", enabled = true, speed = 1,    spring = "glide",        style = "fade" })
hl.animation({ leaf = "zoomFactor",    enabled = true, speed = 7,    bezier = "quick" })
