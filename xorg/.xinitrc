#!/bin/env sh
export PATH="/home/matteo/dotfiles/bin:$PATH"

WM="mwm"

nod &
setxkb us altgr-intl
xcape -e 'Control_L=Escape'
nm-applet &
gsettings set org.blueman.general plugin-list "['!ConnectionNotifier']"
blueman-applet &
slstatus &
picom &
clipmenud &
sxhkd &

# 1. Lock screen listener
xss-lock --transfer-sleep-lock -- lok &

# 2. Dynamic Power Management Loop
(
last=""
while true; do
    ac=$(cat /sys/class/power_supply/AC/online)
    if [[ "$ac" -eq 1 ]]; then
        # AC: 5 min screen off & lock
        xset s 300 300 && xset dpms 300 300 300
    else
        # Battery: 3 min screen off & lock
        xset s 180 180 && xset dpms 180 180 180
    fi

    # Locking/screen-off are handled above. This adds the suspend-then-hibernate layer,
    # restarting xidlehook with a power-appropriate timer only when AC state changes:
    #   AC      -> suspend-then-hibernate after 30 min idle
    #   battery -> suspend-then-hibernate after 15 min idle
    # --not-when-fullscreen / --not-when-audio skip suspending during video/calls.
    if [[ "$ac" != "$last" ]]; then
        pkill -x xidlehook
        if [[ "$ac" -eq 1 ]]; then
            xidlehook --not-when-fullscreen --not-when-audio \
                --timer 600 'systemctl suspend-then-hibernate' '' &
        else
            xidlehook --not-when-fullscreen --not-when-audio \
                --timer 300  'systemctl suspend-then-hibernate' '' &
        fi
        last="$ac"
    fi

    sleep 60
done
) &

x-on-resize -sc 'autorandr --change' & # automatically change display config on connect/disconnect
# wallpaper-set "#eeeeee" black ~/.config/i3/wallpaper.txt /tmp/wall.png &

(
    # Wait a moment for dwm/dbus to initialize
    sleep 1
    dbus-update-activation-environment --systemd DISPLAY XAUTHORITY XDG_CURRENT_DESKTOP=dwm
    systemctl --user import-environment DISPLAY XAUTHORITY XDG_CURRENT_DESKTOP
    systemctl --user restart xdg-desktop-portal xdg-desktop-portal-gtk
) &
#exec dbus-run-session dwm
# set wallpaper + generate wallust colors + merge into xrdb BEFORE the WM
# (and any terminals) start, so st reads correct colors at launch
wallpaper-set-img
exec dbus-run-session mwm
