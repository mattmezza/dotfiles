# Vars
	HISTFILE=~/.zsh_history
	SAVEHIST=10000
	setopt inc_append_history # To save every command before it is executed 
	setopt share_history # setopt inc_append_history

	git config --global push.default current

# Aliases
	alias vim="nvim"
	alias vi="nvim"
	mkdir -p /tmp/log
	
	# This is currently causing problems (fails when you run it anywhere that isn't a git project's root directory)
	# alias vs="v `git status --porcelain | sed -ne 's/^ M //p'`"

	# a better version of ls
	alias ls="exa"
	alias l="exa"
	# some aliases for easier navigation
	# Easier navigation: .., ..., ...., ..... and -
	alias ..="cd .."
	alias ...="cd ../.."
	alias ....="cd ../../.."
	alias .....="cd ../../../.."
	alias -- -="cd -"
	# Tmux stuff
	alias tx="tmuxinator"

# Settings
	export VISUAL=vim
	export VIMCONFIG="$HOME/dotfiles/vim/vimrc.vim"
	export TMUXINATOR_CONFIG="$HOME/tmux/.tmuxinator"
	export EDITOR="vim"

source ~/dotfiles/zsh/plugins/fixls.zsh

 	
# For vim mappings: 
	stty -ixon

# Completions
# These are all the plugin options available: https://github.com/robbyrussell/oh-my-zsh/tree/291e96dcd034750fbe7473482508c08833b168e3/plugins
#
# Edit the array below, or relocate it to ~/.zshrc before anything is sourced
# For help create an issue at github.com/parth/dotfiles

autoload -U compinit

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

compinit

source ~/dotfiles/zsh/plugins/oh-my-zsh/lib/history.zsh
source ~/dotfiles/zsh/plugins/oh-my-zsh/lib/key-bindings.zsh
source ~/dotfiles/zsh/plugins/oh-my-zsh/lib/completion.zsh
source ~/dotfiles/zsh/plugins/vi-mode.plugin.zsh
source ~/dotfiles/zsh/plugins/zsh-autosuggestions/zsh-autosuggestions.zsh
source ~/dotfiles/zsh/plugins/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh
source ~/dotfiles/zsh/keybindings.sh
source ~/dotfiles/jj.sh

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

source ~/dotfiles/zsh/prompt.sh
export PATH=$PATH:$HOME/dotfiles/utils
alias g=git
alias gti=git
alias got=git
alias compose=docker-compose
function pull() { git pull ;}
function push() { git push ;}
function push1() { git push1 ;}
git config --global include.path ~/dotfiles/gitalias.txt

# pyenv
export PYENV_ROOT="$HOME/.pyenv"
export PATH=$PATH:$PYENV_ROOT/bin

if command -v pyenv 1>/dev/null 2>&1; then
 eval "$(pyenv init -)"
 eval "$(pyenv virtualenv-init -)"
fi

# poetry
export PATH=$PATH:$HOME/.poetry/bin

# pip
export PIP_REQUIRE_VIRTUALENV=true
# cause sometimes you need to do this...
gpip() {
    PIP_REQUIRE_VIRTUALENV="" pip "$@"
}
gpip2() {
    PIP_REQUIRE_VIRTUALENV="" pip2 "$@"
}

# gurobi
export PATH=$PATH:$GUROBI_HOME/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GUROBI_HOME/lib

# pbcopy (as in macOS)
if [[ "$OSTYPE" == "linux-gnu" ]]; then
	alias pbcopy='xclip -selection clipboard'
	alias pbpaste='xclip -selection clipboard -o'
fi

function gi() { curl -sLw "\n" https://www.gitignore.io/api/$@ ;}

rand_hash() {
	cat /dev/urandom | head | md5 | cut -c1-${1-8}
}

c() {
	cd $1;
	ls;
}
alias cd="c"

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

# NVM - node version manager
export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"  # This loads nvm

export MYVIMRC="$HOME/dotfiles/vim/vimrc.vim"


# if there is an .extras.sh file then source it
if [ -f $HOME/.extras.sh ]; then
	source $HOME/.extras.sh
fi

source $HOME/dotfiles/plugins.sh
# after this point you can use `source <(plugin_install)` to install the plugins.

source $HOME/dotfiles/plugins/note/note.sh
source $HOME/dotfiles/plugins/todo/todo.sh
source $HOME/dotfiles/msg.sh
source $HOME/dotfiles/env_var.sh
source $HOME/dotfiles/tmux/sp.sh
