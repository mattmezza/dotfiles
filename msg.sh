# install imessage cli if needed
if [ -f '/usr/local/imessage' ]; then
    ;
else
    sudo curl -o /usr/local/imessage https://gist.githubusercontent.com/aktau/8958054/raw/95f62b2e04cf6e21a0724f1cae6bb508f71b46b3/imessage && sudo chmod a+x /usr/local/imessage;
fi

function msg() {
    RCPT=""
    CONTACTS_FILE=${MSG_CONTACTS_FILE_DEFAULT:-$HOME/contacts.txt}
    echo $CONTACTS_FILE
    if [ -f "$MSG_CONTACTS_FILE" ]; then
        RCPT=$(cat "$MSG_CONTACTS_FILE" | grep " ($2)" | cut -f 1 -d ' ' |  xargs -n1 -I{})
    else
        RCPT="$2"
    fi
    case "$1" in
        send|s)
            /usr/local/imessage "$RCPT" "$3"
            ;;
        who|w)
            echo "$RCPT"
            ;;
        help|h)
            echo "Usage:\n"
            echo "$ msg send|s matt 'Text'"
            echo "$ msg send|s mattmezza@gmail.com 'Text'"
            echo "$ msg who|w mattmezza@gmail.com"
            echo "$ msg who|w matt"
            echo "$ msg help|h"
            ;;
        *)
            ;;
    esac
}
