#!/bin/sh
# Validates the config without starting Hyprland. Exit 1 on any error.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
out=$(Hyprland --verify-config -c "$here/config/hyprland.lua" 2>&1 | sed -n '/parsing result/,$p')
echo "$out"
echo "$out" | grep -q '^config ok$'
