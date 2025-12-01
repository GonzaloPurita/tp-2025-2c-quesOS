# Query control

Es el que se encarga de enviar a ejecutar una Query, se la manda al Master (Kernel).

- [Query control](#query-control)
  - [Query](#query)
  - [Con que inicia](#con-que-inicia)
  - [Funcionamiento](#funcionamiento)

## Query

Es una archivo con una serie de intrucciones referente al manejo de archivos.

## Con que inicia

Tenemos que respetar este orden.

1. Archivo de configuración: Supongo que se lo mandamos para que puedan haber varios.
2. Archivo query: Es la ruta al archivo con las instrucciones.
3. Prioridad: Valor numérico que indica la prioridad.

## Funcionamiento

1. Se conecta al módulo Master (Kernel).
2. Handshake.
3. Envía `path` y `prioridad`.
4. Esperar mensajes recibidos. Esperamos:
   1. Una lectura de archivo. Tenemos que mostrar:  
      1. FILE
      2. CONTENIDO
   2. Finalización de la QUERY. Debemos inlcuir:
      1. MOTIVO.