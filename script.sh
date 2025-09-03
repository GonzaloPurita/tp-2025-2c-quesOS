#!/bin/bash

# Compilo los modulos
cd ./master
make clean
make
cd ../storage
make clean
make
cd ../query_control
make clean
make
cd ../worker
make clean
make

# Levantar Master
xfce4-terminal -- bash -c "./master/bin/master; exec bash" &

# Levantar Storage
xfce4-terminal -- bash -c "./storage/bin/storage; exec bash" &

# Levantar Query Control
xfce4-terminal -- bash -c "./query_control/bin/query_control query.config queries/query1.q 1; exec bash" &

# Levantar Worker
xfce4-terminal -- bash -c "./worker/bin/worker worker.config 1; exec bash" &

