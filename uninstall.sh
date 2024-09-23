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
    stow -D alacritty bookmarker git i3 nvim screenlayout tmux xorg
    stow -D -no-folding zsh
    sudo stow -D --target=/usr/local/bin bin

    if [[ $OSTYPE == darwin* ]]; then
        stow -D hammerspoon
    fi
}

#unstow_all
#pacco U
#uninstall_deps
