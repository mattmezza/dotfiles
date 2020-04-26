plugin_install_single() {
    [[ -z "$1" ]] && { return 1 }
    [[ -z "$2" ]] && { return 1 }
    [[ -z "$DOT/$3" ]] && { return 1 }

    if [ -d "$DOT/$3" ]; then
        rm -rf "$DOT/$3"
        echo "Uninstalled '$3'"
    fi

    echo "Installing '$3'..."
    mkdir -p "$DOT/$3"
    git clone -q --branch $1 $2 "$DOT/$3" > /dev/null 2> /dev/null
    echo "Installed '$3:$1'"
    return 0
}

plugin_install() {
    source <(cat plugins.txt | while read in; do echo "plugin_install_single $in"; done)
}
