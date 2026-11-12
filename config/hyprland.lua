-- Hyprland entry point. Each require() loads ~/.config/hypr/<name>.lua.
-- Wiki: https://wiki.hypr.land/configuring/
-- Check after editing: ./check.sh (from the project folder)

require("monitors")
require("env")
require("look")
require("input")
require("binds")
require("windows")
require("veil")
require("autostart")

-- The bar (github.com/uhm26/bar): its installer writes bar.lua next to this file. Not
-- installed: skipped.
pcall(require, "bar")

-- Your own changes for this PC (screens, extra binds, ...): put them in local.lua next to
-- this file. Git ignores it, so updates never touch it, and it loads last, so it wins.
-- package.searchpath looks exactly where require does.
if package.searchpath("local", package.path) then
    require("local")
end
