function note() {
	NOTE_DIR="$HOME"
	if [ -d "${NOTE_SCRATCH_DIR}" ]; then
		NOTE_DIR="${NOTE_SCRATCH_DIR}"
	fi

	case "$1" in
		list)
			ls -1 $NOTE_DIR/*.txt
			;;
		new)
			FILE_NAME="$(date +%Y%m%d%H%M)-$(echo $2 | sed 's/ /_/g')"
			$VISUAL "$NOTE_DIR/$FILE_NAME.txt"
			;;
		open)
			case "$2" in
				last)
					$VISUAL $(note last)
					;;
				*)
					$VISUAL "$NOTE_DIR/$2"
					;;
			esac
			;;
		path)
			echo "$NOTE_DIR/$2"
			;;
		rm)
			case "$2" in
				last)
					rm -i $(note last)
					;;
				*)
					rm -i "$NOTE_DIR/$2"
					;;
			esac
			;;
		last)
			echo $(note list | tail -n 1)
			;;
		help)
			echo "Here's what you can do with `$0`:\n"
			echo "$ $0 list"
			echo "$ $0 new name"
			echo "$ $0 open name"
			echo "$ $0 path name"
			echo "$ $0 rm name"
			echo "\nYour scratch notes dir is: $NOTE_DIR"
			;;
		*)
			echo "Missing command.\n$ $0 help"
			return 1
			;;
	esac
}
