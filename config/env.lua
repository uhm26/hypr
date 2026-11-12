-- Environment variables for every app Hyprland starts.

-- Unset theme = Hyprland draws its built-in droplet fallback.
hl.env("XCURSOR_THEME", "Adwaita")
hl.env("XCURSOR_SIZE", "24")
hl.env("HYPRCURSOR_SIZE", "24")

-- Flatpak apps (+ icons) for the launcher and every app: tty1 autologin starts Hyprland
-- without /etc/profile.d/flatpak.sh, so XDG_DATA_DIRS was unset (spec default /usr/local/share:/usr/share).
hl.env("XDG_DATA_DIRS", os.getenv("HOME") .. "/.local/share/flatpak/exports/share:/var/lib/flatpak/exports/share:/usr/local/share:/usr/share")

-- Programs installed for this user (ferrite tools) live in ~/.local/bin. Many systems leave
-- it off PATH, and then binds and autostart can't find them; put it first if missing.
local bin = os.getenv("HOME") .. "/.local/bin"
local path = os.getenv("PATH") or "/usr/local/bin:/usr/bin"
if not (":" .. path .. ":"):find(":" .. bin .. ":", 1, true) then
    hl.env("PATH", bin .. ":" .. path)
end
