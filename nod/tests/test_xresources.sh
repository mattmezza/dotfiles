#!/bin/bash
# Test nod Xresources auto-reload in an isolated Xephyr + D-Bus session.
# Usage: tests/test_xresources.sh [path-to-nod]
# Run from the repo root. If no path is given, uses ../nod if it exists,
# otherwise falls back to 'nod' in $PATH.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ -n "$1" ]; then
    NOD_BIN="$1"
elif [ -x "$REPO_ROOT/nod" ]; then
    NOD_BIN="$REPO_ROOT/nod"
else
    NOD_BIN="nod"
fi

for cmd in "$NOD_BIN" Xephyr dbus-run-session notify-send; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "Missing command: $cmd" >&2
        exit 1
    fi
done

# pick a free display number without touching any running nod/node processes
DISP=
for d in $(seq 99 110); do
    if [ ! -e "/tmp/.X$d-lock" ]; then
        DISP=":$d"
        break
    fi
done
if [ -z "$DISP" ]; then
    echo "No free X display between :99 and :110" >&2
    exit 1
fi

echo "Using nod binary: $NOD_BIN"
echo "Starting Xephyr on $DISP (look for the nested window)"
Xephyr "$DISP" -screen 800x600 -br -noreset &
XEPHYR_PID=$!
sleep 2

cleanup() {
    echo "Cleaning up..."
    kill "$XEPHYR_PID" 2>/dev/null || true
    wait "$XEPHYR_PID" 2>/dev/null || true
}
trap cleanup EXIT

# Everything inside dbus-run-session gets its own session bus, so it cannot
# conflict with the host notification daemon.
dbus-run-session -- bash -c '
    export DISPLAY="'"$DISP"'"
    NOD_BIN="'"$NOD_BIN"'"

    # RED theme
    xrdb -merge <<EOF
nod.background.normal:   #ff0000
nod.foreground.normal:   #ffffff
nod.border.normal:       #000000
nod.background.critical: #ff0000
nod.foreground.critical: #ffffff
nod.border.critical:     #000000
nod.background.low:      #ff0000
nod.foreground.low:      #ffffff
nod.border.low:          #000000
nod.font:                monospace:size=14
EOF

    "$NOD_BIN" &
    NOD_PID=$!
    sleep 1

    echo ""
    echo "==> RED notification should appear in the Xephyr window"
    notify-send -t 0 "Before reload" "This should be RED"

    read -p "Press Enter to switch to GREEN theme..."

    # GREEN theme
    xrdb -merge <<EOF
nod.background.normal:   #00ff00
nod.foreground.normal:   #000000
nod.border.normal:       #000000
nod.background.critical: #00ff00
nod.foreground.critical: #000000
nod.border.critical:     #000000
nod.background.low:      #00ff00
nod.foreground.low:      #000000
nod.border.low:          #000000
EOF

    # force reload; without this, nod will pick it up on the next poll
    kill -USR1 "$NOD_PID"
    sleep 1

    echo ""
    echo "==> GREEN notification should appear (old one should also turn green)"
    notify-send -t 0 "After reload" "This should be GREEN"

    read -p "Press Enter to quit test..."
    kill "$NOD_PID" 2>/dev/null || true
    wait "$NOD_PID" 2>/dev/null || true
'
