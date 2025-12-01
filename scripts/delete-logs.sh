#!/bin/bash

# Script para eliminar todos los archivos de logs del proyecto

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Eliminando archivos de logs...${NC}"

# Directorio base del proyecto
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Archivos de logs específicos
LOGS=(
    "$BASE_DIR/query_control/queryControl.log"
    "$BASE_DIR/master/master.log"
    "$BASE_DIR/worker/worker.log"
    "$BASE_DIR/storage/storage.log"
)

# Contador de archivos eliminados
deleted_count=0

# Eliminar cada archivo de log si existe
for log_file in "${LOGS[@]}"; do
    if [ -f "$log_file" ]; then
        rm -f "$log_file"
        echo -e "${GREEN}✓${NC} Eliminado: $log_file"
        ((deleted_count++))
    else
        echo -e "${YELLOW}○${NC} No existe: $log_file"
    fi
done

# Buscar y eliminar cualquier otro archivo .log en el proyecto
echo -e "\n${YELLOW}Buscando otros archivos .log...${NC}"
while IFS= read -r -d '' file; do
    rm -f "$file"
    echo -e "${GREEN}✓${NC} Eliminado: $file"
    ((deleted_count++))
done < <(find "$BASE_DIR" -type f -name "*.log" -print0 2>/dev/null)

echo -e "\n${GREEN}Proceso completado.${NC} Archivos eliminados: $deleted_count"
