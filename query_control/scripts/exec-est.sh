#!/bin/bash

# Nos aseguramos de estar en el directorio del script
cd "$(dirname "$0")"

# Definimos las rutas y la prioridad constante
BIN="./../bin/query_control"
CONFIG="../configs/query.config"
PRIORITY=20

echo "Lanzando 25 instancias de cada Query (Total: 100 procesos)..."

for i in {1..25}
do
    # Ejecutamos cada instancia en segundo plano (&)
    # Redirigimos la salida a /dev/null para no saturar la terminal
    
    $BIN $CONFIG AGING_1 $PRIORITY > /dev/null 2>&1 &
    $BIN $CONFIG AGING_2 $PRIORITY > /dev/null 2>&1 &
    $BIN $CONFIG AGING_3 $PRIORITY > /dev/null 2>&1 &
    $BIN $CONFIG AGING_4 $PRIORITY > /dev/null 2>&1 &
done

echo "¡Listo! Se han iniciado 100 procesos en segundo plano con prioridad $PRIORITY."
