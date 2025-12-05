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

echo "Ejecutando prueba est con valgrind..."
echo "Los logs se guardarán en: $VALGRIND_LOGS_DIR"

# Opciones comunes de valgrind para memory leaks
VALGRIND_OPTS="--leak-check=full --show-leak-kinds=all --track-origins=yes --verbose"

# Lanzar Storage con valgrind
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

# Lanzar Master con valgrind
cd ../../master/scripts
CMD_MASTER="../bin/master ../configs/master-est.config"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Master [VALGRIND]" --hold --command="bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/master-est_${TIMESTAMP}.log $CMD_MASTER; exec bash'" &
elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Master [VALGRIND]" -- bash -c "valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/master-est_${TIMESTAMP}.log $CMD_MASTER; exec bash" &
else
    xterm -hold -e "bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/master-est_${TIMESTAMP}.log $CMD_MASTER; exec bash'" &
fi

sleep 2

# Lanzar Workers con valgrind
cd ../../worker/scripts
CMD1="../bin/worker ../configs/worker1-est.config 1"
CMD2="../bin/worker ../configs/worker2-est.config 2"
CMD3="../bin/worker ../configs/worker3-est.config 3"
CMD4="../bin/worker ../configs/worker4-est.config 4"
CMD5="../bin/worker ../configs/worker5-est.config 5"
CMD6="../bin/worker ../configs/worker6-est.config 6"

if command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal --title="Worker 1 [VALGRIND]" --hold --command="bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker1-est_${TIMESTAMP}.log $CMD1; exec bash'" &
    xfce4-terminal --title="Worker 2 [VALGRIND]" --hold --command="bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker2-est_${TIMESTAMP}.log $CMD2; exec bash'" &
    xfce4-terminal --title="Worker 3 [VALGRIND]" --hold --command="bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker3-est_${TIMESTAMP}.log $CMD3; exec bash'" &
    xfce4-terminal --title="Worker 4 [VALGRIND]" --hold --command="bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker4-est_${TIMESTAMP}.log $CMD4; exec bash'" &
    xfce4-terminal --title="Worker 5 [VALGRIND]" --hold --command="bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker5-est_${TIMESTAMP}.log $CMD5; exec bash'" &
    xfce4-terminal --title="Worker 6 [VALGRIND]" --hold --command="bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker6-est_${TIMESTAMP}.log $CMD6; exec bash'" &
elif command -v gnome-terminal &> /dev/null; then
    gnome-terminal --title="Worker 1 [VALGRIND]" -- bash -c "valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker1-est_${TIMESTAMP}.log $CMD1; exec bash" &
    gnome-terminal --title="Worker 2 [VALGRIND]" -- bash -c "valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker2-est_${TIMESTAMP}.log $CMD2; exec bash" &
    gnome-terminal --title="Worker 3 [VALGRIND]" -- bash -c "valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker3-est_${TIMESTAMP}.log $CMD3; exec bash" &
    gnome-terminal --title="Worker 4 [VALGRIND]" -- bash -c "valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker4-est_${TIMESTAMP}.log $CMD4; exec bash" &
    gnome-terminal --title="Worker 5 [VALGRIND]" -- bash -c "valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker5-est_${TIMESTAMP}.log $CMD5; exec bash" &
    gnome-terminal --title="Worker 6 [VALGRIND]" -- bash -c "valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker6-est_${TIMESTAMP}.log $CMD6; exec bash" &
else
    xterm -hold -e "bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker1-est_${TIMESTAMP}.log $CMD1; exec bash'" &
    xterm -hold -e "bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker2-est_${TIMESTAMP}.log $CMD2; exec bash'" &
    xterm -hold -e "bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker3-est_${TIMESTAMP}.log $CMD3; exec bash'" &
    xterm -hold -e "bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker4-est_${TIMESTAMP}.log $CMD4; exec bash'" &
    xterm -hold -e "bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker5-est_${TIMESTAMP}.log $CMD5; exec bash'" &
    xterm -hold -e "bash -c 'valgrind $VALGRIND_OPTS --log-file=../../valgrind-logs/worker6-est_${TIMESTAMP}.log $CMD6; exec bash'" &
fi

sleep 2

# Lanzar Query Control con valgrind
cd ../../query_control/scripts
BIN="../bin/query_control"
CONFIG="../configs/query.config"
PRIORITY=20

echo "Lanzando 25 instancias de cada Query con valgrind (Total: 100 procesos)..."
echo "NOTA: La ejecución será más lenta debido a valgrind"

for i in {1..25}
do
    valgrind --leak-check=full --log-file=../../valgrind-logs/query-control-aging1_${TIMESTAMP}_${i}.log $BIN $CONFIG AGING_1 $PRIORITY > /dev/null 2>&1 &
    valgrind --leak-check=full --log-file=../../valgrind-logs/query-control-aging2_${TIMESTAMP}_${i}.log $BIN $CONFIG AGING_2 $PRIORITY > /dev/null 2>&1 &
    valgrind --leak-check=full --log-file=../../valgrind-logs/query-control-aging3_${TIMESTAMP}_${i}.log $BIN $CONFIG AGING_3 $PRIORITY > /dev/null 2>&1 &
    valgrind --leak-check=full --log-file=../../valgrind-logs/query-control-aging4_${TIMESTAMP}_${i}.log $BIN $CONFIG AGING_4 $PRIORITY > /dev/null 2>&1 &
done

cd ../../scripts

echo ""
echo "=========================================="
echo "Prueba lanzada con valgrind"
echo "Timestamp: $TIMESTAMP"
echo "=========================================="
echo ""
echo "Los logs de valgrind se encuentran en: $VALGRIND_LOGS_DIR"
echo ""
echo "Para ver un resumen de memory leaks después de finalizar, ejecuta:"
echo "  grep 'LEAK SUMMARY' $VALGRIND_LOGS_DIR/*_${TIMESTAMP}.log"
echo ""
echo "Para ver todos los errores detectados:"
echo "  grep 'ERROR SUMMARY' $VALGRIND_LOGS_DIR/*_${TIMESTAMP}.log"
echo ""
