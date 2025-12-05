#!/bin/bash

cd "$(dirname "$0")"

CMD="./../bin/master ../configs/master-mem.config"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Master" --hold --command="$CMD" &

elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Master" -- bash -c "$CMD; exec bash" &

else
    xterm -hold -e "$CMD" &
fi
