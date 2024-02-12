export DOT="$HOME/dotfiles"
export COMPLETIONS="$DOT/zsh/completions"
if command -v brew &> /dev/null
then
    export BREW_PREFIX=$(brew --prefix)
fi

stty -ixon
HISTFILE=~/.zsh_history
SAVEHIST=10000

plugins=(
    brew
    cask
    cargo
    celery
    composer
    cp
    docker-compose
    git
    git-extras
    github
    golang
    gradle
    history
    kubectl
    man
    osx
    node
    npm
    pep8
    pip
    postgres
    pyenv
    pylint
    python
    redis-cli
    ruby
    sbt
    scala
    screen
    sudo
    tmux
    wp-cli
    yarn
    docker
    virtualenv
    safe-paste
)

for plugin ($plugins); do
    fpath=($DOT/zsh/plugins/oh-my-zsh/plugins/$plugin $fpath)
done

completions_regen() {
    if which gh > /dev/null; then gh completion --shell zsh > $COMPLETIONS/gh; fi
}
fpath=($DOT/zsh/completions $fpath)
autoload -U compinit && compinit

#==============[ keybindings ]================
gg() {
    if [ -n "$BUFFER" ];
        then
            BUFFER="git add -A && git commit -m \"$BUFFER\""
    fi

    if [ -z "$BUFFER" ];
        then
            BUFFER="git add -A && git commit -v"
    fi
    zle accept-line
}
zle -N gg
bindkey "^g" gg

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
export VIMCONFIG="$DOT/vim/init.vim"
export MYVIMRC=$VIMCONFIG
export EDITOR=nvim
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/jvm/java-21-openjdk/lib
export PIP_REQUIRE_VIRTUALENV=true
export PATH=$PATH:$HOME/.poetry/bin
export PYENV_ROOT="$HOME/.pyenv"
export PATH=$PATH:$PYENV_ROOT/bin
#export PATH="$(brew --prefix)/opt/openjdk/bin:$PATH"
#export JDK_HOME="$(brew --prefix)/opt/openjdk"
export NVM_DIR="$HOME/.nvm"
export PACCO_DIR="$DOT/pacchi"
export PACCO_FILE="$DOT/pacco.txt"

gpip() { PIP_REQUIRE_VIRTUALENV="" pip "$@" }
function gi() { curl -sLw "\n" https://www.gitignore.io/api/$@ ;}
loop() {
    echo "Executing '${@:1}'..."
    clear
    "${@:1}"
    while true; do
        echo -n "At $(date)"
        read
        clear
        "${@:1}"
    done
}

# alias vim="nvim -u $DOT/vim/init.vim"
alias vim="nvim"
alias vi="vim"
mkdir -p /tmp/log  # TODO find out why we need it
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
bj() {
    git branch --list | grep ${1-" "} | grep ${2-${1-" "}} | sed 's/\*//' | xargs -n1
}
pull() { git pull ;}
push() { git push ;}
push1() { git push1 ;}
# This is currently causing problems
# (fails when you run it anywhere that isn't a git project's root directory)
# alias vs="v `git status --porcelain | sed -ne 's/^ M //p'`"
alias compose=docker-compose
if which exa > /dev/null; then
    alias ls="exa";
    alias l="exa";
    alias ll="exa --long --header --group --inode --blocks --links --modified --accessed --created --git"
    alias la="ll -a"
    alias tree="exa -T"
fi
if which bat > /dev/null; then alias cat=bat; fi
alias k=kubectl

pid () {
    ps -A | grep "$1"  | grep -v "grep" | awk '{print $1}'
}
if ! command -v nproc &> /dev/null
then
    alias nproc="sysctl -n hw.ncpu"
    alias nproc-1="expr $(nproc) - 1"
fi

[ -s "/opt/homebrew/opt/nvm/nvm.sh" ] && \. "/opt/homebrew/opt/nvm/nvm.sh"  # This loads nvm
[ -s "/opt/homebrew/opt/nvm/etc/bash_completion.d/nvm" ] && \. "/opt/homebrew/opt/nvm/etc/bash_completion.d/nvm"  # This loads nvm bash_completion
[ -s "/usr/share/nvm/init-nvm.sh" ] && source "/usr/share/nvm/init-nvm.sh"  # This loads nvm on Linux

# autoactivate virtualenv via pyenv
if command -v pyenv 1>/dev/null 2>&1; then
 eval "$(pyenv init --path)"
 eval "$(pyenv virtualenv-init -)"
fi

eval "$(direnv hook zsh)"  # source .envrc if present

#==========[ Defaults ]=============
git config --global include.path $DOT/gitalias.txt
git config --global push.default current

if [ -f $HOME/.extras.sh ]; then
    source $HOME/.extras.sh
fi

source $DOT/zsh/plugins/oh-my-zsh/lib/history.zsh
source $DOT/zsh/plugins/oh-my-zsh/lib/key-bindings.zsh
source $DOT/zsh/plugins/oh-my-zsh/lib/completion.zsh
source $DOT/zsh/plugins/vi-mode.plugin.zsh
# source $DOT/zsh/plugins/zsh-autosuggestions/zsh-autosuggestions.zsh
# source $BREW_PREFIX/share/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh
source $DOT/zsh/prompt.sh

source $PACCO_DIR/pacco/pacco.sh
pacco source-all  # this sources all the pacco pkgs
export NOTE_SCRATCH_DIR="$HOME/notes"
alias spot=$PACCO_DIR/spot/spot.sh  # this pkg needs custom sourcing

source $DOT/msg.sh
source $DOT/env_var.sh

export PATH="/usr/local/opt/krb5/bin:$PATH"
export PATH="/usr/local/opt/krb5/sbin:$PATH"
export PATH="/usr/local/opt/openssl@1.1/bin:$PATH"
export PATH="$BREW_PREFIX/Cellar/libpq/15.1/bin:$PATH"  # exported to use psql (not from postgresql) https://stackoverflow.com/questions/44654216/correct-way-to-install-psql-without-full-postgres-on-macos
export PATH="$HOME/google-cloud-sdk/bin:$PATH"  # gcloud
export GPG_TTY=$(tty)

# start tmux automatically
# [ -z $TMUX ] && exec tmux new-session -A -s main

export PATH="$HOME/.yarn/bin:$HOME/.config/yarn/global/node_modules/.bin:$DOT/bin:$PATH"
export PATH="$DOT/bin:$PATH"
export TERMINAL=alacritty
