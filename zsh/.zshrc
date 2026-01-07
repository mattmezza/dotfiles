stty -ixon
HISTFILE=$HOME/.zsh_history
SAVEHIST=10000
HISTSIZE=10000 # this limits search history

export DOT="$HOME/dotfiles"
export XDG_CONFIG_HOME="$HOME/.config"
export COMPLETIONS="$XDG_CONFIG_HOME/zsh/completions"

if command -v brew &> /dev/null
then
    export BREW_PREFIX=$(brew --prefix)
fi

completions_regen() {
    if which gh > /dev/null; then gh completion --shell zsh > $COMPLETIONS/gh; fi
}

fpath=($COMPLETIONS $fpath)
autoload -U compinit && compinit

#==============[ keybindings ]================
add_sudo() { BUFFER="sudo "$BUFFER; zle end-of-line; }
zle -N add_sudo
bindkey "^s" add_sudo
bindkey "^k" clear-screen

# Updates editor information when the keymap changes.
zle-keymap-select() {
  zle reset-prompt
  zle -R
}

# Ensure that the prompt is redrawn when the terminal size changes.
TRAPWINCH() {
  zle &&  zle -R
}

zle -N zle-keymap-select

bindkey '^p' up-history
bindkey '^n' down-history
# bindkey '^w' backward-kill-word # default
bindkey '^b' backward-word
bindkey '^a' beginning-of-line
bindkey '^e' end-of-line
bindkey '^r' history-incremental-pattern-search-backward

setopt inc_append_history # To save every command before it is executed
setopt share_history # setopt inc_append_history

# Fix for arrow-key searching
# start typing + [Up-Arrow] - fuzzy find history forward
if [[ "${terminfo[kcuu1]}" != "" ]]; then
    autoload -U up-line-or-beginning-search
    zle -N up-line-or-beginning-search
    bindkey "${terminfo[kcuu1]}" up-line-or-beginning-search
fi
# start typing + [Down-Arrow] - fuzzy find history backward
if [[ "${terminfo[kcud1]}" != "" ]]; then
    autoload -U down-line-or-beginning-search
    zle -N down-line-or-beginning-search
    bindkey "${terminfo[kcud1]}" down-line-or-beginning-search
fi

#=========[ Exports ]===============
export VISUAL=nvim
export EDITOR=nvim
export TERMINAL=st
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk
export PIP_REQUIRE_VIRTUALENV=true
export PYENV_ROOT="$HOME/.pyenv"
export NVM_DIR="$HOME/.nvm"
export PACCO_DIR="$DOT/pacchi"
export PACCO_FILE="$DOT/pacco.txt"
export GPG_TTY=$(tty)
export LC_ALL=en_US.UTF-8
export TERM=st-256color

gpip() { PIP_REQUIRE_VIRTUALENV="" pip "$@" }
gi() { curl -sLw "\n" https://www.gitignore.io/api/$@ ;}
loop() {
    echo "executing '${@:1}' every time you press 'enter' (^C to quit)."
    while true; do
        echo "$(date --iso-8601=seconds)"
        "${@:1}"
        read
    done
}
alias vim="nvim"
alias vi="nvim"
alias tmux="TERM=xterm-256color tmux"
alias ..="cd .."
alias ...="cd ../.."
alias ....="cd ../../.."
alias .....="cd ../../../.."
alias g=git
alias gti=git
alias got=git
alias c="git rev-parse HEAD"
alias cs="git rev-parse HEAD | cut -c-7"
alias b="git branch"
alias br="git branch -d"
alias cb="git checkout -b"
co() {
    git checkout $(git branch --list | grep $1 | sed 's/\*//' | xargs -n1)
}
pull() { git pull ;}
push() { git push ;}
push1() { git push1 ;}
alias compose=docker-compose
alias k8=kubectl
pid() {ps -A | grep "$1"  | grep -v "grep" | awk '{print $1}'}
if ! command -v nproc &> /dev/null
then
    alias nproc="sysctl -n hw.ncpu"
fi
alias nproc-1="expr $(nproc) - 1"

[ -s "/opt/homebrew/opt/nvm/nvm.sh" ] && \. "/opt/homebrew/opt/nvm/nvm.sh"  # This loads nvm
[ -s "/opt/homebrew/opt/nvm/etc/bash_completion.d/nvm" ] && \. "/opt/homebrew/opt/nvm/etc/bash_completion.d/nvm"  # This loads nvm bash_completion
[ -s "/usr/share/nvm/init-nvm.sh" ] && source "/usr/share/nvm/init-nvm.sh"  # This loads nvm on Linux

# autoactivate virtualenv via pyenv
if command -v pyenv 1>/dev/null 2>&1; then
 eval "$(pyenv init --path)"
 eval "$(pyenv virtualenv-init -)"
fi

eval "$(direnv hook zsh)"  # source .envrc if present

source $XDG_CONFIG_HOME/zsh/prompt.sh

source $PACCO_DIR/pacco/pacco.sh
pacco source-all  # this sources all the pacco pkgs
export NOTE_SCRATCH_DIR="$HOME/notes"
alias spot=$PACCO_DIR/spot/spot.sh  # this pkg needs custom sourcing

if test -d "$HOME/google-cloud-sdk"; then
    source "$HOME/google-cloud-sdk/completion.zsh.inc"
    source "$HOME/google-cloud-sdk/path.zsh.inc"
fi

# start tmux automatically
# [ -z $TMUX ] && exec tmux new-session -A -s main

# add a path to the PATH
export PATH="/usr/local/opt/openssl@1.1/bin:$PATH"

# bun completions
[ -s "$HOME/.bun/_bun" ] && source "$HOME/.bun/_bun"

# bun
export BUN_INSTALL="$HOME/.bun"
export PATH="$BUN_INSTALL/bin:$PATH"

# .local/bin
export PATH="$HOME/.local/bin:$PATH"
