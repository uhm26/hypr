#!/bin/sh
# Whole-desktop installer for Arch Linux. From a fresh minimal Arch (text screen) or a
# running Hyprland to the full desktop: Hyprland with this config, the veil plugin, the
# ferrite apps (github.com/uhm26/ferrite), the bar (github.com/uhm26/bar), a login and a
# wallpaper. Run it as your normal user; it asks for your password to install packages.
# Run it again any time to update everything.
#
#   curl -fsSL https://raw.githubusercontent.com/uhm26/hypr/main/install.sh | sh
#
# HYPR_DIR: where the config goes (default ~/hypr). HYPR_LOGIN=sddm|autologin|none skips
# the login question (for scripts and tests).
#
# Everything runs inside main(), called on the last line: if the download is cut off,
# nothing runs.

set -eu

REPO="https://github.com/uhm26/hypr"
RAW="https://raw.githubusercontent.com/uhm26"
TOOLS="term files image video screenshot wallpaper clipboard auth"
# Hyprland, screen sharing, git + compiler (the veil plugin), cursor and app fonts.
BASE="hyprland xdg-desktop-portal-hyprland git base-devel adwaita-cursors adwaita-fonts"
MARK="# added by the hypr installer"

step() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }
say() { printf '    %s\n' "$*"; }
die() { printf '\n\033[1;31mSTOPPED:\033[0m %s\n' "$*" >&2; exit 1; }
has() { command -v "$1" >/dev/null 2>&1; }
in_hyprland() { [ -n "${HYPRLAND_INSTANCE_SIGNATURE:-}" ]; }
# A terminal to ask questions on (/dev/tty exists even without one, e.g. in CI).
tty_ok() { (: </dev/tty) 2>/dev/null; }

check_system() {
    [ "$(id -u)" != 0 ] || die "Run this as your normal user, not as root. It asks for your password when it needs it."
    grep -qE '^(ID|ID_LIKE)=.*arch' /etc/os-release 2>/dev/null || die "This is for Arch Linux only."
    [ "$(uname -m)" = x86_64 ] || die "This needs a 64-bit PC (x86_64)."
    has sudo || die "sudo is missing. Log in as root and run: pacman -S sudo
    Then let your user use it: EDITOR=nano visudo, remove the # in front of %wheel ALL=(ALL:ALL) ALL"
    say "Type your password when asked (nothing shows while you type, that's normal)."
    sudo -v || die "sudo didn't accept you. Is your user in the wheel group? (as root: usermod -aG wheel $user)"
}

install_base() {
    step "1/7  Installing Hyprland and the basics (a few minutes)"
    pkgs=$BASE
    # Sound: PipeWire, unless PulseAudio is already set up (they can't both run).
    if ! pacman -Q pulseaudio >/dev/null 2>&1; then pkgs="$pkgs pipewire pipewire-pulse wireplumber"; fi
    # shellcheck disable=SC2086 # a list of package names
    sudo pacman -Syu --needed --noconfirm $pkgs libpulse || die "Installing packages failed (see the lines above). Check your internet and run this again."
}

# The config: a git clone in $dir, linked to ~/.config/hypr by its link.sh.
get_config() {
    step "2/7  Getting the Hyprland config"
    if [ -d "$dir/.git" ]; then
        git -C "$dir" pull -q --ff-only || say "Couldn't update $dir (files there were changed by hand): keeping them as they are."
    else
        if [ -e "$dir" ]; then
            mv "$dir" "$dir.backup-$(date +%Y%m%d-%H%M%S)"
            say "Moved an old $dir out of the way (kept as $dir.backup-...)."
        fi
        git clone -q "$REPO" "$dir" || die "Downloading the config failed. Check your internet and run this again."
    fi
    "$dir/link.sh" >"$tmp/link.log" || die "Linking the config failed: $(cat "$tmp/link.log")"
    grep -v "^Log out" "$tmp/link.log" | sed 's/^/    /'
}

build_veil() {
    step "3/7  Building the veil plugin (hides chosen windows from screen shares)"
    if "$dir/veil/build.sh" >"$tmp/veil.log" 2>&1; then
        say "Built."
    else
        tail -n 5 "$tmp/veil.log" | sed 's/^/    /'
        say "veil didn't build. Everything else works without it."
    fi
}

# Run another installer from GitHub ($1 = its URL, then its arguments). Its output goes to
# a log; the packages it asks for are installed here, so nothing is left to copy by hand.
run_installer() {
    url=$1
    shift
    curl --proto '=https' --tlsv1.2 -fsSL "$url" -o "$tmp/sub.sh" || die "Download failed: $url"
    if ! sh "$tmp/sub.sh" "$@" >"$tmp/sub.log" 2>&1 </dev/null; then
        tail -n 15 "$tmp/sub.log" | sed 's/^/    /'
        die "That step failed (see the lines above). Run this again; if it fails again, send these lines to whoever gave you this."
    fi
    # Its warnings (another notification program running, an old Hyprland, ...) still show.
    grep "note:" "$tmp/sub.log" | sed 's/^[a-z]*: /    /' || true
    need=$(grep -o 'sudo pacman -S --needed.*' "$tmp/sub.log" | sed 's/^sudo pacman -S --needed//' | tr '\n' ' ')
    if [ -n "$(echo "$need" | tr -d ' ')" ]; then
        say "Installing what it needs:$need"
        # shellcheck disable=SC2086 # a list of package names
        sudo pacman -S --needed --noconfirm $need || die "Installing packages failed (see the lines above)."
    fi
}

install_apps() {
    step "4/7  Installing the apps (terminal, files, image and video viewer, screenshots, ...)"
    # shellcheck disable=SC2086 # one argument per tool
    run_installer "$RAW/ferrite/main/install.sh" $TOOLS
    # The password popup (auth) talks to this polkit socket; it starts by itself from the
    # next boot, start it now too.
    sudo systemctl start polkit-agent-helper.socket 2>/dev/null || true
    say "Installed: $TOOLS"
    # Open folders, pictures and videos with these apps.
    apps="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
    if has xdg-mime; then
        xdg-mime default files.desktop inode/directory
        for t in image video; do
            types=$(sed -n 's/^MimeType=//p' "$apps/$t.desktop" 2>/dev/null | tr ';' ' ')
            # shellcheck disable=SC2086 # a list of file types
            if [ -n "$types" ]; then xdg-mime default "$t.desktop" $types; fi
        done
        say "Folders open in files, pictures in image, videos in video."
    fi
}

install_bar() {
    step "5/7  Installing the bar (top bar, app launcher, notifications)"
    run_installer "$RAW/bar/main/install.sh"
    say "Installed. It starts with Hyprland."
}

# A login screen or auto-login is set up already.
login_exists() {
    [ -e /etc/systemd/system/display-manager.service ] \
        || grep -qs -- '--autologin' /etc/systemd/system/getty@tty1.service.d/*.conf
}

# The login: kept if there is one (a login screen, auto-login, or Hyprland running now).
setup_login() {
    step "6/7  Login"
    if in_hyprland || login_exists; then
        login=kept
        say "You already have a way to log in: kept it."
        return 0
    fi
    login=${HYPR_LOGIN:-}
    if [ -z "$login" ] && tty_ok; then
        say "How do you want to start the desktop after you turn the PC on?"
        say "  1) A login screen: pick Hyprland, type your password (recommended)"
        say "  2) Straight into the desktop, no password"
        say "  3) Nothing: I log in on the text screen and type start-hyprland"
        printf '    Type 1, 2 or 3 and press Enter [1]: ' >/dev/tty
        read -r answer </dev/tty || answer=1
        case "$answer" in 2) login=autologin ;; 3) login=none ;; *) login=sddm ;; esac
    fi
    case "${login:-none}" in
        sddm) setup_sddm ;;
        autologin) setup_autologin ;;
        *) login=none; say "Nothing changed. After logging in on the text screen, type: start-hyprland" ;;
    esac
}

setup_sddm() {
    sudo pacman -S --needed --noconfirm sddm || die "Installing the login screen failed (see above)."
    sudo systemctl enable sddm.service || die "Turning on the login screen failed (see above)."
    say "Login screen on. After a restart: pick \"Hyprland\" (not \"uwsm-managed\") and log in."
}

# Auto-login on the first text screen, then the login shell starts Hyprland there.
setup_autologin() {
    d=/etc/systemd/system/getty@tty1.service.d
    sudo mkdir -p "$d"
    # shellcheck disable=SC2016 # $TERM is for systemd to expand
    printf '[Service]\nExecStart=\nExecStart=-/sbin/agetty -o "-p -f -- \\\\u" --noclear --autologin %s %%I $TERM\n' "$user" \
        | sudo tee "$d/autologin.conf" >/dev/null
    # Auto-login can be faster than the graphics driver: wait up to 10 s for the GPU first.
    # shellcheck disable=SC2016 # runs later, in the login shell
    start='i=0; while [ $i -lt 100 ] && ! ls /dev/dri/by-path/*-card >/dev/null 2>&1; do sleep 0.1; i=$((i+1)); done; udevadm settle -t 5; start-hyprland'
    case "$(getent passwd "$user" | cut -d: -f7)" in
        */fish)
            f="$HOME/.config/fish/conf.d/hypr-autostart.fish"
            mkdir -p "${f%/*}"
            printf '%s\nif status is-login; and test (tty) = /dev/tty1; and not set -q WAYLAND_DISPLAY\n    sh -c %s\nend\n' \
                "$MARK" "'$start'" >"$f" ;;
        */zsh) add_start "$HOME/.zprofile" "$start" ;;
        *) add_start "$HOME/.bash_profile" "$start" ;;
    esac
    say "Auto-login on. After a restart the desktop starts by itself."
}

# Append the tty1 start line to profile $1 once ($2 = the start command).
add_start() {
    grep -qsF "$MARK" "$1" && return 0
    # A new bash profile must still load ~/.bashrc, as Arch's default one does.
    if [ ! -e "$1" ] && [ "${1##*/}" = .bash_profile ]; then echo '[ -f ~/.bashrc ] && . ~/.bashrc' >"$1"; fi
    # shellcheck disable=SC2016 # written into the profile as it is
    printf '\n%s\nif [ -z "${WAYLAND_DISPLAY:-}" ] && [ "$(tty)" = /dev/tty1 ]; then sh -c %s; fi\n' \
        "$MARK" "'$2'" >>"$1"
}

# The wallpaper in assets/ (when this version has one), unless one is already set.
set_wallpaper() {
    step "7/7  Wallpaper"
    state="${XDG_STATE_HOME:-$HOME/.local/state}/wallpaper/last"
    img=$(find "$dir/assets" -maxdepth 1 -name 'wallpaper.*' 2>/dev/null | head -n 1)
    if [ -e "$state" ]; then say "You already picked one: kept it."; return 0; fi
    if [ -z "$img" ]; then say "No wallpaper in this version yet: skipped."; return 0; fi
    pics="$HOME/Pictures/Wallpapers"
    mkdir -p "$pics" "${state%/*}"
    cp "$img" "$pics/"
    printf '%s' "$pics/${img##*/}" >"$state"
    # A running wallpaper daemon switches now; otherwise it shows at the next login.
    if pgrep -u "$user" -x wallpaper >/dev/null 2>&1; then "$HOME/.local/bin/wallpaper" "$pics/${img##*/}" >/dev/null 2>&1 || true; fi
    say "Set to $pics/${img##*/} (change it any time: wallpaper /path/to/image)."
}

finish() {
    step "Done!"
    if in_hyprland; then
        say "Log out (Super+Shift+M) and log in again to start the new desktop."
    else
        case "$login" in
            none) say "Type start-hyprland to start the desktop." ;;
            *) say "Restart the PC to start the desktop: sudo reboot" ;;
        esac
    fi
    say ""
    say "Keys:  Super+Q terminal   Super+R apps   Super+E files   Super+C close window"
    say "       PageDown screenshot   Super+1..0 workspaces   Super+Shift+M log out"
    say "Update everything later: run the same command again."
}

main() {
    dir=${HYPR_DIR:-$HOME/hypr}
    user=$(id -un)
    case "$0" in */install.sh) dir=$(cd "$(dirname "$0")" && pwd) ;; esac # run from a clone
    login=""
    check_system
    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"' EXIT INT TERM
    install_base
    get_config
    build_veil
    install_apps
    install_bar
    setup_login
    set_wallpaper
    finish
}

main "$@"
