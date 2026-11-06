#!/bin/sh
# Builds build/veil.so against the installed Hyprland headers. Rebuild after every Hyprland update.
# Hyprland never truly unloads a plugin (dlopen keeps it by path): after a rebuild, restart
# Hyprland to run the new build; `hyprctl plugin load` of the same path reuses the old one.
set -eu
cd "$(dirname "$0")"
mkdir -p build
deps="hyprland pixman-1 libdrm libinput libudev wayland-server xkbcommon lua"
# shellcheck disable=SC2046 # pkgconf output must word-split into flags
g++ -std=c++2b -shared -fPIC -O2 -Wall -Wno-c++11-narrowing \
    $(pkgconf --cflags $deps) \
    src/main.cpp src/veil.cpp src/shade.cpp -o build/veil.so
echo "built $(pwd)/build/veil.so"
