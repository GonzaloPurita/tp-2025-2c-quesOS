#!/bin/bash

# 1. Validamos que hayas pasado un parámetro. 
# -z chequea si la variable esta vacía.
if [ -z "$1" ]; then
    echo "❌ Error: Falta el parámetro del plan."
    echo "Uso correcto: ./lanzarVarios.sh <NOMBRE_PLAN>"
    echo "Ejemplo: ./lanzarVarios.sh AGING_1"
    exit 1
fi

PLAN=$1 # Guardamos el primer parámetro (ej: AGING_1) en una variable
CANTIDAD=25

echo "Lanzando $CANTIDAD instancias para el plan: $PLAN"

# 2. El bucle de 25 iteraciones
for i in $(seq 1 $CANTIDAD)
do
   # Comando exacto que pediste:
   # $PLAN se reemplaza por lo que escribas al ejecutar (ej: AGING_1)
   # > /dev/null 2>&1 silencia la salida para no saturar tu terminal
   
   ./bin/query_control query.config $PLAN 20 > /dev/null 2>&1 &
   
done

echo "Listo! Las $CANTIDAD instancias se están ejecutando en segundo plano."