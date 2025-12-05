#!/bin/bash

set -euo pipefail

# Script para dar permisos de ejecución a los scripts del proyecto
# - Da chmod +x a todos los archivos *.sh
# - Da chmod +x a archivos que empiecen con shebang (#!) aunque no terminen en .sh

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Proyecto: $BASE_DIR"
echo "Buscando archivos .sh y archivos con shebang..."

changed_count=0

# 1) Archivos .sh
while IFS= read -r -d '' file; do
  if [ -f "$file" ]; then
    if [ ! -x "$file" ]; then
      chmod +x "$file" && echo "✓ Ejecutable: $file"
      ((changed_count++))
    else
      echo "· Ya ejecutable: $file"
    fi
  fi
done < <(find "$BASE_DIR" -type f -name "*.sh" -print0 2>/dev/null || true)

# 2) Archivos con shebang (#!) que no terminan en .sh
while IFS= read -r -d '' file; do
  # Excluir archivos .sh (ya procesados) y el directorio .git
  case "$file" in
    *.sh) continue ;;
    */.git/*) continue ;;
  esac

  if [ -f "$file" ]; then
    firstline=$(head -n 1 "$file" 2>/dev/null || true)
    if [[ "$firstline" == \#!* ]]; then
      if [ ! -x "$file" ]; then
        chmod +x "$file" && echo "✓ Ejecutable (shebang): $file"
        ((changed_count++))
      else
        echo "· Ya ejecutable (shebang): $file"
      fi
    fi
  fi
done < <(find "$BASE_DIR" -type f -print0 2>/dev/null || true)

echo -e "\nProceso completado. Archivos marcados como ejecutables: $changed_count"

exit 0
