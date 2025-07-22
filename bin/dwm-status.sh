#!/bin/env bash

mem() {
    PERCENT=false TOT=false echo $(memory)
}

batt() {
    B0="$(BAT_NUMBER=0 battery)"
    B1="$(BAT_NUMBER=1 battery)"
    if [ -z "$B1" ]; then
        echo -n "$B0"
        return
    fi
    echo -n "${B0} ${B1}"
}

updates() {
    yay -Qu | wc -l
}

BATT=$(batt)
LOAD=$(load_avg)
MEM=$(mem)
UPD=$(updates)

while true; do
    now=$(date +%s)
    every_3s=$((now % 3))
    every_1m=$((now % 60))
    every_1h=$((now % 3600))

    if [ $every_3s -eq 0 ]; then
        LOAD=$(load_avg)
        MEM=$(mem)
    fi

    if [ $every_1m -eq 0 ]; then
        BATT=$(batt)
    fi

    if [ $every_1h -eq 0 ]; then
        UPD=$(updates)
    fi

    # replace echo with 'xsetroot -name' in the following lines
    echo "$BATT l_$LOAD m_$MEM v_$(volume) b_$(brightness) k_$(key-light) u_$UPD $(date '+%FT%H:%M')"
    sleep 1
done
