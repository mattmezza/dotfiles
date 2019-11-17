plugin_install_single() {
	[[ -z "$1" ]] && { return 1; }
	[[ -z "$2" ]] && { return 1; }
	[[ -z "$3" ]] && { return 1; }
	if [ -d "$3" ]; then
		;
	else
		echo "Installing $3 version $1"
		echo "git clone -q --branch $1 $2 $3"
	fi
}

plugin_install() {
	cat plugins.txt | xargs -L1 -I {} -P4 echo "plugin_install_single {}"
}
