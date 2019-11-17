plugin_install() {
	cat plugins.txt | xargs -L1 -I {} echo "git clone -q --branch {}" | bash
}
