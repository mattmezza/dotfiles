prompt_install() {
    echo -n "$1 is not installed. Would you like to install it? (y/n) " >&2
    old_stty_cfg=$(stty -g)
    stty raw -echo
    answer=$( while ! head -c 1 | grep -i '[ny]' ;do true ;done )
    stty $old_stty_cfg && echo
    if echo "$answer" | grep -iq "^y" ;then
        # This could def use community support
        if [ -x "$(command -v apt-get)" ]; then
            sudo apt-get install $1 -y
        elif [ -x "$(command -v brew)" ]; then
            brew install $1
        elif [ -x "$(command -v pkg)" ]; then
            sudo pkg install $1
        elif [ -x "$(command -v pacman)" ]; then
            sudo pacman -S $1
        else
            echo "Could not figure out pkg manager automatically."
        fi
    fi
}

check_for_software() {
    if ! [ -x "$(command -v $1)" ]; then
        prompt_install $1
    fi
}

check_default_shell() {
    if [ -z "${SHELL##*zsh*}" ] ;then
        echo "Default shell is zsh."
    else
        echo -n "Default shell is not zsh. Do you want to chsh -s \$(which zsh)? (y/n)"
        old_stty_cfg=$(stty -g)
        stty raw -echo
        answer=$( while ! head -c 1 | grep -i '[ny]' ;do true ;done )
        stty $old_stty_cfg && echo
        if echo "$answer" | grep -iq "^y" ;then
            chsh -s $(which zsh)
        else
            echo "Warning: Your configuration won't work properly. If you exec zsh, it'll exec tmux which will exec your default shell which isn't zsh."
        fi
    fi
}

install_pacco() {
	curl -s https://raw.githubusercontent.com/mattmezza/pacco/master/pacco.sh > /tmp/pacco &&\
        source /tmp/pacco &&\
        pacco i pacco https://github.com/mattmezza/pacco.git 1.0.0 &&\
        echo "Installed pacco v$(pacco -v) in $(pacco -d)."
}

echo "We're going to do the following:"
echo "1. Check to make sure you have zsh, vim, and tmux installed"
echo "2. We'll help you install them if you don't"
echo "3. We're going to check to see if your default shell is zsh"
echo "4. We'll try to change it if it's not" 

echo "Let's get started? (y/n)"
old_stty_cfg=$(stty -g)
stty raw -echo
answer=$( while ! head -c 1 | grep -i '[ny]' ;do true ;done )
stty $old_stty_cfg
if echo "$answer" | grep -iq "^y" ;then
    echo 
else
    echo "Quitting, nothing was changed."
    exit 0
fi


check_for_software zsh
echo
check_for_software vim
echo
check_for_software tmux
echo
install_pacco
echo

check_default_shell

echo
echo -n "Would you like to backup your current dotfiles? (y/n) "
old_stty_cfg=$(stty -g)
stty raw -echo
answer=$( while ! head -c 1 | grep -i '[ny]' ;do true ;done )
stty $old_stty_cfg
if echo "$answer" | grep -iq "^y" ;then
    mv ~/.zshrc ~/.zshrc.old
    mv ~/.tmux.conf ~/.tmux.conf.old
    mv ~/.vimrc ~/.vimrc.old
fi

cat brew.txt | xargs -n1 brew install

printf "source '$HOME/dotfiles/zsh/.zshrc'" > ~/.zshrc
printf "so $HOME/dotfiles/vim/.vimrc" > ~/.vimrc
printf "source-file $HOME/dotfiles/.tmux.conf" > ~/.tmux.conf

echo
echo "Please log out and log back in for default shell to be initialized."
echo "Afterwards, run 'pacco I' to install all the custom plugins."

# to supercharge the caps key
mkdir -p ~/.hammerspoon/Spoons
git clone https://github.com/jasonrudolph/ControlEscape.spoon.git ~/.hammerspoon/Spoons/ControlEscape.spoon

# install oh-my-zsh
sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
