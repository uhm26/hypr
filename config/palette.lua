-- The one palette: every color, frame size and spring time of the setup lives here
-- (monochrome glass).
-- Use: `local p = require("palette")`, then p.rgba("fg", 0.3) -> "rgba(d4d4d44c)".

local p = {}

p.color = {
    bg      = "0d0d0d",
    surface = "151515",
    line    = "2a2a2a",
    dim     = "6a6a6a",
    fg      = "d4d4d4",
    bright  = "ffffff",
    black   = "000000",
    accent  = "d9c8af", -- the one muted accent; use rarely
    error   = "c04a4a",
}

p.frame = 1 -- px, hairline borders everywhere

-- Springs: seconds to settle, no bounce (live accepted 2026-11-01).
p.motion = {
    snap  = 0.15, -- small UI: menus, popups
    glide = 0.22, -- windows, workspaces
}

-- Hyprland color string from a palette name and an opacity 0..1.
function p.rgba(name, alpha)
    local hex = assert(p.color[name], "no palette color " .. tostring(name))
    return string.format("rgba(%s%02x)", hex, math.floor((alpha or 1) * 255 + 0.5))
end

return p
