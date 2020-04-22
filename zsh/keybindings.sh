up_widget() { BUFFER="cd .."; zle accept-line; }
zle -N up_widget
bindkey "^k" up_widget

git_prepare() {
    if [ -n "$BUFFER" ];
        then
            BUFFER="git add -A && git commit -m \"$BUFFER\" && git push"
    fi

    if [ -z "$BUFFER" ];
        then
            BUFFER="git add -A && git commit -v && git push"
    fi
    zle accept-line
}
zle -N git_prepare
bindkey "^g" git_prepare

goto_home() { BUFFER="cd ~/"$BUFFER; zle end-of-line; zle accept-line; }
zle -N goto_home
bindkey "^h" goto_home

edit_and_run() { BUFFER="fc"; zle accept-line; }
zle -N edit_and_run
bindkey "^b" edit_and_run

ctrl_l() { BUFFER="ls"; zle accept-line; }
zle -N ctrl_l
bindkey "^l" ctrl_l

enter_line() { zle accept-line }
zle -N enter_line
bindkey "^o" enter_line

add_sudo() { BUFFER="sudo "$BUFFER; zle end-of-line; }
zle -N add_sudo
bindkey "^s" add_sudo
bindkey "^k" clear-screen
