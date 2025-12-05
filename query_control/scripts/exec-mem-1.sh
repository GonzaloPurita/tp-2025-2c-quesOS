#!/bin/bash

cd "$(dirname "$0")"

CMD="./../bin/query_control ../configs/query.config MEMORIA_WORKER 0"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Query" --hold --command="$CMD" &
elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Query" -- bash -c "$CMD; exec bash" &
else
    xterm -hold -e "$CMD" &
fi
