-- Environment variables for every app Hyprland starts.

-- Unset theme = Hyprland draws its built-in droplet fallback.
hl.env("XCURSOR_THEME", "Adwaita")
hl.env("XCURSOR_SIZE", "24")
hl.env("HYPRCURSOR_SIZE", "24")

-- Flatpak apps (+ icons) for the launcher and every app: tty1 autologin starts Hyprland
-- without /etc/profile.d/flatpak.sh, so XDG_DATA_DIRS was unset (spec default /usr/local/share:/usr/share).
hl.env("XDG_DATA_DIRS", os.getenv("HOME") .. "/.local/share/flatpak/exports/share:/var/lib/flatpak/exports/share:/usr/local/share:/usr/share")
