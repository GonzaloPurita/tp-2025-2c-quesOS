#!/bin/bash

cd "$(dirname "$0")"

CMDS=(
    "./../storage/scripts/exec-mem.sh"
    "./../master/scripts/exec-mem.sh"
    "./../worker/scripts/exec-mem-1.sh"
    "./../query_control/scripts/exec-mem-1.sh"
)

for CMD in "${CMDS[@]}"; do
    $CMD &
    sleep 1
done
