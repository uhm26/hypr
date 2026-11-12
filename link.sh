#!/bin/sh
# Points ~/.config/hypr at this project's config/. A folder that is already there (for
# example the default config Hyprland makes on its first start) is moved aside first, with
# the date in its name; nothing is deleted.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
target="$HOME/.config/hypr"
mkdir -p "$(dirname "$target")"
if [ -e "$target" ] && [ ! -L "$target" ]; then
    old="$target.backup-$(date +%Y%m%d-%H%M%S)"
    mv "$target" "$old"
    echo "Your old Hyprland config is now in $old"
    # The bar's installer writes bar.lua there; keep it, or the bar stops starting.
    if [ -f "$old/bar.lua" ] && [ ! -e "$here/config/bar.lua" ]; then
        cp "$old/bar.lua" "$here/config/bar.lua"
        echo "Kept the bar's start file (bar.lua) from it."
    fi
fi
ln -sfn "$here/config" "$target"
echo "Done: Hyprland now uses this config ($target -> $here/config)."
echo "Log out (Super+Shift+M) and log in again to start with it."
