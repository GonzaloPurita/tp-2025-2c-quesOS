#!/bin/bash

cd "$(dirname "$0")"

CMD1="./../bin/query_control ../configs/query.config ESCRITURA_ARCHIVO_COMMITED 1"
CMD2="./../bin/query_control ../configs/query.config FILE_EXISTENTE 1"
CMD3="./../bin/query_control ../configs/query.config LECTURA_FUERA_DEL_LIMITE 1"
CMD4="./../bin/query_control ../configs/query.config TAG_EXISTENTE 1"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Query 1" --hold --command="$CMD1" &
    xfce4-terminal --title="Query 2" --hold --command="$CMD2" &
    xfce4-terminal --title="Query 3" --hold --command="$CMD3" &
    xfce4-terminal --title="Query 4" --hold --command="$CMD4" &
elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Query 1" -- bash -c "$CMD1; exec bash" &
    gnome-terminal --title="Query 2" -- bash -c "$CMD2; exec bash" &
    gnome-terminal --title="Query 3" -- bash -c "$CMD3; exec bash" &
    gnome-terminal --title="Query 4" -- bash -c "$CMD4; exec bash" &
else
    xterm -hold -e "$CMD1" &
    xterm -hold -e "$CMD2" &
    xterm -hold -e "$CMD3" &
    xterm -hold -e "$CMD4" &
fi
