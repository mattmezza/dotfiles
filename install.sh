#!/bin/bash

deps_linux() {
    sudo pacman -Sy `cat .arch-pacman`
    yay -Sy `cat .arch-aur`
}

deps_macos() {
    echo "Installing brew packages..."
    cat brew.txt | xargs brew install
}

install_deps() {
    if [[ $OSTYPE == darwin* ]]; then
        deps_macos
    else
        deps_linux
    fi
}


install_pacco() {
	curl -s https://raw.githubusercontent.com/mattmezza/pacco/master/pacco.sh > /tmp/pacco &&\
        source /tmp/pacco &&\
        pacco i pacco https://github.com/mattmezza/pacco.git 1.0.0 &&\
        echo "Installed pacco v$(pacco -v) in $(pacco -d)."
}

stow_all() {
    stow alacritty bookmarker dunst git i3 nvim picom screenlayout sxhkd tmux xorg wallpapers zsh
    sudo stow --target=/usr/local/bin bin

    if [[ $OSTYPE == darwin* ]]; then
        stow hammerspoon
    fi
}

install_suckless() {
    cd dmenu && rm -f config.h && sudo make clean install && cd ..
    cd dwm && rm -f config.h patches.h && sudo make clean install && cd ..
    cd sent && rm -f config.h && sudo make clean install && cd ..
    cd slock && rm -f config.h && sudo make clean install && cd ..
    cd slstatus && rm -f config.h && sudo make clean install && cd ..
    cd st && rm -f config.h && sudo make clean install && cd ..
}

if [[ "$1" =~ ^.*(deps).*$ ]]; then
    echo "Installing dependencies..."
    install_deps
fi
if [[ "$1" =~ ^.*(pacco).*$ ]]; then
    echo "Installing pacco..."
    install_pacco
    pacco I
fi
if [[ "$1" =~ ^.*(stow|stowed).*$ ]]; then
    echo "Stowing files..."
    stow_all
fi
if [[ "$1" =~ ^.*(suckless).*$ ]]; then
    echo "Installing suckless..."
    install_suckless
fi
