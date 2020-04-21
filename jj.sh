jj() {
    DB=${JJDB:-"$HOME/jj.txt"}
    if [ $# -eq 0 ]
    then
        cd $HOME
        return
    elif [ $# -eq 1 ]
    then
        cd $(cat $DB | grep -E "^$1" | sed -E 's/^.*\:(.*)$/\1/')
        return
    else
        case $1 in
            -a)
                if [ $# -eq 3]
                then
                    echo "$2:$3" >> $DB
                    return
                fi
                ;;
            *)
                ;;
        esac
        return
    fi

    cat <<- EOF
Usage:
    $ $0 dev
    $ $0 -a name path

DB is at $DB.
EOF
}
