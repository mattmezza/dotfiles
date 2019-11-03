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
	case "$2" in
		today|t)
			WHEN_VIEW=$TODAY
			;;
		yesterday|y)
			WHEN_VIEW=$(date -v-1d "+%Y-%m-%d")
			;;
		tomorrow|tm|tomo)
			WHEN_VIEW=$(date -v+1d "+%Y-%m-%d")
			;;
		*)
			if [[ $2 =~ ^[+-][0-9]+d$ ]]; then
				WHEN_VIEW=$(date -v$2 "+%Y-%m-%d")
			elif [ -z "$2" ]; then
				WHEN_VIEW=$TODAY
			else
				WHEN_VIEW=$2
			fi
			;;
	esac
	case "$1" in
		add|a)
			WHEN=${WHEN:-$TODAY}
			echo "- [ ] $2" >> "$TODO_DIR/$WHEN.md"
			$0 view "$WHEN"
			;;
		remove|rm)
			WHEN=${WHEN:-$TODAY}
			sed -i "" "/^\- \[[ x]\] $2$/d" "$TODO_DIR/$WHEN.md"
			$0 view "$WHEN"
			;;
		done|d)
			WHEN=${WHEN:-$TODAY}
			sed -i "" "s/^\- \[ \] $2$/- [x] $2/g" "$TODO_DIR/$WHEN.md"
			$0 view "$WHEN"
			;;
		undone|u)
			WHEN=${WHEN:-$TODAY}
			sed -i "" "s/^\- \[x\] $2$/- [ ] $2/g" "$TODO_DIR/$WHEN.md"
			$0 view "$WHEN"
			;;
		help|h)
			echo "Usage:"
			echo "  $0 [CMD] [WHAT] [WHEN]"
			echo ""
			echo "CMD can be:"
			echo "  'add|a' to add element to list"
			echo "  'remove|rm' to remove an element from a list"
			echo "  'view|v' to print the list on stdout"
			echo "  'left|l' to print the items left to do for WHEN"
			echo "  'did' to print the items already did for WHEN"
			echo "  'done|d' or undone|u to mark or unmark the item as done"
			echo "  'carry-on|co moves yesterday's undone items to today's list"
			echo "  'help|h' prints this message"
			echo "If no command is passed, 'view' is assumed."
			echo ""
			echo "WHEN can be:"
			echo "  today|t"
			echo "  tomorrow|tomo|tm"
			echo "  yesterday|y"
			echo "  a temporal interaval referred to today's date (e.g. +2d, -2d etc...)."
			echo "When CMD is 'view', it can also be a string matching the format '%Y-%m-%d' to list all the items in different days. If no WHEN is passed, 'today' is assumed."
			echo ""
			echo "Examples:"
			echo "$ $0 add 'Buy milk'"
			echo "$ $0 rm 'Buy milk'"
			echo "$ $0 view tomorrow"
			echo "$ $0 view 2019-11"
			echo "$ $0 left today"
			echo "$ $0 did yesterday"
			echo "$ $0 done 'Buy milk' yesterday"
			echo "$ $0 undone|u 'Buy milk' yesterday"
			echo "$ $0 carry-on"
			echo ""
			;;
		view|v)
			find "$TODO_DIR" -name "$WHEN_VIEW*.md" -type f -exec basename {} .md \; -exec cat {} \; -exec echo "" \; 2> /dev/null || echo "No todos for $WHEN_VIEW."
			;;
		left|l)
			echo "$WHEN_VIEW"
			$0 view "$WHEN_VIEW" | grep -e "^\- \[ \] .*$"
			;;
		did)
			echo "$WHEN_VIEW"
			$0 view "$WHEN_VIEW" | grep -e "^\- \[x\] .*$"
			;;
		carry-on|co)
			$0 left yesterday | tail -n 1 >> "$TODO_DIR/$TODAY.md"
			$0 view today
			;;
		*)
			$0 view today
			;;
	esac
}
