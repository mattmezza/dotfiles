plugin_install_single() {
	[[ -z "$1" ]] && { return 1; }
	[[ -z "$2" ]] && { return 1; }
	[[ -z "$3" ]] && { return 1; }
	if [ -d "$3" ]; then
		echo ""
		rm -rf "$3"
		echo "Uninstalled '$3'"
		mkdir "$3"
		echo "Installing '$3'..."
		git clone -q --branch $1 $2 $3 > /dev/null 2> /dev/null
		echo "Installed '$3:$1'"
	else
		echo "$3 is not a directory"
		return 1
	fi
}

plugin_install() {
	source <(cat plugins.txt | while read in; do echo "plugin_install_single $in"; done)
}
