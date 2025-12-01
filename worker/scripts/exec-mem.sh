#!/bin/bash

cd "$(dirname "$0")"

CMD="./../bin/worker ../configs/worker-mem.config 1"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Worker" --hold --command="$CMD" &

elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Worker" -- bash -c "$CMD; exec bash" &

else
    xterm -hold -e "$CMD" &
fi
