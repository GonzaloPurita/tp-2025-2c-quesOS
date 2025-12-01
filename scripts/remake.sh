#!/bin/bash

set -e

cd "$(dirname "$0")"

DIRS=(
    "../storage"
    "../master"
    "../worker"
    "../query_control"
)

echo "Building..."

for DIR in "${DIRS[@]}"; do
    echo "Processing $DIR..."

    make -C "$DIR" clean --no-print-directory || true
    
    # 2. Compilar
    make -C "$DIR" --no-print-directory
    
    echo -e "\e[32m✔ $DIR compiled.\e[0m"
    echo -e "\n-----------------------------------\n"
done

echo -e "\e[32m✔ Build succesfull\e[0m"
