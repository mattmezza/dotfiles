# bash completion for mwmc - the mwm control client
# install (any of):
#   mwmc completions bash > ~/.local/share/bash-completion/completions/mwmc
#   echo 'source <(mwmc completions bash)' >> ~/.bashrc
#   drop this file in /usr/share/bash-completion/completions/mwmc

_mwmc()
{
    local cur prev words cword
    if declare -F _init_completion >/dev/null 2>&1; then
        _init_completion || return
    else
        COMPREPLY=()
        cur=${COMP_WORDS[COMP_CWORD]}
        prev=${COMP_WORDS[COMP_CWORD-1]}
        words=("${COMP_WORDS[@]}")
        cword=$COMP_CWORD
    fi

    local verbs="focus move swap layout master gap bar traybar win query reload quit version help kill completions"

    # top-level verb
    if [ "$cword" -eq 1 ]; then
        COMPREPLY=( $(compgen -W "$verbs" -- "$cur") )
        return
    fi

    local verb=${words[1]}
    local sub=${words[2]}

    case "$verb" in
    focus)
        case "$cword" in
        2) COMPREPLY=( $(compgen -W "next prev last win tag mon" -- "$cur") ) ;;
        3)
            case "$sub" in
            win) COMPREPLY=( $(compgen -W "next prev last" -- "$cur") ) ;;
            tag|mon) COMPREPLY=( $(compgen -W "next prev" -- "$cur") ) ;;
            esac ;;
        esac ;;
    move)
        case "$cword" in
        2) COMPREPLY=( $(compgen -W "win tag" -- "$cur") ) ;;
        3)
            case "$sub" in
            win) COMPREPLY=( $(compgen -W "next prev master mon" -- "$cur") ) ;;
            tag) COMPREPLY=( $(compgen -W "next prev" -- "$cur") ) ;;
            esac ;;
        4)
            if [ "$sub" = win ] && [ "${words[3]}" = mon ]; then
                COMPREPLY=( $(compgen -W "next prev" -- "$cur") )
            fi ;;
        esac ;;
    swap)
        [ "$cword" -eq 2 ] && COMPREPLY=( $(compgen -W "next prev" -- "$cur") ) ;;
    layout)
        [ "$cword" -eq 2 ] && COMPREPLY=( $(compgen -W "cols stack float next prev" -- "$cur") ) ;;
    master)
        case "$cword" in
        2) COMPREPLY=( $(compgen -W "ratio +1 -1" -- "$cur") ) ;;
        3) [ "$sub" = ratio ] && COMPREPLY=( $(compgen -W "+0.05 -0.05" -- "$cur") ) ;;
        esac ;;
    gap)
        [ "$cword" -eq 2 ] && COMPREPLY=( $(compgen -W "+5 -5" -- "$cur") ) ;;
    bar|traybar)
        [ "$cword" -eq 2 ] && COMPREPLY=( $(compgen -W "toggle show hide" -- "$cur") ) ;;
    win)
        case "$cword" in
        2) COMPREPLY=( $(compgen -W "close tag toggle move resize center" -- "$cur") ) ;;
        3) [ "$sub" = toggle ] && COMPREPLY=( $(compgen -W "float full sticky" -- "$cur") ) ;;
        esac ;;
    query)
        [ "$cword" -eq 2 ] && COMPREPLY=( $(compgen -W "tags windows monitors layout state" -- "$cur") ) ;;
    completions)
        [ "$cword" -eq 2 ] && COMPREPLY=( $(compgen -W "bash zsh" -- "$cur") ) ;;
    esac
}
complete -F _mwmc mwmc
