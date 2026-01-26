#!/bin/env sh
export PATH="/home/matteo/dotfiles/bin:$PATH"

# if $WM is not set, default to dwm
if [ -z "$WM" ]; then
    WM="dwm"
fi

if [ "$WM" = "i3" ]; then
   exec i3
elif [ "$WM" = "dwm" ]; then
    xcape -e 'Caps_Lock=Escape' &
    setxkbmap -layout us -variant altgr-intl -option "caps:ctrl_modifier,compose:rctrl" &
    nm-applet &
    slstatus &
    picom &
    dunst &
    sxhkd &
    xset s 300 # Set XScreenSaver idle timeout to 5 minutes (300 seconds)
    xset dpms 420 600 900 # screen: blanked +7m, suspended +10m, off +15m
    xss-lock -l slock &
    x-on-resize -sc 'autorandr --change' & # automatically change display config on connect/disconnect
    # delay to allow dwm to start properly before setting the wallpaper
    (sleep 1 && exec wallpaper-set-img) &
    # wallpaper-set "#eeeeee" black ~/.config/i3/wallpaper.txt /tmp/wall.png &
    exec dbus-run-session dwm
else
    echo "Unsupported window manager: $WM"
    exit 1
fi
