#compdef mwmc
# zsh completion for mwmc - the mwm control client
# install (any of):
#   mwmc completions zsh > ~/.zsh/completions/_mwmc   # dir must be in $fpath, then compinit
#   mwmc completions zsh > "${fpath[1]}/_mwmc"
#   copy this file to a dir in $fpath named `_mwmc` (e.g. /usr/share/zsh/site-functions/_mwmc)

_mwmc() {
    local curcontext="$curcontext" state line
    typeset -A opt_args

    _arguments -C \
        '1: :->verb' \
        '*:: :->args' && return

    case $state in
    verb)
        local -a verbs
        verbs=(
            'focus:focus a window, tag, or monitor'
            'move:move a window or tag'
            'swap:reorder the focused window in the stack'
            'layout:set or cycle the layout'
            'master:adjust master count or ratio'
            'gap:adjust the gap size'
            'bar:toggle/show/hide the bar'
            'traybar:toggle/show/hide the system tray'
            'win:act on the focused window'
            'query:print machine-readable state'
            'reload:re-read Xresources and restyle live'
            'quit:ask mwm to exit'
            'version:print the running mwm version'
            'help:print the command summary'
            'completions:print a shell completion script'
        )
        _describe -t commands 'mwmc command' verbs
        ;;
    args)
        case $words[1] in
        focus)
            case $CURRENT in
            2) _values 'target' next prev last win tag mon ;;
            3) case $words[2] in
                win)      _values 'window' next prev last ;;
                tag|mon)  _values 'direction' next prev ;;
                esac ;;
            esac ;;
        move)
            case $CURRENT in
            2) _values 'target' win tag ;;
            3) case $words[2] in
                win) _values 'dest' next prev master mon ;;
                tag) _values 'direction' next prev ;;
                esac ;;
            4) [[ $words[2] == win && $words[3] == mon ]] && _values 'direction' next prev ;;
            esac ;;
        swap)    (( CURRENT == 2 )) && _values 'direction' next prev ;;
        layout)  (( CURRENT == 2 )) && _values 'layout' cols stack float next prev ;;
        master)
            case $CURRENT in
            2) _values 'master' ratio +1 -1 ;;
            3) [[ $words[2] == ratio ]] && _values 'ratio' +0.05 -0.05 ;;
            esac ;;
        gap)     (( CURRENT == 2 )) && _values 'gap' +5 -5 ;;
        bar|traybar) (( CURRENT == 2 )) && _values 'action' toggle show hide ;;
        win)
            case $CURRENT in
            2) _values 'action' close tag toggle move resize center ;;
            3) [[ $words[2] == toggle ]] && _values 'state' float full sticky ;;
            esac ;;
        query)   (( CURRENT == 2 )) && _values 'query' tags windows monitors layout state ;;
        completions) (( CURRENT == 2 )) && _values 'shell' bash zsh ;;
        esac ;;
    esac
}

_mwmc "$@"
