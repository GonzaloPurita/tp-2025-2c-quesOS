#!/bin/bash

cd "$(dirname "$0")"

CMDS=(
    "./../storage/scripts/exec-sto.sh"
    "./../master/scripts/exec-sto.sh"
    "./../worker/scripts/exec-sto.sh"
    "./../query_control/scripts/exec-sto.sh"
)

for CMD in "${CMDS[@]}"; do
    $CMD &
    sleep 1
done
