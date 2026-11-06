<div align="center">

<img src="assets/banner.svg" alt="hypr: my Hyprland config and plugins" width="100%">

<br>

[![Arch Linux](https://img.shields.io/badge/platform-Arch%20Linux-242424?style=flat-square&labelColor=111111)](#requirements)
[![Hyprland](https://img.shields.io/badge/Hyprland-0.57-242424?style=flat-square&labelColor=111111)](#requirements)
[![Lua + C++](https://img.shields.io/badge/written%20in-Lua%20%2B%20C%2B%2B-242424?style=flat-square&labelColor=111111)](#whats-here)
[![License](https://img.shields.io/badge/license-MIT%20%2F%20Apache--2.0-242424?style=flat-square&labelColor=111111)](#license)

My Hyprland setup: the config I use every day and the plugins I wrote for it. I built them
with Claude: I decide what everything does and how it looks, Claude writes most of the code.

</div>

## What's here

| Folder | What it is |
|---|---|
| [`config`](config) | My Hyprland config (Lua): monochrome glass look, springy animations without overshoot, keybinds, rules |
| [`veil`](veil) | Plugin: hides chosen windows and layers from screen shares: black, blur or pixelated. Your own screen stays normal. |

The apps the config starts (terminal, file manager, screenshots, wallpaper, clipboard) are
my [ferrite](https://github.com/uhm26/ferrite) tools.

## Requirements

- Arch Linux (or an Arch-based distro), x86_64.
- Hyprland 0.57 with its Lua config. Plugins reach into Hyprland's internals, so a new
  Hyprland release can break them until they are updated here.

## Use the config

```sh
git clone https://github.com/uhm26/hypr
cd hypr
./check.sh     # checks the config without starting Hyprland
./link.sh      # points ~/.config/hypr at config/ (refuses to touch a real folder)
```

It is made for my PC: [config/README.md](config/README.md) says what to change first.

## Build the plugins

Arch's `hyprland` package ships the headers plugins build against. You also need a compiler
and `pkgconf`:

```sh
sudo pacman -S --needed base-devel hyprland
./veil/build.sh        # -> veil/build/veil.so
```

**Rebuild after every Hyprland update.** A plugin built for another Hyprland version
refuses to load and shows a notification saying so.

Hyprland keeps a plugin it has loaded in memory under its file path. After a rebuild,
restart Hyprland, or unload it and load a copy under a new name:

```sh
hyprctl plugin unload "$PWD/veil/build/veil.so"
cp veil/build/veil.so veil/build/veil-new.so
hyprctl plugin load "$PWD/veil/build/veil-new.so"
```

How to set up each plugin is in its own README.

## License

MIT or Apache-2.0, at your option: [LICENSE-MIT](LICENSE-MIT), [LICENSE-APACHE](LICENSE-APACHE).
