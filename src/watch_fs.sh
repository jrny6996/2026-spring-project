
watch_sync() {
    SRC="$1"
    DST="$2"

    rsync -a --delete "$SRC/" "$DST/"

    while inotifywait -r -e modify,create,delete,move "$SRC"; do
        sleep 1
        rsync -a --delete "$SRC/" "$DST/"
        trap 'exit 0' INT TERM
    done
}

watch_sync ../src/static/js ./static/js &
watch_sync ../src/templates ./templates &
wait