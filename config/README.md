# config

My Hyprland config, in Hyprland's Lua format. `hyprland.lua` only loads the other files;
each one holds one topic.

| File | What's in it |
|---|---|
| `hyprland.lua` | Entry point: `require`s the files below |
| `monitors.lua` | Two 1080p screens side by side (165 Hz left, 75 Hz right), exact modes |
| `env.lua` | Cursor theme and size, Flatpak app folders |
| `look.lua` | Gaps, 1 px borders, rounding, glass blur, shadows, animations |
| `palette.lua` | Every color, border size and spring time, in one place |
| `input.lua` | US layout, focus follows mouse, flat mouse acceleration |
| `binds.lua` | Keybinds (below) |
| `windows.lua` | Window and layer rules: floating apps, glass for the bar and launcher |
| `veil.lua` | Loads the [veil](../veil) plugin and hides password and mail windows from screen shares |
| `autostart.lua` | Starts the bar, wallpaper, clipboard keeper and polkit agent |
| `xdph.conf` | Screen sharing at up to 165 fps (the portal's default caps it at 120) |

## Look and motion

- Monochrome: black, grays and white, plus one muted accent. Borders are 1 px white
  hairlines (30 % on the active window, 8 % on the others).
- Glass: strong blur, slightly darkened, no grain. It shows on see-through surfaces only.
- Windows and workspaces move on springs that settle in 0.22 s and never overshoot; menus
  and popups in 0.15 s. `look.lua` turns "settle time + bounce" into Hyprland's spring
  stiffness and damping.

## Keybinds

| Keys | Action |
|---|---|
| `Super+Q` | Terminal |
| `Super+R` | Launcher |
| `Super+E` | File manager |
| `Super+C` | Close window |
| `Super+V` / `Super+P` / `Super+J` | Float / pseudo-tile / flip split |
| `Super+F` / `F11` | Maximize (bar stays) / real fullscreen |
| `Super+arrows` / `Super+Shift+arrows` | Move focus / move window |
| `Super+1…0` / `Super+Shift+1…0` | Go to workspace / send window there |
| `Super+S` / `Super+Shift+S` | Scratchpad / send window to it |
| `PgDn` | Screenshot: drag a region or click a window |
| `Super+Shift+M` | Quit Hyprland |

## Make it yours

- `monitors.lua` names my outputs (`DP-3`, `DP-2`). Put yours in: `hyprctl monitors all`
  lists them. Any other screen gets its preferred mode.
- `autostart.lua` starts the bar (github.com/uhm26/bar) and ferrite tools. Remove what
  you don't have; `binds.lua` uses `term` and `files` from ferrite too.
- `veil.lua` loads the plugin from `../veil/build/veil.so`: build it first, or delete that
  file and its `require` line in `hyprland.lua`.
- After any edit, `./check.sh` from the repo root checks the whole config.
