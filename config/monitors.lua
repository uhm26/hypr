-- Monitors. DP-3 = main screen, DP-2 = the 75 Hz one, placed to its right.
-- Exact modes on purpose: "highrr" picked 1024x768@75 on DP-2 (stretched).
-- List modes: `hyprctl monitors all` (inside Hyprland).

hl.monitor({ output = "DP-3", mode = "1920x1080@165",   position = "0x0",    scale = 1 })
hl.monitor({ output = "DP-2", mode = "1920x1080@74.97", position = "1920x0", scale = 1 })

-- Pin the first workspaces: 1 on the main screen, 2 on the right one.
hl.workspace_rule({ workspace = "1", monitor = "DP-3", default = true })
hl.workspace_rule({ workspace = "2", monitor = "DP-2", default = true })

-- Anything else plugged in later.
hl.monitor({ output = "", mode = "preferred", position = "auto", scale = "auto" })
