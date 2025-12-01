#!/bin/bash

cd "$(dirname "$0")"

CMD="./../bin/storage ../configs/storage-plani.config"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Storage" --hold --command="$CMD" &

elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Storage" -- bash -c "$CMD; exec bash" &

else
    xterm -hold -e "$CMD" &
fi

