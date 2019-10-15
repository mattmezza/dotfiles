function env_var() {
	if [[ "$2" == "" && "$1" != "help" ]]; then
		echo "No such file: '$2'."
		return 1
	fi
	
	case "$1" in
		set)
			eval $(cat $2 | sed -e "s/^/export /")
			;;
		unset)
			eval $(cat $2 | sed -E "s/=.*/=''/")
			;;
		list)
			cat $2 | sed -E "s/=.*//" | xargs -n1 -I{} echo "echo -n '{}=' && (printenv {} | sed -E 's/\n$//')  && echo ''" | source /dev/stdin | grep -v "^$"
			;;
		mask)
			cat $2 | sed -E "s/=.*/=\"\*\*\*\*\*\*\*\*\"/"
			;;
		help)
			echo "Usage:"
			echo "$ $0 set .env"
			echo "$ $0 unset .env"
			echo "$ $0 list .env"
			echo "$ $0 mask .env"
			echo "$ $0 help"
			echo ""
			echo "The env file used is $2."
			;;
		*)
			echo "Missing command.\n$ $0 help"
			return 1
			;;
	esac
}

function env_passwd() {
	if [[ "$1" == "list" ]]; then
		CMD="mask"
	elif [[ "$1" == "help" ]]; then
		echo "Usage:"
		echo "$ $0 set"
		echo "$ $0 unset"
		echo "$ $0 mask"
	else
		CMD="$1"
	fi
	(test -f "$ENV_PASSWD_FILE" && env_var $CMD $ENV_PASSWD_FILE || env_var set "$HOME/.env_passwd") && return 0 || return 1
}
