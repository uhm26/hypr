-- Programs started once when Hyprland starts.

hl.on("hyprland.start", function()
    -- My bar (Quickshell, not published). First, so it hosts the tray and notifications
    -- before other apps start and look for them.
    hl.exec_cmd("qs -p ~/projects/bar/shell")
    -- Own wallpaper daemon (ferrite wallpaper, github.com/uhm26/ferrite): bare `wallpaper` shows the last wallpaper.
    -- Change it any time with `wallpaper <image>`.
    hl.exec_cmd("wallpaper")
    -- Own clipboard keeper (ferrite clipboard, github.com/uhm26/ferrite): copied text survives its app closing.
    hl.exec_cmd("clipboard")
    -- Own polkit agent (ferrite auth, github.com/uhm26/ferrite): the password popup for mounts and pkexec.
    hl.exec_cmd("auth")
end)
