#!/bin/sh
# Validates the config without starting Hyprland. Exit 1 on any error (then shows all output).
set -u
here=$(cd "$(dirname "$0")" && pwd)
all=$(Hyprland --verify-config -c "$here/config/hyprland.lua" 2>&1)
out=$(echo "$all" | sed -n '/parsing result/,$p')
if echo "$out" | grep -q '^config ok$'; then
    echo "$out"
else
    echo "$all"
    exit 1
fi
