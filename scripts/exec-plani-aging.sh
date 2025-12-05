#!/bin/bash

cd "$(dirname "$0")"

CMDS=(
    "./../storage/scripts/exec-plani-aging.sh"
    "./../master/scripts/exec-plani-aging.sh"
    "./../worker/scripts/exec-plani.sh"
    "./../query_control/scripts/exec-plani-aging.sh"
)

for CMD in "${CMDS[@]}"; do
    $CMD &
    sleep 0.5
done
