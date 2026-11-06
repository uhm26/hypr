<div align="center">

<img src="assets/banner.svg" alt="hypr-plugins: my Hyprland plugins, written in C++" width="100%">

<br>

[![Arch Linux](https://img.shields.io/badge/platform-Arch%20Linux-242424?style=flat-square&labelColor=111111)](#requirements)
[![Hyprland](https://img.shields.io/badge/Hyprland-0.57-242424?style=flat-square&labelColor=111111)](#requirements)
[![C++](https://img.shields.io/badge/written%20in-C%2B%2B-242424?style=flat-square&labelColor=111111)](#build)
[![License](https://img.shields.io/badge/license-MIT%20%2F%20Apache--2.0-242424?style=flat-square&labelColor=111111)](#license)

Plugins for Hyprland, made for my own setup. I built them with Claude: I decide what each
plugin does and how it behaves, Claude writes most of the code, and I use them every day.

</div>

## Plugins

| Plugin | What it does |
|---|---|
| [`veil`](veil) | Hides chosen windows and layers from screen shares: black, blur or pixelated. Your own screen stays normal. |

## Requirements

- Arch Linux (or an Arch-based distro), x86_64.
- Hyprland 0.57. Plugins reach into Hyprland's internals, so a new Hyprland release can
  break them until they are updated here.

## Build

Arch's `hyprland` package ships the headers plugins build against. You also need a compiler
and `pkgconf`:

```sh
sudo pacman -S --needed base-devel hyprland
git clone https://github.com/uhm26/hypr-plugins
cd hypr-plugins
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

How to load each plugin and set it up is in its own README.

## License

MIT or Apache-2.0, at your option: [LICENSE-MIT](LICENSE-MIT), [LICENSE-APACHE](LICENSE-APACHE).
