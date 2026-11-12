-- Monitors: every screen is found by itself and gets its highest resolution, at the
-- highest refresh rate that resolution offers ("highres"), side by side from left to right.
-- To set a screen yourself, do it in local.lua (see the end of hyprland.lua).
-- `hyprctl monitors all` lists your screens and their modes.

hl.monitor({ output = "", mode = "highres", position = "auto", scale = "auto" })
