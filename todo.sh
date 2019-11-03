todo() {
	TODAY=$(date "+%Y-%m-%d")
	TODO_DIR=${TODO_DIR_DEFAULT:-"$HOME/todo"}
	case "$1" in
		add)
			case "$3" in
				today|t)
					WHEN=$TODAY
					;;
				tomorrow|tm|tomo)
					WHEN=$(date -v+1d "+%Y-%m-%d")
					;;
				*)
					if [[ $3 =~ ^[+-][0-9]+d$ ]]; then
						WHEN=$(date -v$3 "+%Y-%m-%d")
					else
						WHEN=$3
					fi
					;;
			esac
			WHEN=${WHEN:-$TODAY}
			echo "- [ ] $2" >> "$TODO_DIR/$WHEN.md"
			;;
		help)
			echo "Usage:"
			echo "$ $0 add 'Buy milk'"
			echo "$ $0 view [today|tomorrow|yesterday|+2d|-2d]"
			echo "$ $0 view [t|tm|y|+2d|-2d]"
			echo ""
			;;
		view)
			case "$2" in
				today|t)
					WHEN=$TODAY
					;;
				yesterday|y)
					WHEN=$(date -v-1d "+%Y-%m-%d")
					;;
				tomorrow|tm|tomo)
					WHEN=$(date -v+1d "+%Y-%m-%d")
					;;
				*)
					if [[ $2 =~ ^[+-][0-9]+d$ ]]; then
						WHEN=$(date -v$2 "+%Y-%m-%d")
					elif [ -z "$2" ]; then
						WHEN=$TODAY
					else
						WHEN=$2
					fi
					;;
			esac
			find "$TODO_DIR" -name "$WHEN*.md" -type f -exec basename {} \; -exec cat {} \; 2> /dev/null || echo "No todos for $WHEN."
			;;
		*)
			$0 view today
			;;
	esac
}
