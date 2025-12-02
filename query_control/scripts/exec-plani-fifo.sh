#!/bin/bash

cd "$(dirname "$0")"

CMD1="./../bin/query_control ../configs/query.config FIFO_1 4"
CMD2="./../bin/query_control ../configs/query.config FIFO_2 3"
CMD3="./../bin/query_control ../configs/query.config FIFO_3 5"
CMD4="./../bin/query_control ../configs/query.config FIFO_4 1"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Query 1" --hold --command="$CMD1" &
    xfce4-terminal --title="Query 2" --hold --command="$CMD2" &
    xfce4-terminal --title="Query 3" --hold --command="$CMD3" &
    xfce4-terminal --title="Query 4" --hold --command="$CMD4" &
elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Query 1" -- bash -c "$CMD1; exec bash" &
    sleep 1
    gnome-terminal --title="Query 2" -- bash -c "$CMD2; exec bash" &
    sleep 1
    gnome-terminal --title="Query 3" -- bash -c "$CMD3; exec bash" &
    sleep 1
    gnome-terminal --title="Query 4" -- bash -c "$CMD4; exec bash" &
    sleep 1
else
    xterm -hold -e "$CMD1" &
    xterm -hold -e "$CMD2" &
    xterm -hold -e "$CMD3" &
    xterm -hold -e "$CMD4" &
fi
