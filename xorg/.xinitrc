#!/bin/env sh
export PATH="/home/matteo/dotfiles/bin:$PATH"

WM="dwm"

if [ "$WM" = "i3" ]; then
   exec i3
elif [ "$WM" = "dwm" ]; then
    exec xcape -e 'Caps_Lock=Escape' &
    exec setxkbmap -layout us -variant altgr-intl -option "caps:ctrl_modifier,compose:rctrl" &
    # exec wallpaper-set "#111111" "#b43cbd" ~/.config/i3/wallpaper.txt /tmp/wall.png &
    exec wallpaper-set "#eeeeee" black ~/.config/i3/wallpaper.txt /tmp/wall.png &
    # make the mouse buttons back-fwd work
    exec xbindkeys -f /home/matteo/.xbindkeysrc &
    exec nm-applet &
    exec slstatus &
    exec picom &
    exec dunst &
    xset s 300 # Set XScreenSaver idle timeout to 5 minutes (300 seconds)
    xset dpms 420 600 900 # screen: blanked +7m, suspended +10m, off +15m
    xss-lock -l slock &
    exec dwm
else
    echo "Unsupported window manager: $WM"
    exit 1
fi
