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
