#!/bin/bash

cd "$(dirname "$0")"

CMDS=(
    "./../storage/scripts/exec-err.sh"
    "./../master/scripts/exec-err.sh"
    "./../worker/scripts/exec-err.sh"
    "./../query_control/scripts/exec-err.sh"
)

for CMD in "${CMDS[@]}"; do
    $CMD &
    sleep 1
done
