# Reference for colors: http://stackoverflow.com/questions/689765/how-can-i-change-the-color-of-my-prompt-in-zsh-different-from-normal-text
autoload -U colors && colors
setopt PROMPT_SUBST

excerpt() {
    MAX_LENGTH=$(($2 + $3))
    if [ ${#1} -ge $MAX_LENGTH ]; then
        echo $1 | sed -E 's/(.{0,15}).*(.{15})/\1...\2/'
    else
        echo $1
    fi
}

set_prompt() {
    PS1=''
    RPS1=""

    if [[ $VIRTUAL_ENV != "" ]]; then
        PS1+="%{$fg[green]%}($(basename $VIRTUAL_ENV)) %{$reset_color%}"
    fi

    # [: insert the first char in the next line as follows: %{$fg[white]%}[%{$reset_color%}
    PS1+="%{$fg[white]%}%{$reset_color%}"

    # Path: http://stevelosh.com/blog/2010/02/my-extravagant-zsh-prompt/
    PS1+="%{$fg_bold[cyan]%}${PWD/#$HOME/~}%{$reset_color%}"

    # Status Code
    PS1+='%(?.. %{$fg[red]%}%?%{$reset_color%})'

    # Git
    if git rev-parse --is-inside-work-tree 2> /dev/null | grep -q 'true' ; then
        BRANCH=$(git rev-parse --abbrev-ref HEAD 2> /dev/null)
        RPS1+="%{$fg[magenta]%}$(excerpt $BRANCH 15 15)%{$reset_color%}"
        if [ $(git status --short | wc -l) -gt 0 ]; then
            RPS1+="%{$fg[red]%}+$(git status --porcelain | wc -l | awk '{$1=$1};1')%{$reset_color%}"
        fi
    fi


    # Timer: http://stackoverflow.com/questions/2704635/is-there-a-way-to-find-the-running-time-of-the-last-executed-command-in-the-shel
    if [[ $_elapsed[-1] -gt 3 ]]; then
        RPS1="%{$fg[magenta]%}$_elapsed[-1]s%{$reset_color%} $RPS1"
    fi

    # PID
    if [[ $! -ne 0 ]]; then
        RPS1+="%{$fg[yellow]%}PID:$!%{$reset_color%} $RPS1"
    fi

    # Sudo: https://superuser.com/questions/195781/sudo-is-there-a-command-to-check-if-i-have-sudo-and-or-how-much-time-is-left
    CAN_I_RUN_SUDO=$(sudo -n uptime 2>&1|grep "load"|wc -l)
    if [ ${CAN_I_RUN_SUDO} -gt 0 ]
    then
        PS1+=$'%{$fg[white]%} # %{$reset_color%}% '
    else
        PS1+=$'%{$fg[white]%} $ %{$reset_color%}% '
    fi

}

precmd_functions+=set_prompt

preexec() {
   (( ${#_elapsed[@]} > 1000 )) && _elapsed=(${_elapsed[@]: -1000})
   _start=$SECONDS
}

precmd() {
   (( _start >= 0 )) && _elapsed+=($(( SECONDS-_start )))
   _start=-1
}
