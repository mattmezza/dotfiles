# nod

Suckless-style notification daemon for X11. Implements the
[Desktop Notifications Specification 1.2][spec] over D-Bus.

Think "herbe with native D-Bus support" — no wrapper scripts, works out
of the box with `notify-send` and any app that emits standard
notifications.

[spec]: https://specifications.freedesktop.org/notification-spec/notification-spec-latest.html

## Features

- Single `.c` source file, single `config.h`, no runtime config
- Native D-Bus — no `herbe` wrapper, no FIFOs
- Multi-monitor aware (follows focused window via XRandR)
- Stacked notifications, configurable corner
- Per-urgency colors and timeouts
- Click to dismiss / invoke `"default"` action
- `~800` lines of C99

## Requirements

- libX11, libXft, libXrandr
- libdbus-1
- a running X session

On Arch: `pacman -S libx11 libxft libxrandr dbus`

## Build

```sh
make
sudo make install
```

Edit `config.h` then `make clean && make` to reconfigure.

## Run

```sh
# Start from ~/.xinitrc or equivalent, before your WM:
nod &
exec dwm
```

Only one notification daemon can own the name
`org.freedesktop.Notifications` at a time — make sure `dunst`, `mako`,
or any DE daemon is not running.

## nod-send

A `notify-send`-compatible CLI wrapper shipped alongside nod. Adds
support for the `-A/--action` flag (upstream `notify-send` already has
this, but `libnotify`-less systems may not). Installed to
`$(PREFIX)/bin/nod-send`. Depends on `gdbus` (from `glib2`).

```sh
# basic — identical to notify-send
nod-send "Hello" "This is a test"
nod-send -u critical -t 3000 "Critical" "Server down"

# with a default action — blocks until dismissed, prints "default" if clicked
key=$(nod-send -A "Open" "New mail" "Click to open inbox")
[ "$key" = "default" ] && xdg-open https://mail.example.com

# named actions
nod-send -A open="Open" -A snooze="Snooze" "Reminder" "Meeting in 5min"
```

The wait is bounded: `expire-time + 3s` slack for finite timeouts,
unbounded for persistent (`-t 0`).

## Test

```sh
notify-send "Hello" "This is a test"
notify-send -u low "Low" "Low urgency"
notify-send -u critical "Critical" "Critical urgency"
notify-send -t 3000 "Quick" "3 second timeout"

# Replace an existing notification
notify-send -r 1 "Updated" "Replaced notification"

# Notification with a default action
gdbus call --session \
    --dest org.freedesktop.Notifications \
    --object-path /org/freedesktop/Notifications \
    --method org.freedesktop.Notifications.Notify \
    "test" 0 "" "Click me" "Has default action" \
    '["default", "Open"]' '{}' -1
```

## Interaction

- **Left-click**  a notification — invoke `default` action (if any), then dismiss
- **Right-click** a notification — dismiss only, never invoke action
- **Timeout**     — auto-dismiss

## What nod does NOT do

Icons. Markup. Sound. History. Persistence (timeout 0 = default, not
infinite). Runtime config. CLI flags beyond `-v`. Focus stealing.

If you want those, use `dunst`.

## Screenshot

_placeholder_

## License

MIT — see `LICENSE`.
