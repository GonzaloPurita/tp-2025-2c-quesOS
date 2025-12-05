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

echo "=========================================="
echo "Prueba de estabilidad completa"
echo "STORAGE con VALGRIND"
echo "=========================================="
echo "El log de valgrind se guardará en: $VALGRIND_LOGS_DIR/storage-est_${TIMESTAMP}.log"
echo ""

# Opciones comunes de valgrind para memory leaks
VALGRIND_OPTS="--leak-check=full --show-leak-kinds=all --track-origins=yes --verbose"

# 1. Lanzar Storage CON valgrind
cd ../storage/scripts
CMD_STORAGE="../bin/storage ../configs/storage-est.config"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Storage [VALGRIND]" --hold --command="bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/storage-est_${TIMESTAMP}.log $CMD_STORAGE; exec bash'" &
elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Storage [VALGRIND]" -- bash -c "valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/storage-est_${TIMESTAMP}.log $CMD_STORAGE; exec bash" &
else
    xterm -hold -e "bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/storage-est_${TIMESTAMP}.log $CMD_STORAGE; exec bash'" &
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

# 3. Lanzar Workers SIN valgrind
cd ../../worker/scripts
CMD1="../bin/worker ../configs/worker1-est.config 1"
CMD2="../bin/worker ../configs/worker2-est.config 2"
CMD3="../bin/worker ../configs/worker3-est.config 3"
CMD4="../bin/worker ../configs/worker4-est.config 4"
CMD5="../bin/worker ../configs/worker5-est.config 5"
CMD6="../bin/worker ../configs/worker6-est.config 6"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Worker 1" --hold --command="$CMD1" &
    xfce4-terminal --title="Worker 2" --hold --command="$CMD2" &
    xfce4-terminal --title="Worker 3" --hold --command="$CMD3" &
    xfce4-terminal --title="Worker 4" --hold --command="$CMD4" &
    xfce4-terminal --title="Worker 5" --hold --command="$CMD5" &
    xfce4-terminal --title="Worker 6" --hold --command="$CMD6" &
elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Worker 1" -- bash -c "$CMD1; exec bash" &
    gnome-terminal --title="Worker 2" -- bash -c "$CMD2; exec bash" &
    gnome-terminal --title="Worker 3" -- bash -c "$CMD3; exec bash" &
    gnome-terminal --title="Worker 4" -- bash -c "$CMD4; exec bash" &
    gnome-terminal --title="Worker 5" -- bash -c "$CMD5; exec bash" &
    gnome-terminal --title="Worker 6" -- bash -c "$CMD6; exec bash" &
else
    xterm -hold -e "$CMD1" &
    xterm -hold -e "$CMD2" &
    xterm -hold -e "$CMD3" &
    xterm -hold -e "$CMD4" &
    xterm -hold -e "$CMD5" &
    xterm -hold -e "$CMD6" &
fi

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
echo "Storage corriendo con VALGRIND"
echo "=========================================="
echo ""
echo "Para ver los resultados después de finalizar:"
echo "  cat $VALGRIND_LOGS_DIR/storage-est_${TIMESTAMP}.log | grep 'LEAK SUMMARY' -A 10"
echo "  cat $VALGRIND_LOGS_DIR/storage-est_${TIMESTAMP}.log | grep 'ERROR SUMMARY'"
echo ""
