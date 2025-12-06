#!/bin/bash

# Script para actualizar las IPs en todas las configuraciones
# Uso: ./update-ips.sh <IP_MASTER> <IP_STORAGE>

if [ "$#" -ne 2 ]; then
    echo "❌ Error: Faltan parámetros"
    echo "Uso: ./update-ips.sh <IP_MASTER> <IP_STORAGE>"
    echo "Ejemplo: ./update-ips.sh 192.168.1.100 192.168.1.101"
    exit 1
fi

IP_MASTER=$1
IP_STORAGE=$2

# Obtener el directorio raíz del proyecto (un nivel arriba de scripts/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "🔄 Actualizando IPs en todas las configuraciones..."
echo "   IP_MASTER: $IP_MASTER"
echo "   IP_STORAGE: $IP_STORAGE"
echo "   Directorio: $PROJECT_ROOT"
echo ""

# Función para actualizar archivos
update_config() {
    local file=$1
    local ip_type=$2
    local new_ip=$3
    
    if [ -f "$file" ]; then
        sed -i "s/^${ip_type}=.*/${ip_type}=${new_ip}/" "$file"
        echo "   ✓ Actualizado: $(basename $file)"
    fi
}

# Cambiar al directorio raíz del proyecto
cd "$PROJECT_ROOT"

# Actualizar Query Control
echo "📋 Query Control:"
for config in query_control/configs/*.config query_control/*.config; do
    if [ -f "$config" ]; then
        update_config "$config" "IP_MASTER" "$IP_MASTER"
    fi
done
echo ""

# Actualizar Worker
echo "🔧 Worker:"
for config in worker/configs/*.config worker/*.config; do
    if [ -f "$config" ]; then
        update_config "$config" "IP_MASTER" "$IP_MASTER"
        update_config "$config" "IP_STORAGE" "$IP_STORAGE"
    fi
done
echo ""

# Actualizar Master
echo "🎯 Master:"
for config in master/configs/*.config master/*.config; do
    if [ -f "$config" ]; then
        echo "   ℹ️  Master no tiene IPs para actualizar: $config"
    fi
done
echo ""

# Actualizar Storage
echo "💾 Storage:"
for config in storage/configs/*.config storage/*.config; do
    if [ -f "$config" ]; then
        echo "   ℹ️  Storage no tiene IPs para actualizar: $config"
    fi
done
echo ""

echo "✅ IPs actualizadas correctamente!"
