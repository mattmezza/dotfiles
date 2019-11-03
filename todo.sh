todo() {
	TODAY=$(date "+%Y-%m-%d")
	TODO_DIR=${TODO_DIR_DEFAULT:-"$HOME/todo"}
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
	case "$1" in
		add|a)
			WHEN=${WHEN:-$TODAY}
			echo "- [ ] $2" >> "$TODO_DIR/$WHEN.md"
			;;
		done|d)
			WHEN=${WHEN:-$TODAY}
			sed -i "" "s/\- \[ \] $2$/- [x] $2/g" "$TODO_DIR/$WHEN.md"
			$0 view "$WHEN"
			;;
		undone|u)
			WHEN=${WHEN:-$TODAY}
			sed -i "" "s/\- \[x\] $2$/- [ ] $2/g" "$TODO_DIR/$WHEN.md"
			$0 view "$WHEN"
			;;
		help|h)
			echo "Usage:"
			echo "  $0 [CMD] [WHAT] [WHEN]"
			echo ""
			echo "CMD: can be add|a or view|v or done|d or undone|u. If no command is passed, 'view' is assumed."
			echo ""
			echo "WHEN: can be today|t or tomorrow|tomo or yesterday|y or it can be a temporal interaval referred to today's date (e.g. +2d, -2d etc...)."
			echo "  When CMD is 'view', it can also be a string matching the format '%Y-%m-%d' to list all the items in different days. If no WHEN is passed, 'today' is assumed."
			echo ""
			echo "$ $0 add|a 'Buy milk' [WHEN]"
			echo "$ $0 view|v [WHEN]"
			echo "$ $0 done|d 'Buy milk' [WHEN]"
			echo "$ $0 undone|u 'Buy milk' [WHEN]"
			echo ""
			;;
		view|v)
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
			find "$TODO_DIR" -name "$WHEN*.md" -type f -exec basename {} .md \; -exec cat {} \; -exec echo "" \; 2> /dev/null || echo "No todos for $WHEN."
			;;
		*)
			$0 view today
			;;
	esac
}
