# **Guía de Pruebas entre Worker y Storage (Master of Files)**

Esta guía detalla los pasos para probar el flujo entre el módulo **Worker** y el **Storage** dentro del TP **Master of Files**, usando un **mock de Storage** hecho por tu compañero.  
 El objetivo es validar que el Worker cumple con el protocolo, maneja correctamente la memoria y realiza las operaciones esperadas sobre el Storage.

---

## **1️ Qué debe hacer el mock de Storage**

Para que las pruebas sean útiles, el mock debe:

* Aceptar **conexiones TCP** del Worker.

* Responder al **handshake** con la información necesaria (por ejemplo: `BLOCK_SIZE`, `RETARDO_OPERACION`, etc.).

Implementar las operaciones que el Worker solicita:

 `CREATE`  
`TRUNCATE`  
`READ BLOCK`  
`WRITE BLOCK`  
`TAG`  
`COMMIT`  
`FLUSH`  
`DELETE`

*   
* Simular **errores controlados**:

  * Archivo o tag inexistente.
  * Escritura no permitida.
  * Lectura fuera de rango.
  * Espacio insuficiente.

* Simular **retardos configurables**.
* Loguear las operaciones recibidas para verificar el intercambio.

---

## **2️ Flujo de pruebas sugerido**

### **A. Handshake / Conexión**

1. Iniciar el mock del Storage (por ejemplo con `BLOCK_SIZE = 128`).

2. Iniciar el Worker apuntando al mock.  
    **Esperado:** el Worker solicita el tamaño de bloque y loguea handshake exitoso.

---

### **B. Operaciones básicas**

Ejecutar Queries simples que prueben cada instrucción del protocolo:

1. `CREATE`
2. `TRUNCATE`
3. `WRITE`
4. `READ`
5. `FLUSH`
6. `COMMIT`
7. `TAG`
8. `DELETE`
9. `END`

**Verificar:**

* Que el Worker envía correctamente la petición al Storage.

* Que el mock responde y el Worker loguea la instrucción completada.

---

### **C. Prueba de memoria interna (Page Faults)**

* Configurar el Worker con poca memoria (`TAM_MEMORIA = 2 * BLOCK_SIZE`).

* Ejecutar una Query con más bloques de los que caben en memoria.

* **Esperar:**

  * Logs de `Memoria Miss` y `Memoria Add`.

  * Cuando se reemplace una página dirty, el Worker debe enviar `WRITE BLOCK` al Storage.

---

### **D. Flush / Commit / Deduplicación**

* Ejecutar instrucciones `FLUSH` y `COMMIT`.

* Verificar que el Worker:

  * Envia los bloques modificados al Storage.

  * Loguea `Bloque Físico Reservado`, `Bloque Físico Liberado` o `Hard Link Agregado` (según corresponda).

---

### **E. Manejo de errores**

Probar distintos escenarios:

| Caso | Simulación en mock | Resultado esperado en Worker |
| ----- | ----- | ----- |
| Lectura fuera de rango | Responder `READ_OUT_OF_BOUNDS` | Query finaliza con error y logea el motivo |
| Escritura no permitida | Responder `ESCRITURA_NO_PERMITIDA` | Query abortada con log de error |
| Espacio insuficiente | Responder `ESPACIO_INSUFICIENTE` | Log de error y Query finalizada |
| Desconexión del Storage | Cerrar socket desde el mock | Worker detecta desconexión y la maneja sin crash |

---

### **F. Concurrencia**

* Levantar dos Workers conectados al mismo Storage mock.

* Hacer operaciones simultáneas (`WRITE`, `COMMIT`, `DELETE`).

* Verificar que no haya corrupción de datos ni errores de protocolo.

---

### **G. End-to-End (opcional)**

Probar el flujo completo con los demás módulos:

`Query Control → Master → Worker → Storage`

y verificar que cada módulo genere los logs pedidos por el enunciado.

---

## **3️ Casos de prueba de ejemplo**

### ** Caso 1 — Handshake**

**Acción:** El Worker conecta y pide el tamaño de bloque.  
 **Mock responde:**

`OK BLOCK_SIZE=128`

**Esperado:** Log del Worker con handshake exitoso y `PAGE_SIZE=128`.

---

### ** Caso 2 — Flujo básico**

**Query de prueba:**

`CREATE MATERIAS:BASE`  
`TRUNCATE MATERIAS:BASE 1024`  
`WRITE MATERIAS:BASE 0 SISTEMAS_OPERATIVOS`  
`FLUSH MATERIAS:BASE`  
`COMMIT MATERIAS:BASE`  
`READ MATERIAS:BASE 0 8`  
`END`

**Mock:**

* Devuelve `OK` en todas las operaciones.

* En `READ` devuelve el mismo contenido que se escribió.

**Verificar:**

* Logs del Worker (`Instrucción realizada`, `Memoria Add`, `Memoria Miss`, `Acción: LEER`).

* Logs del Storage mock (`File Creado`, `Bloque Lógico Escrito`, `Commit`).

---

### ** Caso 3 — Reemplazo de páginas**

* Configurar `TAM_MEMORIA = 2 * BLOCK_SIZE`.

* Escribir tres bloques diferentes.

* **Esperar:** Log `Se reemplaza la página X` y que el Worker envíe `WRITE BLOCK` al mock.

---

### ** Caso 4 — Error controlado**

**Acción:** `READ EXISTENTE:TAG 100000 128`  
 **Mock responde:** `READ_OUT_OF_BOUNDS`  
 **Esperado:** Worker finaliza la Query con error y log correspondiente.

---

## **4️ Cómo testearlo en C**

1. **Configurar el mock Storage**

   * Storage debe levantarse como servidor TCP (`bind()`, `listen()`, `accept()`).

   * En cada conexión, responder el handshake y luego escuchar los mensajes del Worker.

2. **Ejecutar el Worker**

   * Configurar `IP_STORAGE` y `PUERTO_STORAGE` en el archivo de configuración del Worker.

   * Ejecutar Queries desde `PATH_QUERIES`.

3. **Verificar logs**

   * Worker y Storage deben guardar logs en archivos separados.

   * Validar los mensajes pedidos en el enunciado (por ejemplo: “Memoria Add”, “Query Finalizada \- Motivo”).

4. **Automatizar**

   * Podés crear un script bash para lanzar el mock, el Worker y validar los resultados.

---

## **5️ Checklist final antes de entregar**

* Handshake correcto (Worker recibe `BLOCK_SIZE`).
* Todas las instrucciones básicas funcionan.
* Logs generados según el enunciado.
* `Memoria Miss` y reemplazos correctamente logueados.
* `FLUSH` y `COMMIT` actualizan los bloques.
* Casos de error probados (lectura fuera de rango, etc.).
* Worker maneja desconexiones del Storage.
* Logs guardados con nivel `INFO` o superior.
