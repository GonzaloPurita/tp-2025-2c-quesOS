#!/bin/bash

cd "$(dirname "$0")"

CMD1="./../bin/worker ../configs/worker1-est.config 1"
CMD2="./../bin/worker ../configs/worker2-est.config 2"
CMD3="./../bin/worker ../configs/worker3-est.config 3"
CMD4="./../bin/worker ../configs/worker4-est.config 4"
CMD5="./../bin/worker ../configs/worker5-est.config 5"
CMD6="./../bin/worker ../configs/worker6-est.config 6"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Worker 1" --hold --command="$CMD1" &
    xfce4-terminal --title="Worker 2" --hold --command="$CMD2" &
    xfce4-terminal --title="Worker 3" --hold --command="$CMD3" &
    xfce4-terminal --title="Worker 4" --hold --command="$CMD4" &
    xfce4-terminal --title="Worker 5" --hold --command="$CMD5" &
    xfce4-terminal --title="Worker 6" --hold --command="$CMD6" &

elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Worker 1" -- bash -c "$CMD1; exec bash" &
    gnome-terminal --title="Worker 2" -- bash -c "$CMD2; exec bash" &
    gnome-terminal --title="Worker 3" -- bash -c "$CMD3; exec bash" &
    gnome-terminal --title="Worker 4" -- bash -c "$CMD4; exec bash" &
    gnome-terminal --title="Worker 5" -- bash -c "$CMD5; exec bash" &
    gnome-terminal --title="Worker 6" -- bash -c "$CMD6; exec bash" &

else
    xterm -hold -e "$CMD1" &
    xterm -hold -e "$CMD2" &
    xterm -hold -e "$CMD3" &
    xterm -hold -e "$CMD4" &
    xterm -hold -e "$CMD5" &
    xterm -hold -e "$CMD6" &

fi
