#!/bin/bash

# Script para actualizar IP del Master en configs de Query Control
# Uso: ./update-ips.sh <IP_MASTER>

if [ "$#" -ne 1 ]; then
    echo "❌ Error: Falta parámetro"
    echo "Uso: ./update-ips.sh <IP_MASTER>"
    echo "Ejemplo: ./update-ips.sh 192.168.1.100"
    exit 1
fi

IP_MASTER=$1

echo "🔄 Actualizando IP_MASTER en Query Control configs..."
echo "   Nueva IP_MASTER: $IP_MASTER"
echo ""

# Actualizar todos los archivos .config en configs/
for config in configs/*.config; do
    if [ -f "$config" ]; then
        sed -i "s/^IP_MASTER=.*/IP_MASTER=$IP_MASTER/" "$config"
        echo "   ✓ $config"
    fi
done

# Actualizar query.config en la raíz si existe
if [ -f "query.config" ]; then
    sed -i "s/^IP_MASTER=.*/IP_MASTER=$IP_MASTER/" "query.config"
    echo "   ✓ query.config"
fi

echo ""
echo "✅ IPs actualizadas correctamente!"
