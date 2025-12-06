#!/bin/bash

# Script para actualizar IPs del Master y Storage en configs de Worker
# Uso: ./update-ips.sh <IP_MASTER> <IP_STORAGE>

if [ "$#" -ne 2 ]; then
    echo "❌ Error: Faltan parámetros"
    echo "Uso: ./update-ips.sh <IP_MASTER> <IP_STORAGE>"
    echo "Ejemplo: ./update-ips.sh 192.168.1.100 192.168.1.101"
    exit 1
fi

IP_MASTER=$1
IP_STORAGE=$2

echo "🔄 Actualizando IPs en Worker configs..."
echo "   Nueva IP_MASTER: $IP_MASTER"
echo "   Nueva IP_STORAGE: $IP_STORAGE"
echo ""

# Actualizar todos los archivos .config en configs/
for config in configs/*.config; do
    if [ -f "$config" ]; then
        sed -i "s/^IP_MASTER=.*/IP_MASTER=$IP_MASTER/" "$config"
        sed -i "s/^IP_STORAGE=.*/IP_STORAGE=$IP_STORAGE/" "$config"
        echo "   ✓ $config"
    fi
done

# Actualizar worker.config en la raíz si existe
if [ -f "worker.config" ]; then
    sed -i "s/^IP_MASTER=.*/IP_MASTER=$IP_MASTER/" "worker.config"
    sed -i "s/^IP_STORAGE=.*/IP_STORAGE=$IP_STORAGE/" "worker.config"
    echo "   ✓ worker.config"
fi

echo ""
echo "✅ IPs actualizadas correctamente!"
