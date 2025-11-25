#ifndef _BLOQUES_H_
#define _BLOQUES_H_

#include <stdbool.h>
#include <utilsStorage/auxiliares.h>
#include <sys/mman.h>
#include <commons/bitarray.h>
#include <commons/crypto.h>
#include <utils/paquete.h>

// TODO: Implementar con mutex --> Directamente en las funciones que usan el bitmap
extern t_bitarray* bitmap; // Bitmap de bloques físicos

/**
 * @brief Busca el contenido de un bloque físico dado su número.
 * 
 * @param numeroBloqueFisico Número del bloque físico a buscar.
 * @return Un puntero a una cadena de caracteres que es la ruta del bloque físico.
 *         Es responsabilidad del llamador liberar la memoria asignada.
 */
char* buscarBloqueFisico(int numeroBloqueFisico);

/**
 * @brief Crea un nombre único para un bloque basado en su número.
 * 
 * @param nroBloque Número del bloque para generar el nombre.
 * @return Un puntero a una cadena de caracteres que contiene el nombre del bloque.
 *         Es responsabilidad del llamador liberar la memoria asignada.
 */
char* crearNombreBloque(int nroBloque);

/**
 * @brief Verifica si un bloque físico es válido.
 * 
 * @param numeroBloqueFisico Número del bloque físico a verificar.
 * @return true si el bloque físico es válido, false en caso contrario.
 */
bool bloqueFisicoValido(int numeroBloqueFisico);

/**
 * @brief Escribe datos en un bloque físico.
 * 
 * @param numeroBloqueFisico Número del bloque físico donde se escribirán los datos.
 * @param datos Puntero a los datos que se desean escribir.
 * @param sizeDatos Tamaño de los datos a escribir.
 * @param agregar Indica si los datos deben agregarse al contenido existente (true) 
 *                o sobrescribirlo (false).
 * @return true si la operación fue exitosa, false en caso de error.
 */
bool escribirBloqueFisico(int numeroBloqueFisico, void* datos, size_t sizeDatos, bool agregar);

/**
 * @brief Escribe el bloque inicial del sistema de almacenamiento.
 * 
 * @return true si la operación fue exitosa, false en caso de error.
 */
bool escribirBloqueInicial();

/**
 * @brief Obtiene la ruta del archivo correspondiente a un bloque físico.
 * 
 * @param nroBloqueFisico Número del bloque físico.
 * @return Un puntero a una cadena de caracteres que contiene la ruta del bloque físico.
 *         La memoria debe ser liberada por el llamador.
 */
char* rutaBloqueFisico(int nroBloqueFisico);

/**
 * @brief Inicializa el bitmap utilizado para gestionar los bloques físicos.
 *
 * Configura el bitmap en memoria, estableciendo todos los bloques como libres o según el estado inicial.
 * Debe llamarse una vez al inicio del sistema con el bitmap ya creado.
 */
void inicializarBitmap();

/**
 * @brief Carga el estado del bitmap desde almacenamiento persistente.
 *
 * Lee el bitmap desde disco y lo carga en memoria, reflejando el estado actual de los bloques físicos.
 * Debe llamarse durante la inicialización del sistema de archivos.
 */
void cargarBitmap();

/**
 * @brief Crea un hardlink entre un bloque lógico y un bloque físico.
 * 
 * @param rutaBloqueLogico Ruta de la carpeta de logical_blocks de ese File-Tag.
 * @param numeroBloqueLogico Número del bloque lógico.
 * @param numeroBloqueFisico Número del bloque físico al que se enlazará.
 * @return true si el hardlink fue creado exitosamente, false en caso de error.
 */
bool crearHardlink(char* rutaBloqueLogico, int numeroBloqueLogico, int numeroBloqueFisico);

/**
 * @brief Obtiene un bloque físico libre y lo marca como usado en el bitmap.
 * 
 * @return El número del bloque físico libre, o -1 si no hay bloques libres disponibles.
 */
int obtenerBloqueLibre();

/**
 * @brief Cuenta la cantidad de hardlinks asociados a un archivo.
 * 
 * @param path Ruta del archivo cuyo número de hardlinks se desea contar.
 * @return El número de hardlinks, o -1 en caso de error.
 */
int contarHardlinks(const char *path);

/**
 * @brief Verifica si un archivo tiene un único hardlink.
 * 
 * @param path Ruta del archivo a verificar.
 * @return true si el archivo tiene un único hardlink, false en caso contrario o error.
 */
bool esHardlinkUnico(const char *path);

/**
 * @brief Obtiene el contenido de un bloque físico.
 * 
 * @param numeroBloqueFisico Numero del bloque físico.
 * @return El contenido leido o NULL si hubo un error.
 */
char* leerBloqueFisico(int numeroBloqueFisico);

/**
 * @brief Obtiene el número de bloque físico asociado a un nombre de bloque.
 * 
 * @param nombreBloque Nombre del bloque.
 * @return El número del bloque físico, o -1 si no se encuentra.
 */
int obtenerNumeroBloqueFisico(const char* nombreBloque);

/**
 * @brief Actualiza el bloque lógico en la metadata del FileTag.
 * 
 * @param metadata Puntero a la metadata del FileTag.
 * @param nroBloqueLogico Número del bloque lógico a actualizar.
 * @param nroBloqueFisico Nuevo número del bloque físico a asociar.
 * @return true si la actualización fue exitosa, false en caso de error.
 */
bool actualizarBloqueFileTag(t_config* metadata, int nroBloqueLogico, int nroBloqueFisico);

#endif /* _BLOQUES_H_ */