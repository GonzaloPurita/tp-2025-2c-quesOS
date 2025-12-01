#!/bin/bash

cd "$(dirname "$0")"

CMD1="./../bin/worker ../configs/worker1-plani.config 1"
CMD2="./../bin/worker ../configs/worker2-plani.config 2"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Worker 1" --hold --command="$CMD1" &
    xfce4-terminal --title="Worker 2" --hold --command="$CMD2" &

elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Worker 1" -- bash -c "$CMD1; exec bash" &
    gnome-terminal --title="Worker 2" -- bash -c "$CMD2; exec bash" &

else
    xterm -hold -e "$CMD1" &
    xterm -hold -e "$CMD2" &
fi
