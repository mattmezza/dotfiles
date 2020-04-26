stty -ixon
autoload -U compinit
compinit
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
    fpath=(~/dotfiles/zsh/plugins/oh-my-zsh/plugins/$plugin $fpath)
done

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
export VISUAL=vim
export VIMCONFIG="$HOME/dotfiles/vim/vimrc.vim"
export TMUXINATOR_CONFIG="$HOME/tmux/.tmuxinator"
export EDITOR="vim"
export PATH=$PATH:$HOME/dotfiles/utils
export MYVIMRC="$HOME/dotfiles/vim/.vimrc"
export PATH=$PATH:$GUROBI_HOME/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GUROBI_HOME/lib
export PIP_REQUIRE_VIRTUALENV=true
export PATH=$PATH:$HOME/.poetry/bin
export PYENV_ROOT="$HOME/.pyenv"
export PATH=$PATH:$PYENV_ROOT/bin
export NVM_DIR="$HOME/.nvm"
export DOT="$HOME/dotfiles"

#=========[ Functions ]===============
gpip() { PIP_REQUIRE_VIRTUALENV="" pip "$@" }
function gi() { curl -sLw "\n" https://www.gitignore.io/api/$@ ;}
rand_hash() { cat /dev/urandom | head | md5 | cut -c1-${1-8} }
c() { cd $1; ls; }
pull() { git pull ;}
push() { git push ;}
push1() { git push1 ;}
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

#=========[ Aliases ]===============
alias vim="nvim"
alias vi="nvim"
mkdir -p /tmp/log  # TODO find out why we need it
alias ..="cd .."
alias ...="cd ../.."
alias ....="cd ../../.."
alias .....="cd ../../../.."
alias -- -="cd -"
alias tx="tmuxinator"
alias cd="c"
alias g=git
alias gti=git
alias got=git
# This is currently causing problems
# (fails when you run it anywhere that isn't a git project's root directory)
# alias vs="v `git status --porcelain | sed -ne 's/^ M //p'`"
alias compose=docker-compose
alias ls="exa"
alias l="exa"

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    alias pbcopy='xclip -selection clipboard'
    alias pbpaste='xclip -selection clipboard -o'
    # pbcopy now works on linux as in macos
fi

[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"  # This loads nvm

# autoactivate virtualenv via pyenv
if command -v pyenv 1>/dev/null 2>&1; then
 eval "$(pyenv init -)"
 eval "$(pyenv virtualenv-init -)"
fi

eval "$(direnv hook zsh)"  # source .envrc if present

#==========[ Defaults ]=============
git config --global include.path ~/dotfiles/gitalias.txt
git config --global push.default current

if [ -f $HOME/.extras.sh ]; then
    source $HOME/.extras.sh
fi

source $DOT/zsh/plugins/fixls.zsh
source $DOT/zsh/plugins/oh-my-zsh/lib/history.zsh
source $DOT/zsh/plugins/oh-my-zsh/lib/key-bindings.zsh
source $DOT/zsh/plugins/oh-my-zsh/lib/completion.zsh
source $DOT/zsh/plugins/vi-mode.plugin.zsh
source $DOT/zsh/plugins/zsh-autosuggestions/zsh-autosuggestions.zsh
source $DOT/zsh/plugins/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh
source $DOT/zsh/keybindings.sh
source $DOT/zsh/prompt.sh

source $HOME/dotfiles/plugins.sh
# add plugin in plugins.txt and run `source <(plugin_install)` to install them.
source $HOME/dotfiles/plugins/note/note.sh
source $HOME/dotfiles/plugins/todo/todo.sh
source $HOME/dotfiles/plugins/jump/j.sh

source $HOME/dotfiles/msg.sh
source $HOME/dotfiles/env_var.sh

source $HOME/dotfiles/tmux/sp.sh

# start tmux
[ -z $TMUX ] && exec tmux new-session -A -s main