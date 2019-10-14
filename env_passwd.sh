function env_passwd() {
	if [[ "$ENV_PASSWD_FILE" == "" ]]; then
		ENV_PASSWD_FILE="$HOME/.env_passwd"
	fi
	
	case "$1" in
		set)
			eval $(cat $ENV_PASSWD_FILE | sed -e "s/^/export /")
			;;
		unset)
			eval $(cat $ENV_PASSWD_FILE | sed -E "s/=.*/=''/")
			;;
		list)
			cat $ENV_PASSWD_FILE | sed -E "s/=.*/=\"\*\*\*\*\*\*\*\*\"/"
			;;
		help)
			echo "Usage:"
			echo "$ $0 set"
			echo "$ $0 unset"
			echo "$ $0 list"
			echo ""
			echo "The password file used is $ENV_PASSWD_FILE."
			;;
		*)
			echo "Missing command.\n$ $0 help"
			return 1
			;;
	esac
}
