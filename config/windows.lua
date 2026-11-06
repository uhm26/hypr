-- Window rules. https://wiki.hypr.land/configuring/core/rules/

-- Ignore apps asking to maximize themselves.
hl.window_rule({
    name  = "suppress-maximize-events",
    match = { class = ".*" },
    suppress_event = "maximize",
})

-- Fix drag-and-drop glitches in X11 (XWayland) apps.
hl.window_rule({
    name  = "fix-xwayland-drags",
    match = { class = "^$", title = "^$", xwayland = true, float = true, fullscreen = false, pin = false },
    no_focus = true,
})

-- Run dialog (SUPER+R): floating, bottom-left.
hl.window_rule({
    name  = "move-hyprland-run",
    match = { class = "hyprland-run" },
    move  = "20 monitor_h-120",
    float = true,
})

-- My bar (Quickshell, not published): glass blur behind it (glass mode), slides in from its edge.
hl.layer_rule({
    name      = "bar-glass",
    match     = { namespace = "^bar$" },
    blur      = true,
    ignore_alpha = 0.2, -- blur only the bar, not its clear area (glass tint must stay > 0.2)
    animation = "slide",
})

-- Notification toasts (bar): glass blur, but a higher threshold than the bar so their soft
-- shadow (≤ 45 % black) isn't frosted; the glass card itself is 35 % and still blurs.
hl.layer_rule({
    name      = "bar-toast-glass",
    match     = { namespace = "^bar-toast$" },
    blur      = true,
    ignore_alpha = 0.3,
})

-- Launcher (bar shell, grows from the bottom edge itself): same glass blur, no slide.
hl.layer_rule({
    name      = "launcher-glass",
    match     = { namespace = "^launcher$" },
    blur      = true,
    ignore_alpha = 0.2,
})

-- screenshot's freeze overlay (ferrite screenshot, github.com/uhm26/ferrite): must appear at once, no slide or fade.
hl.layer_rule({
    name    = "screenshot-freeze",
    match   = { namespace = "^screenshot$" },
    no_anim = true,
})

-- File manager (ferrite files, SUPER+E): floating and centered, a comfortable size.
hl.window_rule({
    name   = "files-float",
    match  = { class = "^files$" },
    float  = true,
    size   = "1100 700",
    center = true,
})

-- Image viewer (ferrite image, github.com/uhm26/ferrite): floating and centered; the viewer sizes itself to the
-- picture's shape, and dragging the border keeps that shape (no bars around the image).
hl.window_rule({
    name              = "image-float",
    match             = { class = "^image$" },
    float             = true,
    center            = true,
    keep_aspect_ratio = true,
})
