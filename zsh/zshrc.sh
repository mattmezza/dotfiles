# Vars
	HISTFILE=~/.zsh_history
	SAVEHIST=1000 
	setopt inc_append_history # To save every command before it is executed 
	setopt share_history # setopt inc_append_history

	git config --global push.default current

# Aliases
	alias vim="nvim"
	alias vi="nvim"
	alias v="vim -p"
	mkdir -p /tmp/log
	
	# This is currently causing problems (fails when you run it anywhere that isn't a git project's root directory)
	# alias vs="v `git status --porcelain | sed -ne 's/^ M //p'`"

# Settings
	export VISUAL=vim

source ~/dotfiles/zsh/plugins/fixls.zsh

#Functions
	# Loop a command and show the output in vim
	loop() {
		echo ":cq to quit\n" > /tmp/log/output 
		fc -ln -1 > /tmp/log/program
		while true; do
			cat /tmp/log/program >> /tmp/log/output ;
			$(cat /tmp/log/program) |& tee -a /tmp/log/output ;
			echo '\n' >> /tmp/log/output
			vim + /tmp/log/output || break;
			rm -rf /tmp/log/output
		done;
	}

 	# Custom cd
 	c() {
 		cd $1;
 		ls;
 	}
 	alias cd="c"

# For vim mappings: 
	stty -ixon

# Completions
# These are all the plugin options available: https://github.com/robbyrussell/oh-my-zsh/tree/291e96dcd034750fbe7473482508c08833b168e3/plugins
#
# Edit the array below, or relocate it to ~/.zshrc before anything is sourced
# For help create an issue at github.com/parth/dotfiles

autoload -U compinit

plugins=(
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

# pip
export PIP_REQUIRE_VIRTUALENV=true

# gurobi
export GUROBI_HOME=/Library/gurobi801/mac64
export GRB_LICENSE_FILE=$HOME/gurobi.lic
export PATH=$PATH:$GUROBI_HOME/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GUROBI_HOME/lib

# pbcopy (as in macOS)
if [[ "$OSTYPE" == "linux-gnu" ]]; then
	alias pbcopy='xclip -selection clipboard'
	alias pbpaste='xclip -selection clipboard -o'
fi

# gitignore
function gi() { curl -sLw "\n" https://www.gitignore.io/api/$@ ;}

# install imessage cli if needed
if [ -f '/usr/local/imessage' ]; then
	;
else
	sudo curl -o /usr/local/imessage https://gist.githubusercontent.com/aktau/8958054/raw/95f62b2e04cf6e21a0724f1cae6bb508f71b46b3/imessage && sudo chmod a+x /usr/local/imessage;
fi

# alias msg for rapid messaging `msg buddy "msg"`
function msg() {
	RCPT=""
	if [ -f '.contacts.txt' ]; then
		RCPT=$(cat .contacts.txt | grep " ($1)" | cut -f 1 -d ' ' |  xargs -n1 -I{})
	else
		RCPT=$1
	fi
	/usr/local/imessage $RCPT $2
}

# take a note (a scratch one)
function note() {
	NOTE_DIR="$HOME"
	if [ -d "${NOTE_SCRATCH_DIR}" ]; then
		NOTE_DIR="${NOTE_SCRATCH_DIR}"
	fi

	case "$1" in
		list)
			ls $NOTE_DIR
			;;
		new)
			FILE_NAME="$(date +%Y%m%d%H%M)-$(echo $2 | sed 's/ /_/g')"
			$VISUAL "$NOTE_DIR/$FILE_NAME.txt"
			;;
		open)
			$VISUAL "$NOTE_DIR/$2"
			;;
		path)
			echo "$NOTE_DIR/$2"
			;;
		rm)
			rm -i "$NOTE_DIR/$2"
			;;
		help)
			echo "Here's what you can do with `note`:\n"
			echo "$ note list"
			echo "$ note new name"
			echo "$ note open name"
			echo "$ note path name"
			echo "$ note rm name"
			echo "\nYour scratch notes dir is: $NOTE_DIR"
			;;
		*)
			echo "Missing command.\n$ note help"
			return 1
			;;
	esac
}

# if there is an .extras.sh file then source it
if [ -f '.extras.sh' ]; then
	source $HOME/dotfiles/.extras.sh
fi
