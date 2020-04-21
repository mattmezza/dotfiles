jj() {
    DB=${JJDB:-"$HOME/.jj"}
    if [ $# -eq 0 ]
    then
        cd $HOME
        return
    elif [ $# -eq 1 ]
    then
        if [[ $1 == -* ]]
        then
            # skip and goto help
        else
            cd $(cat $DB | grep -Em1 "^$1" | sed -E 's/^.*\:(.*)$/\1/')
            return
        fi
    else
        case "$1" in
            "-a")
                if [ $# -eq 3 ]
                then
                    echo "$2:$3" >> $DB
                    return
                fi
                ;;
            "-d")
                if [ $# -eq 2 ]
                then
                    sed -Ei '' "/^$2\:.*$/d" $DB
                    return
                fi
                ;;
            "-r")
                if [ $# -eq 2 ]
                then
                    cat $DB | grep -Em1 "^$2" | sed -E 's/^.*\:(.*)$/\1/'
                    return
                fi
                ;;
            *)
                ;;
        esac
    fi

    cat <<- EOF
Usage:
    $ $0 name         # to cd directly into 'name'
    $ $0 -r name      # to resolve 'name'
    $ $0 -a name path # to add 'name' as 'path'
    $ $0 -d name      # to delete 'name'

DB is at '$DB'.
EOF
}
