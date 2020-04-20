sp() {
    if [ $# -lt 2 ]
    then
        cat <<- EOF
Usage:
    $ ide split split ... split pane
Each split should be a three char string with:
    0th    : 'v' or 'h' (vertical or horizontal)
    1st-2nd: percentage of the split (e.g. 20, 50, etc...)
'pane' is the number of the pane to be selected at the end of the split.

Examples:
    $ ide v20 h50 0
    $ ide v25 h33 h33 0
EOF

        return 1
    fi
    PANESELECT=${@:$#}

    for sp in ${@:1:${#}-1}
    do
        eval "tmux split-window -${sp:0:1}p ${sp:1:3}"
    done
    eval "tmux select-pane -t$PANESELECT"
}
