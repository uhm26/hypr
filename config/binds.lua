-- Key and mouse bindings. SUPER = Windows key.

local mod      = "SUPER"
local terminal = "term"  -- own terminal (ferrite term, github.com/uhm26/ferrite); kitty stays installed

-- Apps and session
hl.bind(mod .. " + Q", hl.dsp.exec_cmd(terminal))
hl.bind(mod .. " + R", hl.dsp.global("quickshell:launcher")) -- launcher in my bar (Quickshell, not published)
hl.bind(mod .. " + E", hl.dsp.exec_cmd("files")) -- own file manager (ferrite files, github.com/uhm26/ferrite)
hl.bind(mod .. " + C", hl.dsp.window.close())
hl.bind(mod .. " + SHIFT + M", hl.dsp.exit()) -- quits Hyprland; SHIFT so it is hard to hit by mistake

-- Window layout
hl.bind(mod .. " + V", hl.dsp.window.float({ action = "toggle" }))
hl.bind(mod .. " + P", hl.dsp.window.pseudo())
hl.bind(mod .. " + J", hl.dsp.layout("togglesplit"))
hl.bind(mod .. " + F", hl.dsp.window.fullscreen({ mode = "maximized" })) -- fill the screen, bar stays
hl.bind("F11",         hl.dsp.window.fullscreen({ mode = "fullscreen" })) -- real fullscreen, covers everything

-- Focus with arrow keys
hl.bind(mod .. " + left",  hl.dsp.focus({ direction = "left" }))
hl.bind(mod .. " + right", hl.dsp.focus({ direction = "right" }))
hl.bind(mod .. " + up",    hl.dsp.focus({ direction = "up" }))
hl.bind(mod .. " + down",  hl.dsp.focus({ direction = "down" }))

-- Move the window with SHIFT + arrow keys
hl.bind(mod .. " + SHIFT + left",  hl.dsp.window.move({ direction = "left" }))
hl.bind(mod .. " + SHIFT + right", hl.dsp.window.move({ direction = "right" }))
hl.bind(mod .. " + SHIFT + up",    hl.dsp.window.move({ direction = "up" }))
hl.bind(mod .. " + SHIFT + down",  hl.dsp.window.move({ direction = "down" }))

-- Workspaces: SUPER+[0-9] go there, SUPER+SHIFT+[0-9] move the window there
for i = 1, 10 do
    local key = i % 10 -- workspace 10 is key 0
    hl.bind(mod .. " + " .. key,         hl.dsp.focus({ workspace = i }))
    hl.bind(mod .. " + SHIFT + " .. key, hl.dsp.window.move({ workspace = i }))
end

-- Scratchpad
hl.bind(mod .. " + S",         hl.dsp.workspace.toggle_special("magic"))
hl.bind(mod .. " + SHIFT + S", hl.dsp.window.move({ workspace = "special:magic" }))

-- Mouse: scroll through workspaces, drag to move (left) / resize (right)
hl.bind(mod .. " + mouse_down", hl.dsp.focus({ workspace = "e+1" }))
hl.bind(mod .. " + mouse_up",   hl.dsp.focus({ workspace = "e-1" }))
hl.bind(mod .. " + mouse:272",  hl.dsp.window.drag(),   { mouse = true })
hl.bind(mod .. " + mouse:273",  hl.dsp.window.resize(), { mouse = true })

-- Volume keys (keyboard knob / media keys), via pactl
local vol = { locked = true, repeating = true }
hl.bind("XF86AudioRaiseVolume", hl.dsp.exec_cmd("pactl set-sink-volume @DEFAULT_SINK@ +5%"),  vol)
hl.bind("XF86AudioLowerVolume", hl.dsp.exec_cmd("pactl set-sink-volume @DEFAULT_SINK@ -5%"),  vol)
hl.bind("XF86AudioMute",        hl.dsp.exec_cmd("pactl set-sink-mute @DEFAULT_SINK@ toggle"), { locked = true })
hl.bind("KP_Insert",           hl.dsp.exec_cmd("audio set mic.mute toggle"), { locked = true }) -- numpad 0 without NumLock: mute the mic (ferrite audio, github.com/uhm26/ferrite)

-- Screenshot (ferrite screenshot, github.com/uhm26/ferrite): PgDn freezes the screen; drag a region or click a window.
-- The PNG goes to ~/Pictures and the clipboard. Note: PgDn no longer reaches apps.
hl.bind("Next", hl.dsp.exec_cmd("screenshot -s"))
