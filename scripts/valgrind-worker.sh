#!/bin/bash

cd "$(dirname "$0")"

# Verificar que valgrind esté instalado
if ! command -v valgrind &> /dev/null; then
    echo "Error: valgrind no está instalado. Instálalo con: sudo apt-get install valgrind"
    exit 1
fi

# Crear directorio para los logs de valgrind si no existe
VALGRIND_LOGS_DIR="../valgrind-logs"
mkdir -p "$VALGRIND_LOGS_DIR"

# Timestamp para identificar esta ejecución
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Por defecto lanzar 1 worker con valgrind, pero se puede especificar cuántos
NUM_WORKERS_VALGRIND=${1:-1}

if [ "$NUM_WORKERS_VALGRIND" -gt 6 ]; then
    echo "Máximo 6 workers permitidos"
    NUM_WORKERS_VALGRIND=6
fi

echo "=========================================="
echo "Prueba de estabilidad completa"
echo "$NUM_WORKERS_VALGRIND WORKER(S) con VALGRIND"
echo "=========================================="
echo "Los logs se guardarán en: $VALGRIND_LOGS_DIR/worker*-est_${TIMESTAMP}.log"
echo ""

# Opciones comunes de valgrind para memory leaks
VALGRIND_OPTS="--leak-check=full --show-leak-kinds=all --track-origins=yes --verbose"

# 1. Lanzar Storage SIN valgrind
cd ../storage/scripts
CMD_STORAGE="../bin/storage ../configs/storage-est.config"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Storage" --hold --command="$CMD_STORAGE" &
elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Storage" -- bash -c "$CMD_STORAGE; exec bash" &
else
    xterm -hold -e "$CMD_STORAGE" &
fi

sleep 2

# 2. Lanzar Master SIN valgrind
cd ../../master/scripts
CMD_MASTER="../bin/master ../configs/master-est.config"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Master" --hold --command="$CMD_MASTER" &
elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Master" -- bash -c "$CMD_MASTER; exec bash" &
else
    xterm -hold -e "$CMD_MASTER" &
fi

sleep 2

# 3. Lanzar Workers - algunos CON valgrind, otros SIN
cd ../../worker/scripts

# Workers CON valgrind
for i in $(seq 1 $NUM_WORKERS_VALGRIND); do
    CMD="../bin/worker ../configs/worker${i}-est.config ${i}"
    
    if command -v xfce4-terminal &> /dev/null; then
        xfce4-terminal --title="Worker $i [VALGRIND]" --hold --command="bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker${i}-est_${TIMESTAMP}.log $CMD; exec bash'" &
    elif command -v gnome-terminal &> /dev/null; then
        gnome-terminal --title="Worker $i [VALGRIND]" -- bash -c "valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker${i}-est_${TIMESTAMP}.log $CMD; exec bash" &
    else
        xterm -hold -e "bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker${i}-est_${TIMESTAMP}.log $CMD; exec bash'" &
    fi
    
    sleep 0.5
done

# Workers restantes SIN valgrind (para completar 6)
for i in $(seq $(($NUM_WORKERS_VALGRIND + 1)) 6); do
    CMD="../bin/worker ../configs/worker${i}-est.config ${i}"
    
    if command -v xfce4-terminal &> /dev/null; then
        xfce4-terminal --title="Worker $i" --hold --command="$CMD" &
    elif command -v gnome-terminal &> /dev/null; then
        gnome-terminal --title="Worker $i" -- bash -c "$CMD; exec bash" &
    else
        xterm -hold -e "$CMD" &
    fi
    
    sleep 0.5
done

sleep 2

# 4. Lanzar Query Control SIN valgrind
cd ../../query_control/scripts
BIN="../bin/query_control"
CONFIG="../configs/query.config"
PRIORITY=20

echo "Lanzando 25 instancias de cada Query (Total: 100 procesos)..."

for i in {1..25}
do
    $BIN $CONFIG AGING_1 $PRIORITY > /dev/null 2>&1 &
    $BIN $CONFIG AGING_2 $PRIORITY > /dev/null 2>&1 &
    $BIN $CONFIG AGING_3 $PRIORITY > /dev/null 2>&1 &
    $BIN $CONFIG AGING_4 $PRIORITY > /dev/null 2>&1 &
done

cd ../../scripts

echo ""
echo "=========================================="
echo "Prueba lanzada exitosamente"
echo "$NUM_WORKERS_VALGRIND Worker(s) corriendo con VALGRIND"
echo "=========================================="
echo ""
echo "Para ver los resultados después de finalizar:"
echo "  cat $VALGRIND_LOGS_DIR/worker*-est_${TIMESTAMP}.log | grep 'LEAK SUMMARY' -A 10"
echo "  cat $VALGRIND_LOGS_DIR/worker*-est_${TIMESTAMP}.log | grep 'ERROR SUMMARY'"
echo ""
echo "Uso: $0 [NUM_WORKERS_CON_VALGRIND]"
echo "Ejemplo: $0 2  (lanza 2 workers con valgrind y 4 sin valgrind)"
echo ""
