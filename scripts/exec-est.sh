#!/bin/bash

cd "$(dirname "$0")"

CMDS=(
    "./../storage/scripts/exec-est.sh"
    "./../master/scripts/exec-est.sh"
    "./../worker/scripts/exec-est.sh"
    "./../query_control/scripts/exec-est.sh"
)

for CMD in "${CMDS[@]}"; do
    $CMD &
    sleep 1
done
