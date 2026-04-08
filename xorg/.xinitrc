#!/bin/env sh
export PATH="/home/matteo/dotfiles/bin:$PATH"

# if $WM is not set, default to dwm
if [ -z "$WM" ]; then
    WM="dwm"
fi

if [ "$WM" = "i3" ]; then
   exec i3
elif [ "$WM" = "dwm" ]; then
    setxkb us altgr-intl
    xcape -e 'Control_L=Escape' &
    nm-applet &
    blueman-applet &
    slstatus &
    picom &
    dunst &
    clipmenud &
    sxhkd &

    # 1. Lock screen listener
    xss-lock --transfer-sleep-lock -- slock &

    # 2. Dynamic Power Management Loop
    (
    while true; do
        if [[ $(cat /sys/class/power_supply/AC/online) -eq 1 ]]; then
            # AC: 5 min screen off & lock
            xset s 300 300 && xset dpms 300 300 300
        else
            # Battery: 3 min screen off & lock
            xset s 180 180 && xset dpms 180 180 180
        fi
        sleep 60
    done
    ) &

    x-on-resize -sc 'autorandr --change' & # automatically change display config on connect/disconnect
    # delay to allow dwm to start properly before setting the wallpaper
    (sleep 1 && exec wallpaper-set-img) &
    # wallpaper-set "#eeeeee" black ~/.config/i3/wallpaper.txt /tmp/wall.png &

    (
        # Wait a moment for dwm/dbus to initialize
        sleep 1
        dbus-update-activation-environment --systemd DISPLAY XAUTHORITY XDG_CURRENT_DESKTOP=dwm
        systemctl --user import-environment DISPLAY XAUTHORITY XDG_CURRENT_DESKTOP
        systemctl --user restart xdg-desktop-portal xdg-desktop-portal-gtk
    ) &
    exec dbus-run-session dwm
else
    echo "Unsupported window manager: $WM"
    exit 1
fi
