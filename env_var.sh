function env_var() {
	ENVFILE=".env"
	if [[ "$2" != "" ]]; then
		ENVFILE="$2"
	fi
	
	case "$1" in
		set)
			eval $(cat $ENVFILE | sed -e "s/^/export /")
			;;
		unset)
			eval $(cat $ENVFILE | sed -E "s/=.*/=''/")
			;;
		list)
			cat $ENVFILE | sed -E "s/=.*//" | xargs -n1 -I{} echo "echo -n '{}=' && (printenv {} | sed -E 's/\n$//')  && echo ''" | source /dev/stdin | grep -v "^$"
			;;
		mask)
			$0 list $ENVFILE | sed -E "s/=.+$/=\"\*\*\*\*\*\*\*\*\"/"
			;;
		help)
			echo "Usage:"
			echo "$ $0 set [.env]"
			echo "$ $0 unset [.env]"
			echo "$ $0 list [.env]"
			echo "$ $0 mask [.env]"
			echo "$ $0 help"
			echo ""
			echo "The env file used is $ENVFILE."
			;;
		*)
			echo "Missing command.\n$ $0 help"
			return 1
			;;
	esac
}

function env_passwd() {
	ENVFILE="$HOME/.env_passwd"
	if [[ "$ENV_PASSWD_FILE" != "" ]]; then
		ENVFILE="$ENV_PASSWD_FILE"
	fi
	if [[ "$1" == "list" ]]; then
		CMD="mask"
	elif [[ "$1" == "help" ]]; then
		echo "Usage:"
		echo "$ $0 set"
		echo "$ $0 unset"
		echo "$ $0 mask"
		echo "$ $0 help"
		echo ""
		echo "The env passwd file used is $ENVFILE."
		return 0
	else
		CMD="$1"
	fi
	env_var "$CMD" "$ENVFILE" || return 1
}
