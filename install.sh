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
    stow alacritty bookmarker git i3 nvim screenlayout tmux xorg
    stow --no-folding zsh
    sudo stow --target=/usr/local/bin bin

    if [[ $OSTYPE == darwin* ]]; then
        stow hammerspoon
    fi
}

#install_deps
#install_pacco
#pacco I
#stow_all
