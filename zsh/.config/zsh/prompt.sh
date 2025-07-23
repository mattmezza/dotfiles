# Reference for colors: http://stackoverflow.com/questions/689765/how-can-i-change-the-color-of-my-prompt-in-zsh-different-from-normal-text
autoload -U colors && colors
setopt PROMPT_SUBST

autoload -Uz vcs_info
zstyle ':vcs_info:*' enable git
zstyle ':vcs_info:*' check-for-changes true
zstyle ':vcs_info:*' check-for-staged true
# Configure vcs_info to format the git string for us
# %b -> branch name
# %u -> unstaged changes indicator
# %c -> staged changes indicator
zstyle ':vcs_info:git:*' formats \
    '%{$fg[yellow]%}%c%{$reset_color%}%{$fg[red]%}%u%{$reset_color%}%{$fg[magenta]%}%b%{$reset_color%}'
zstyle ':vcs_info:git:*' actionformats \
    '%{$fg[yellow]%}%c%{$reset_color%}%{$fg[red]%}%u%{$reset_color%}%{$fg[magenta]%}%b|%a%{$reset_color%}'

# Define the symbols to use for changes
zstyle ':vcs_info:*' unstagedstr '+'
zstyle ':vcs_info:*' stagedstr '!'


set_prompt() {
    PS1=''
    RPS1=""

    if [[ $VIRTUAL_ENV != "" ]]; then
        PS1+="%{$fg[green]%}($(basename $VIRTUAL_ENV)) %{$reset_color%}"
    fi

    PS1+="%{$fg[white]%}%{$reset_color%}"
    PS1+="%{$fg_bold[cyan]%}${PWD/#$HOME/~}%{$reset_color%}"
    PS1+='%(?.. %{$fg[red]%}%?%{$reset_color%}) '

    # The entire formatted string is in $vcs_info_msg_0_
    # We just check if it's not empty, which means we're in a repo.
    if [[ -n "$vcs_info_msg_0_" ]]; then
        # Add a space if the timer is already in RPS1
        [[ -n "$RPS1" ]] && RPS1+=" "
        RPS1+="$vcs_info_msg_0_"
    fi

    # Timer: http://stackoverflow.com/questions/2704635/is-there-a-way-to-find-the-running-time-of-the-last-executed-command-in-the-shel
    if [[ $_elapsed[-1] -gt 3 ]]; then
        RPS1="%{$fg[magenta]%}$_elapsed[-1]s%{$reset_color%} $RPS1"
    fi
}

precmd_functions+=vcs_info # Ensure vcs_info is called before precmd
precmd_functions+=set_prompt

preexec() {
    # If array is too large, remove the oldest elements
    while (( ${#_elapsed[@]} > 1000 )); do
        _elapsed=( "${_elapsed[@]:1}" ) # Shifts array, slightly more efficient than reassigning entire slice
    done
    _start=$SECONDS
}

precmd() {
   (( _start >= 0 )) && _elapsed+=($(( SECONDS-_start )))
   _start=-1
}
