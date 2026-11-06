#!/bin/sh
# Points ~/.config/hypr at this project's config/. Refuses to overwrite a real dir.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
target="$HOME/.config/hypr"
mkdir -p "$(dirname "$target")"
if [ -L "$target" ]; then
    ln -sfn "$here/config" "$target"
elif [ -e "$target" ]; then
    echo "$target exists and is not a symlink; move it away first" >&2
    exit 1
else
    ln -s "$here/config" "$target"
fi
echo "$target -> $(readlink "$target")"
