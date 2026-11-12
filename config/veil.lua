-- veil plugin (../veil in this repo): hides things from screen shares (black/blur/pixel).
-- Rebuild it after every Hyprland update (veil/build.sh), or it refuses to load.
-- ~/.config/hypr links to this folder (link.sh), so ../veil is the plugin next to it.
-- Not built: skip it quietly (the rest of the config works without it).
local so = os.getenv("HOME") .. "/.config/hypr/../veil/build/veil.so"
local built = io.open(so)
if built then
    built:close()
    hl.plugin.load(so)
end

-- The plugin adds hl.plugin.veil; until it has loaded (and on a Hyprland without it) the
-- `veil` rule would be an unknown field, so it sits behind this check.
if hl.plugin.veil then
    hl.layer_rule({
        name  = "veil-notifications",
        match = { namespace = "^bar-toast$" }, -- my bar's notification toasts
        veil  = "pixel",
    })
    hl.window_rule({
        name  = "veil-passwords",
        match = { class = "^password$" }, -- ferrite's password manager
        veil  = "pixel",
    })
    hl.window_rule({
        name  = "veil-gmail",
        match = { title = ".*@gmail\\.com.*" }, -- any window showing a gmail address in its title
        veil  = "pixel",
    })
end
