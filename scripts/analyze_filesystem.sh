#!/bin/bash

# ========================================
# Análisis del Filesystem - TP Master of Files
# ========================================
# Script para analizar el estado completo del filesystem
# en /home/utnso/storage

STORAGE_PATH="/home/utnso/storage"
BLOCK_SIZE=16  # Tamaño de bloque por defecto

# Detectar si la salida es a terminal o a archivo
if [ -t 1 ]; then
    # Salida a terminal - usar colores
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    CYAN='\033[0;36m'
    MAGENTA='\033[0;35m'
    NC='\033[0m' # No Color
    BOLD='\033[1m'
else
    # Salida redirigida a archivo - sin colores
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    CYAN=''
    MAGENTA=''
    NC=''
    BOLD=''
fi

echo -e "${BOLD}========================================"
echo "ANÁLISIS DEL FILESYSTEM - TP 2025 2C"
echo -e "========================================${NC}"
echo "Punto de montaje: $STORAGE_PATH"
echo ""

# Verificar argumentos de línea de comandos
ANALYSIS_OPTION=""
if [ "$1" = "--full" ] || [ "$1" = "-f" ]; then
    ANALYSIS_OPTION=1
elif [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "Uso: $0 [OPCIONES]"
    echo ""
    echo "Opciones:"
    echo "  -f, --full        Análisis completo (sin menú interactivo)"
    echo "  -s, --superblock  Solo superblock y archivos"
    echo "  -t, --tags        Solo tags y metadata"
    echo "  -b, --blocks      Solo bloques físicos"
    echo "  -h, --help        Mostrar esta ayuda"
    echo ""
    echo "Si no se especifica opción, se mostrará el menú interactivo"
    exit 0
elif [ "$1" = "--superblock" ] || [ "$1" = "-s" ]; then
    ANALYSIS_OPTION=2
elif [ "$1" = "--tags" ] || [ "$1" = "-t" ]; then
    ANALYSIS_OPTION=3
elif [ "$1" = "--blocks" ] || [ "$1" = "-b" ]; then
    ANALYSIS_OPTION=4
fi

# Verificar que el directorio existe
if [ ! -d "$STORAGE_PATH" ]; then
    echo -e "${RED}ERROR: El directorio $STORAGE_PATH no existe${NC}"
    exit 1
fi

# ========================================
# FUNCIÓN: Leer y parsear superblock
# ========================================
read_superblock() {
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo -e "${BOLD}${BLUE}1. CONFIGURACIÓN DEL SUPERBLOCK${NC}"
    echo -e "${BOLD}${BLUE}========================================${NC}"

    if [ -f "$STORAGE_PATH/superblock.config" ]; then
        echo -e "${GREEN}[OK]${NC} Archivo superblock.config encontrado"
        echo ""
        cat "$STORAGE_PATH/superblock.config"
        echo ""

        # Extraer valores
        FS_SIZE=$(grep "^FS_SIZE=" "$STORAGE_PATH/superblock.config" | cut -d'=' -f2)
        BLOCK_SIZE=$(grep "^BLOCK_SIZE=" "$STORAGE_PATH/superblock.config" | cut -d'=' -f2)

        if [ -n "$FS_SIZE" ] && [ -n "$BLOCK_SIZE" ]; then
            TOTAL_BLOCKS=$((FS_SIZE / BLOCK_SIZE))
            echo -e "${CYAN}Tamaño del filesystem:${NC} $FS_SIZE bytes"
            echo -e "${CYAN}Tamaño de bloque:${NC} $BLOCK_SIZE bytes"
            echo -e "${CYAN}Total de bloques disponibles:${NC} $TOTAL_BLOCKS bloques"
        fi
    else
        echo -e "${YELLOW}[!]${NC} Archivo superblock.config no encontrado"
    fi
    echo ""
}

# ========================================
# FUNCIÓN: Listar archivos base
# ========================================
list_base_files() {
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo -e "${BOLD}${BLUE}2. ARCHIVOS BASE${NC}"
    echo -e "${BOLD}${BLUE}========================================${NC}"

    local file_count=0

    # Buscar en el directorio files si existe
    local files_dir="$STORAGE_PATH/files"

    if [ -d "$files_dir" ]; then
        # Buscar directorios que representen archivos dentro de /files
        for file_dir in "$files_dir"/*; do
            if [ -d "$file_dir" ]; then
                local basename=$(basename "$file_dir")

                # Ignorar directorios especiales
                if [[ "$basename" != "." && "$basename" != ".." ]]; then
                    file_count=$((file_count + 1))
                    echo -e "${GREEN}[FILE] Archivo $file_count:${NC} ${BOLD}$basename${NC}"

                    # Contar tags asociados
                    local tag_count=$(find "$file_dir" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l)
                    echo -e "   ${CYAN}Tags asociados:${NC} $tag_count"

                    # Listar nombres de tags
                    if [ $tag_count -gt 0 ]; then
                        echo -e "   ${CYAN}Tags:${NC}"
                        find "$file_dir" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | while read tag_dir; do
                            local tag_name=$(basename "$tag_dir")
                            echo -e "      - $tag_name"
                        done
                    fi

                    echo ""
                fi
            fi
        done
    else
        # Fallback: buscar en la raíz
        for file_dir in "$STORAGE_PATH"/*; do
            if [ -d "$file_dir" ]; then
                local basename=$(basename "$file_dir")

                # Ignorar directorios especiales
                if [[ "$basename" != "." && "$basename" != ".." && "$basename" != "superblock.config" && "$basename" != "files" && "$basename" != "physical_blocks" ]]; then
                    file_count=$((file_count + 1))
                    echo -e "${GREEN}[FILE] Archivo $file_count:${NC} ${BOLD}$basename${NC}"

                    # Contar tags asociados
                    local tag_count=$(find "$file_dir" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l)
                    echo -e "   ${CYAN}Tags asociados:${NC} $tag_count"

                    echo ""
                fi
            fi
        done
    fi

    if [ $file_count -eq 0 ]; then
        echo -e "${YELLOW}[!]${NC} No se encontraron archivos base"
    else
        echo -e "${GREEN}Total de archivos base: $file_count${NC}"
    fi
    echo ""
}

# ========================================
# FUNCIÓN: Listar tags y su metadata
# ========================================
list_tags_and_metadata() {
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo -e "${BOLD}${BLUE}3. TAGS Y METADATA${NC}"
    echo -e "${BOLD}${BLUE}========================================${NC}"

    local total_tags=0

    # Buscar en el directorio files si existe
    local files_dir="$STORAGE_PATH/files"

    if [ -d "$files_dir" ]; then
        # Recorrer cada archivo base en /files
        for file_dir in "$files_dir"/*; do
            if [ -d "$file_dir" ]; then
                local file_basename=$(basename "$file_dir")

                if [[ "$file_basename" != "." && "$file_basename" != ".." ]]; then
                    # Buscar tags dentro del archivo
                    for tag_dir in "$file_dir"/*; do
                        if [ -d "$tag_dir" ]; then
                            local tagname=$(basename "$tag_dir")
                            total_tags=$((total_tags + 1))

                            echo -e "${MAGENTA}[TAG] Tag $total_tags:${NC} ${BOLD}$file_basename:$tagname${NC}"

                            # Leer metadata del tag
                            if [ -f "$tag_dir/metadata.config" ]; then
                                echo -e "   ${CYAN}[META] Metadata:${NC}"

                                # Parsear y mostrar metadata de forma legible
                                local size=$(grep "^TAMAÑO=" "$tag_dir/metadata.config" 2>/dev/null | cut -d'=' -f2)
                                local estado=$(grep "^ESTADO=" "$tag_dir/metadata.config" 2>/dev/null | cut -d'=' -f2)
                                local blocks=$(grep "^BLOCKS=" "$tag_dir/metadata.config" 2>/dev/null | cut -d'=' -f2)

                                if [ -n "$size" ]; then
                                    echo -e "      ${CYAN}Tamaño:${NC} $size bytes"
                                fi

                                if [ -n "$estado" ]; then
                                    if [ "$estado" = "COMMITED" ]; then
                                        echo -e "      ${CYAN}Estado:${NC} ${GREEN}$estado${NC}"
                                    else
                                        echo -e "      ${CYAN}Estado:${NC} ${YELLOW}$estado${NC}"
                                    fi
                                fi

                                if [ -n "$blocks" ]; then
                                    # Limpiar el formato [2,1,1,1] -> 2 1 1 1
                                    local blocks_clean=$(echo "$blocks" | tr -d '[]' | tr ',' ' ')
                                    local block_array=($blocks_clean)
                                    local block_count=${#block_array[@]}

                                    echo -e "      ${CYAN}Bloques físicos asignados:${NC} $blocks"
                                    echo -e "      ${YELLOW}Mapeo lógico → físico:${NC}"

                                    local idx=0
                                    for physical in $blocks_clean; do
                                        echo -e "         Bloque lógico $idx → Bloque físico $physical"
                                        idx=$((idx + 1))
                                    done

                                    echo -e "      ${CYAN}Total de bloques:${NC} $block_count"
                                else
                                    # Si no hay formato específico, mostrar toda la metadata
                                    echo -e "      ${CYAN}Contenido completo:${NC}"
                                    sed 's/^/         /' "$tag_dir/metadata.config"
                                fi
                            else
                                echo -e "   ${YELLOW}[!]${NC} No se encontró metadata.config"
                            fi

                            # Buscar directorio de bloques lógicos
                            if [ -d "$tag_dir/logical_blocks" ]; then
                                local block_count=$(find "$tag_dir/logical_blocks" -name "*.dat" 2>/dev/null | wc -l)
                                echo -e "   ${CYAN}[BLOCK] Bloques lógicos:${NC} $block_count archivos .dat"
                            fi

                            echo ""
                        fi
                    done
                fi
            fi
        done
    else
        # Fallback: buscar en estructura antigua
        for file_dir in "$STORAGE_PATH"/*; do
            if [ -d "$file_dir" ]; then
                local basename=$(basename "$file_dir")

                if [[ "$basename" != "." && "$basename" != ".." ]]; then
                    local file_tags=0

                    # Buscar tags dentro del archivo
                    for tag_dir in "$file_dir"/*; do
                        if [ -d "$tag_dir" ]; then
                            local tagname=$(basename "$tag_dir")
                            file_tags=$((file_tags + 1))
                            total_tags=$((total_tags + 1))

                            echo -e "${MAGENTA}[TAG] Tag $total_tags:${NC} ${BOLD}$basename:$tagname${NC}"

                            # Leer metadata del tag
                            if [ -f "$tag_dir/metadata.config" ]; then
                                echo -e "   ${CYAN}[META] Metadata encontrada${NC}"
                                sed 's/^/      /' "$tag_dir/metadata.config"
                            else
                                echo -e "   ${YELLOW}[!]${NC} No se encontró metadata.config"
                            fi

                            echo ""
                        fi
                    done
                fi
            fi
        done
    fi

    if [ $total_tags -eq 0 ]; then
        echo -e "${YELLOW}[!]${NC} No se encontraron tags"
    else
        echo -e "${GREEN}Total de tags: $total_tags${NC}"
    fi
    echo ""
}

# ========================================
# FUNCIÓN: Listar bloques físicos
# ========================================
list_physical_blocks() {
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo -e "${BOLD}${BLUE}4. BLOQUES FÍSICOS${NC}"
    echo -e "${BOLD}${BLUE}========================================${NC}"

    # Buscar bloques lógicos en la estructura real
    echo ""

    # Buscar directorios logical_blocks
    local logical_blocks_dirs=$(find "$STORAGE_PATH" -type d -name "logical_blocks" 2>/dev/null)

    if [ -n "$logical_blocks_dirs" ]; then
        echo -e "${GREEN}[OK]${NC} Directorios de bloques lógicos encontrados"
        echo ""

        local total_blocks=0

        # Recorrer cada directorio de bloques
        echo "$logical_blocks_dirs" | while read blocks_dir; do
            # Obtener el contexto (archivo:tag)
            local parent_dir=$(dirname "$blocks_dir")
            local tag_name=$(basename "$parent_dir")
            local file_name=$(basename $(dirname "$parent_dir"))

            echo -e "${YELLOW}[DIR] Bloques de:${NC} ${BOLD}$file_name:$tag_name${NC}"

            # Listar bloques en este directorio
            local block_files=$(find "$blocks_dir" -type f -name "*.dat" 2>/dev/null | sort -V)
            local block_count=$(echo "$block_files" | grep -c "\.dat$" 2>/dev/null || echo 0)

            if [ $block_count -gt 0 ]; then
                echo -e "   ${CYAN}Total de bloques:${NC} $block_count"
                echo ""

                local display_count=0
                echo "$block_files" | while read block_file; do
                    if [ -f "$block_file" ]; then
                        display_count=$((display_count + 1))
                        local block_num=$(basename "$block_file" .dat)
                        local block_size=$(stat -c%s "$block_file" 2>/dev/null)

                        echo -e "   ${YELLOW}[BLOCK] Bloque lógico:${NC} ${BOLD}$block_num${NC} (${block_size} bytes)"

                        # Mostrar contenido del bloque de forma legible
                        echo -e "      ${CYAN}Hexadecimal:${NC}"
                        hexdump -C "$block_file" 2>/dev/null | head -2 | sed 's/^/         /'

                        # Intentar mostrar contenido como texto
                        echo -e "      ${CYAN}Texto:${NC}"
                        if [ -r "$block_file" ]; then
                            local text_content=$(cat "$block_file" 2>/dev/null | tr -d '\0' | strings)
                            if [ -n "$text_content" ]; then
                                echo "         \"$text_content\""
                            else
                                local raw_content=$(cat "$block_file" 2>/dev/null | head -c 50)
                                if [ -n "$raw_content" ]; then
                                    echo "         (contenido binario)"
                                else
                                    echo "         (vacío)"
                                fi
                            fi
                        fi

                        echo ""

                        # Limitar la salida para no saturar
                        if [ $display_count -ge 10 ]; then
                            local remaining=$((block_count - 10))
                            if [ $remaining -gt 0 ]; then
                                echo -e "      ${YELLOW}... y $remaining bloques más en este tag${NC}"
                            fi
                            break
                        fi
                    fi
                done

                echo ""
            fi
        done
    else
        echo -e "${YELLOW}[!]${NC} No se encontraron directorios logical_blocks"
        echo "Buscando bloques .dat en toda la estructura..."

        local all_blocks=$(find "$STORAGE_PATH" -type f -name "*.dat" 2>/dev/null | sort)
        local block_count=$(echo "$all_blocks" | grep -c "\.dat$" 2>/dev/null || echo 0)

        if [ $block_count -gt 0 ]; then
            echo ""
            echo -e "${CYAN}Bloques .dat encontrados: $block_count${NC}"
            echo ""

            local display_count=0
            echo "$all_blocks" | while read block_file; do
                if [ -f "$block_file" ]; then
                    display_count=$((display_count + 1))
                    local block_path=$(echo "$block_file" | sed "s|$STORAGE_PATH/||")
                    local block_size=$(stat -c%s "$block_file" 2>/dev/null)

                    echo -e "   ${YELLOW}[BLOCK]${NC} $block_path (${block_size} bytes)"

                    # Limitar salida
                    if [ $display_count -ge 20 ]; then
                        local remaining=$((block_count - 20))
                        if [ $remaining -gt 0 ]; then
                            echo -e "   ${YELLOW}... y $remaining bloques más${NC}"
                        fi
                        break
                    fi
                fi
            done
        else
            echo -e "${YELLOW}[!]${NC} No se encontraron bloques .dat"
        fi
    fi
    echo ""
}

# ========================================
# FUNCIÓN: Análisis de deduplicación
# ========================================
analyze_deduplication() {
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo -e "${BOLD}${BLUE}5. ANÁLISIS DE DEDUPLICACIÓN${NC}"
    echo -e "${BOLD}${BLUE}========================================${NC}"

    echo ""

    # Buscar todos los bloques .dat
    local all_blocks=$(find "$STORAGE_PATH" -type f -name "*.dat" 2>/dev/null)

    if [ -z "$all_blocks" ]; then
        echo -e "${YELLOW}[!]${NC} No se encontraron bloques para analizar"
        echo ""
        return
    fi

    # Crear mapa temporal de hashes
    local temp_file=$(mktemp)

    # Calcular hash de cada bloque
    echo "$all_blocks" | while read block_file; do
        if [ -f "$block_file" ]; then
            local hash=$(md5sum "$block_file" 2>/dev/null | cut -d' ' -f1)
            local block_path=$(echo "$block_file" | sed "s|$STORAGE_PATH/||")
            echo "$hash:$block_path" >> "$temp_file"
        fi
    done

    # Buscar duplicados
    local duplicates_found=0
    local unique_hashes=$(cat "$temp_file" | cut -d':' -f1 | sort -u)

    echo "$unique_hashes" | while read hash; do
        local blocks_with_hash=$(grep "^$hash:" "$temp_file" | cut -d':' -f2-)
        local count=$(echo "$blocks_with_hash" | wc -l)

        if [ $count -gt 1 ]; then
            duplicates_found=$((duplicates_found + 1))
            echo -e "${GREEN}[OK] Deduplicación detectada:${NC} $count bloques con contenido idéntico"
            echo -e "   ${CYAN}Hash MD5:${NC} $hash"
            echo "$blocks_with_hash" | while read block_path; do
                echo -e "      - $block_path"
            done
            echo ""
        fi
    done

    # Limpiar archivo temporal
    rm -f "$temp_file"

    # Estadísticas de deduplicación
    local total_blocks=$(echo "$all_blocks" | wc -l)
    local unique_count=$(echo "$unique_hashes" | wc -l)
    local duplicated=$((total_blocks - unique_count))

    echo -e "${CYAN}Estadísticas de deduplicación:${NC}"
    echo "   Total de bloques: $total_blocks"
    echo "   Bloques únicos: $unique_count"
    echo "   Bloques deduplicados: $duplicated"

    if [ $total_blocks -gt 0 ]; then
        local dedup_percent=$((duplicated * 100 / total_blocks))
        echo "   Ratio de deduplicación: ${dedup_percent}%"
    fi

    echo ""
}

# ========================================
# FUNCIÓN: Estadísticas generales
# ========================================
show_statistics() {
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo -e "${BOLD}${BLUE}6. ESTADÍSTICAS GENERALES${NC}"
    echo -e "${BOLD}${BLUE}========================================${NC}"

    echo -e "${CYAN}Uso de espacio en disco:${NC}"
    du -h "$STORAGE_PATH" 2>/dev/null | tail -1
    echo ""

    echo -e "${CYAN}Distribución de archivos por tipo:${NC}"
    local total_files=$(find "$STORAGE_PATH" -type f 2>/dev/null | wc -l)
    local total_dirs=$(find "$STORAGE_PATH" -type d 2>/dev/null | wc -l)
    echo "   Total de archivos: $total_files"
    echo "   Total de directorios: $total_dirs"
    echo ""

    echo -e "${CYAN}Archivos más grandes:${NC}"
    find "$STORAGE_PATH" -type f -exec ls -lh {} \; 2>/dev/null | \
        sort -k5 -h -r | head -5 | \
        awk '{print "   " $9 " (" $5 ")"}'
    echo ""

    if [ -n "$FS_SIZE" ] && [ -n "$BLOCK_SIZE" ]; then
        local used_space=$(du -sb "$STORAGE_PATH" 2>/dev/null | cut -f1)
        local usage_percent=$((used_space * 100 / FS_SIZE))
        echo -e "${CYAN}Uso del filesystem:${NC} $used_space / $FS_SIZE bytes (${usage_percent}%)"

        # Barra de progreso visual
        local bar_length=50
        local filled_length=$((usage_percent * bar_length / 100))
        local bar=""
        for i in $(seq 1 $filled_length); do bar="${bar}█"; done
        for i in $(seq $filled_length $bar_length); do bar="${bar}░"; done
        echo "   [$bar] ${usage_percent}%"
    fi
    echo ""
}

# ========================================
# FUNCIÓN: Exportar estructura en formato árbol
# ========================================
show_tree_structure() {
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo -e "${BOLD}${BLUE}7. ESTRUCTURA EN ÁRBOL${NC}"
    echo -e "${BOLD}${BLUE}========================================${NC}"

    if command -v tree >/dev/null 2>&1; then
        tree -L 3 "$STORAGE_PATH" 2>/dev/null
    else
        echo -e "${YELLOW}[!]${NC} Comando 'tree' no disponible. Mostrando estructura básica:"
        echo ""
        find "$STORAGE_PATH" -maxdepth 3 2>/dev/null | sed 's|[^/]*/| |g'
    fi
    echo ""
}

# ========================================
# FUNCIÓN: Validar integridad
# ========================================
validate_integrity() {
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo -e "${BOLD}${BLUE}8. VALIDACIÓN DE INTEGRIDAD${NC}"
    echo -e "${BOLD}${BLUE}========================================${NC}"

    local errors=0

    # Verificar que todos los tags tengan metadata

    local files_dir="$STORAGE_PATH/files"

    if [ -d "$files_dir" ]; then
        for file_dir in "$files_dir"/*; do
            if [ -d "$file_dir" ]; then
                for tag_dir in "$file_dir"/*; do
                    if [ -d "$tag_dir" ]; then
                        local file_name=$(basename "$file_dir")
                        local tag_name=$(basename "$tag_dir")
                        if [ ! -f "$tag_dir/metadata.config" ]; then
                            echo -e "   ${RED}[X]${NC} Tag $file_name:$tag_name no tiene metadata.config"
                            errors=$((errors + 1))
                        fi
                    fi
                done
            fi
        done
    else
        # Fallback para estructura antigua
        for file_dir in "$STORAGE_PATH"/*; do
            if [ -d "$file_dir" ]; then
                for tag_dir in "$file_dir"/*; do
                    if [ -d "$tag_dir" ]; then
                        if [ ! -f "$tag_dir/metadata.config" ]; then
                            echo -e "   ${RED}[X]${NC} Tag $(basename "$tag_dir") no tiene metadata.config"
                            errors=$((errors + 1))
                        fi
                    fi
                done
            fi
        done
    fi

    if [ $errors -eq 0 ]; then
        echo -e "   ${GREEN}[OK]${NC} Todos los tags tienen metadata"
    fi
    echo ""

    # Verificar permisos
    if [ -r "$STORAGE_PATH" ] && [ -w "$STORAGE_PATH" ]; then
        echo -e "   ${GREEN}[OK]${NC} Permisos de lectura/escritura correctos"
    else
        echo -e "   ${RED}[X]${NC} Problemas de permisos detectados"
        errors=$((errors + 1))
    fi
    echo ""

    if [ $errors -eq 0 ]; then
        echo -e "${GREEN}[OK] Integridad validada correctamente${NC}"
    else
        echo -e "${YELLOW}[!] Se detectaron $errors problemas de integridad${NC}"
    fi
    echo ""
}

# ========================================
# EJECUCIÓN PRINCIPAL
# ========================================

# Si ya se definió la opción por línea de comandos, usarla
if [ -z "$ANALYSIS_OPTION" ]; then
    # Menú de opciones solo si no hay argumento de línea de comandos
    if [ -t 0 ]; then
        # Solo mostrar menú si hay entrada interactiva (no stdin redirigido)
        echo "Opciones de análisis:"
        echo "  1. Análisis completo (recomendado)"
        echo "  2. Solo superblock y archivos base"
        echo "  3. Solo tags y metadata"
        echo "  4. Solo bloques físicos"
        echo "  5. Análisis personalizado"
        echo ""
        echo -n "Selecciona opción [1-5] (Enter = opción 1): "
        read ANALYSIS_OPTION

        if [ -z "$ANALYSIS_OPTION" ]; then
            ANALYSIS_OPTION=1
        fi
    else
        # Si stdin está redirigido, usar análisis completo por defecto
        ANALYSIS_OPTION=1
    fi
fi

echo ""

case $ANALYSIS_OPTION in
    1)
        echo ""
        read_superblock
        list_base_files
        list_tags_and_metadata
        list_physical_blocks
        analyze_deduplication
        show_statistics
        show_tree_structure
        validate_integrity
        ;;
    2)
        read_superblock
        list_base_files
        ;;
    3)
        list_tags_and_metadata
        ;;
    4)
        list_physical_blocks
        ;;
    5)
        echo "Selecciona los análisis que deseas ejecutar:"
        echo -n "  Superblock? (y/n): "; read do_sb
        echo -n "  Archivos base? (y/n): "; read do_files
        echo -n "  Tags y metadata? (y/n): "; read do_tags
        echo -n "  Bloques físicos? (y/n): "; read do_blocks
        echo -n "  Deduplicación? (y/n): "; read do_dedup
        echo -n "  Estadísticas? (y/n): "; read do_stats
        echo -n "  Estructura árbol? (y/n): "; read do_tree
        echo -n "  Validación integridad? (y/n): "; read do_valid
        echo ""

        [[ "$do_sb" =~ ^[Yy]$ ]] && read_superblock
        [[ "$do_files" =~ ^[Yy]$ ]] && list_base_files
        [[ "$do_tags" =~ ^[Yy]$ ]] && list_tags_and_metadata
        [[ "$do_blocks" =~ ^[Yy]$ ]] && list_physical_blocks
        [[ "$do_dedup" =~ ^[Yy]$ ]] && analyze_deduplication
        [[ "$do_stats" =~ ^[Yy]$ ]] && show_statistics
        [[ "$do_tree" =~ ^[Yy]$ ]] && show_tree_structure
        [[ "$do_valid" =~ ^[Yy]$ ]] && validate_integrity
        ;;
    *)
        echo -e "${RED}Opción inválida${NC}"
        exit 1
        ;;
esac

echo -e "${BOLD}${GREEN}========================================${NC}"
echo -e "${BOLD}${GREEN}ANÁLISIS COMPLETADO${NC}"
echo -e "${BOLD}${GREEN}========================================${NC}"
echo ""
echo "Para guardar este análisis en un archivo:"
echo "  ./analyze_filesystem.sh --full > filesystem_report.txt"
echo ""
echo "Otras opciones útiles:"
echo "  ./analyze_filesystem.sh --help        # Ver ayuda"
echo "  ./analyze_filesystem.sh --tags        # Solo analizar tags"
echo "  ./analyze_filesystem.sh --blocks      # Solo analizar bloques"
echo ""
