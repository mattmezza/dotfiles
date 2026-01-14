deps_macos() {
    echo "Uninstalling brew packages..."
    cat brew.txt | xargs brew uninstall
}

deps_linux() {
    sudo pacman --noconfirm -Rns $(cat .arch-pacman)
    yay --noconfirm -Rns $(cat .arch-aur)
}

uninstall_deps() {
    if [[ $OSTYPE == darwin* ]]; then
        deps_macos
    else
        deps_linux
    fi
}

unstow_all() {
    stow -D
        alacritty \
        bookmarker \
        dunst \
        flameshot \
        git \
        i3 \
        nvim \
        picom \
        screenkey \
        screenlayout \
        sxhkd \
        tmux \
        wallpapers \
        xorg \
        zsh
    sudo stow -D --target=/usr/local/bin bin

    if [[ $OSTYPE == darwin* ]]; then
        stow -D hammerspoon
    fi
}

uninstall_suckless() {
    cd dmenu && sudo make uninstall && cd ..
    cd dwm && sudo make uninstall && cd ..
    cd sent && sudo make uninstall && cd ..
    cd slock && sudo make uninstall && cd ..
    cd slstatus && sudo make uninstall && cd ..
    cd st && sudo make uninstall && cd ..
}


if [[ "$1" =~ ^.*(suckless).*$ ]]; then
    echo "Uninstalling suckless..."
    uninstall_suckless
fi
if [[ "$1" =~ ^.*(stow|stowed).*$ ]]; then
    echo "Unstowing files..."
    unstow_all
fi
if [[ "$1" =~ ^.*(pacco).*$ ]]; then
    echo "Uninstalling pacco..."
    pacco U
fi
if [[ "$1" =~ ^.*(deps).*$ ]]; then
    echo "Uninstalling dependencies..."
    uninstall_deps
fi
