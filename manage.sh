#!/bin/bash
#
# manage.sh — Unified dotfiles manager
#
# Usage:
#   ./manage.sh help                Show this help
#   ./manage.sh install  [what]     Install components (default: all)
#   ./manage.sh uninstall [what]    Uninstall components (default: all)
#
# Components (what):
#   deps       System packages / brew dependencies
#   pacco      Package manager pacco (lazy-loaded shell pkg mgr)
#   stow       GNU stow symlinks (dotfiles + system bins/config)
#   suckless   Build & install suckless tools (dwm, st, sent, etc.)
#   all        Every component above
#
# Examples:
#   ./manage.sh install              # everything
#   ./manage.sh install stow deps    # only stow + dependencies
#   ./manage.sh install suckless     # rebuild suckless tools
#   ./manage.sh uninstall deps       # remove system packages
#   ./manage.sh uninstall stow       # tear down symlinks
#
# Fuzzy matching on $what keeps backward compat with old args
# (e.g. "stowed", "suckless-*", "pacco-install").
# ────────────────────────────────────────────────────────────

set -euo pipefail

# ── Help ──────────────────────────────────────────────────────
show_help() {
    sed -n '/^# Usage:/,/^set -/p' "$0" | sed 's/^# \?//' | head -n -1
}

# ── Dependencies ──────────────────────────────────────────────
deps_linux_install() {
    sudo pacman -Sy "$(cat .arch-pacman)"
    yay -Sy "$(cat .arch-aur)"
}

deps_macos_install() {
    echo "Installing brew packages..."
    xargs brew install < brew.txt
}

deps_linux_uninstall() {
    sudo pacman --noconfirm -Rns "$(cat .arch-pacman)"
    yay --noconfirm -Rns "$(cat .arch-aur)"
}

deps_macos_uninstall() {
    echo "Uninstalling brew packages..."
    xargs brew uninstall < brew.txt
}

run_deps() {
    local action=$1
    if [[ $OSTYPE == darwin* ]]; then
        "deps_macos_${action}"
    else
        "deps_linux_${action}"
    fi
}

# ── pacco ─────────────────────────────────────────────────────
run_pacco() {
    local action=$1   # install | uninstall
    if [[ $action == install ]]; then
        echo "Installing pacco..."
        curl -s https://raw.githubusercontent.com/mattmezza/pacco/master/pacco.sh > /tmp/pacco \
            && source /tmp/pacco \
            && pacco i pacco https://github.com/mattmezza/pacco.git 1.0.0 \
            && echo "Installed pacco v$(pacco -v) in $(pacco -d)."
        pacco I
    else
        echo "Uninstalling pacco..."
        pacco U
    fi
}

# ── Stow ──────────────────────────────────────────────────────
declare -a ALL_PACKAGES=(
    alacritty bookmarker dunst flameshot fonts git gnupg hop i3
    nvim picom portals screenkey screenlayout spl sxhkd tmux
    wallpapers wallust wireplumber xorg zsh
)

stow_all() {
    stow "${ALL_PACKAGES[@]}"
    sudo stow --target=/usr/local/bin bin
    sudo stow --target=/etc/keyd keyd
    [[ $OSTYPE == darwin* ]] && stow hammerspoon
}

unstow_all() {
    stow -D "${ALL_PACKAGES[@]}"
    sudo stow -D --target=/usr/local/bin bin
    sudo stow -D --target=/etc/keyd keyd
    [[ $OSTYPE == darwin* ]] && stow -D hammerspoon
}

run_stow() {
    local action=$1
    local verb
    if [[ $action == install ]]; then
        verb="Stowing"
        stow_all
    else
        verb="Unstowing"
        unstow_all
    fi
    echo "${verb} files..."
}

# ── Suckless ──────────────────────────────────────────────────
declare -a SUCKLESS_DIRS=(dmenu mwm sent lok st nod)

run_suckless() {
    local action=$1   # install | uninstall
    local target
    if [[ $action == install ]]; then
        target="clean install"
    else
        target="uninstall"
    fi

    echo "${action^}ing suckless tools..."
    for dir in "${SUCKLESS_DIRS[@]}"; do
        (
            cd "$dir"
            rm -f config.h
            sudo make "$target"
        )
    done
}

# ── Dispatch ──────────────────────────────────────────────────
do_install() {
    local components=("$@")
    (( ${#components[@]} == 0 )) && components=(all)

    for what in "${components[@]}"; do
        case "$what" in
            all)       do_install deps pacco stow suckless ;;
            deps)      run_deps install ;;
            pacco)     run_pacco install ;;
            stow|stowed|stow*) run_stow install ;;
            suckless)  run_suckless install ;;
            *)         echo "Unknown component: $what"; exit 1 ;;
        esac
    done
}

do_uninstall() {
    local components=("$@")
    (( ${#components[@]} == 0 )) && components=(all)

    for what in "${components[@]}"; do
        case "$what" in
            all)       do_uninstall suckless stow pacco deps ;;
            deps)      run_deps uninstall ;;
            pacco)     run_pacco uninstall ;;
            stow|stowed|stow*) run_stow uninstall ;;
            suckless)  run_suckless uninstall ;;
            *)         echo "Unknown component: $what"; exit 1 ;;
        esac
    done
}

# ── Main ──────────────────────────────────────────────────────
main() {
    local cmd=${1:-}
    if [[ -z $cmd ]]; then
        echo "Error: missing command. Use 'help', 'install', or 'uninstall'." >&2
        show_help
        exit 1
    fi
    shift

    case "$cmd" in
        help|--help|-h)
            show_help
            ;;
        install|i)
            do_install "$@"
            ;;
        uninstall|remove|rm|u)
            do_uninstall "$@"
            ;;
        *)
            echo "Unknown command: $cmd" >&2
            show_help
            exit 1
            ;;
    esac
}

main "$@"
