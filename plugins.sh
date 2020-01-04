plugin_install_single() {
	[[ -z "$1" ]] && { return 1; }
	[[ -z "$2" ]] && { return 1; }
	[[ -z "$3" ]] && { return 1; }
	if [ -d "$3" ]; then
		echo "Uninstalling $3:$1"
		rm -rf "$3"
		mkdir "$3"
		echo "Installing $3:$1"
		git clone -q --branch $1 $2 $3
		echo "Installed $3:$1"
	else
		return 1
	fi
}

plugin_install() {
	cat plugins.txt | xargs -L1 -I {} -P4 echo "plugin_install_single {}"
}
