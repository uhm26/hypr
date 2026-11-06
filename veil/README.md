# veil

Hides chosen windows and layers from screen shares. In the shared picture they are covered
with black, a blur or pixels. On your own screen you still see everything as usual.

Hyprland's own `noscreenshare` rule paints black boxes. veil also:

- **blurs or pixelates** instead of painting black, cut to the window's or layer's own shape
  (rounded corners, cards inside a bigger layer);
- keeps covering windows and layers **while they close**: their close animation is drawn from
  a snapshot, which the black boxes miss;
- covers layers that ignore input (click-through notifications) and popups (menus);
- puts popup boxes in the right place on a monitor that is not at position 0,0;
- keeps a window covered for 35 ms after its rule stops matching. Some apps change their
  title a frame or two before they draw the new content (switching browser tabs), so a
  title rule alone would show one or two frames;
- draws the mouse pointer on top of the cover, like Hyprland does.

It works on monitor and region shares.

## Use

Build it first (see the [main README](../README.md#build)), then load it and add rules.
The examples use Hyprland's Lua config:

```lua
hl.plugin.load("/path/to/hypr-plugins/veil/build/veil.so")

-- The plugin adds hl.plugin.veil. Until it has loaded, `veil` would be an unknown
-- rule field, so the rules sit behind this check.
if hl.plugin.veil then
    hl.window_rule({
        name  = "veil-passwords",
        match = { class = "^org.keepassxc.KeePassXC$" },
        veil  = "pixel",
    })
    hl.window_rule({
        name  = "veil-mail",
        match = { title = ".*@example\\.com.*" }, -- any window with that address in its title
        veil  = "blur",
    })
    hl.layer_rule({
        name  = "veil-notifications",
        match = { namespace = "^notifications$" },
        veil  = "pixel",
    })
end
```

| `veil =` | Cover |
|---|---|
| `"pixel"` (or `"pixelate"`) | 16 px blocks, scaled with the monitor |
| `"blur"` | your Hyprland blur settings, at full strength |
| `"black"` | solid black |
| `"none"` (or `"off"`, `""`) | no cover |

Any other value covers in black: a typo never shows the window.

Blur needs `decoration:blur:enabled`. With blur turned off, `"blur"` covers in black.

`hl.plugin.veil.version()` returns the loaded version.

## How it works

veil hooks the function that draws a monitor or region into a share buffer. After Hyprland
has drawn the frame, veil paints its covers on top, then the pointer. Blur and pixels come
from the same frame your monitor shows, through veil's own small GL shader. A second and
third hook (layer and window unmap) keep track of close animations until they end.

## Limits

- Built for Hyprland 0.57 (built and run on 0.57.0 and 0.57.1). It uses Hyprland internals, so a
  new Hyprland release can break it.
- Blur mode blurs the whole monitor once per shared frame. Its GPU cost is not measured yet.
- Window shares (sharing one window) are not covered.
