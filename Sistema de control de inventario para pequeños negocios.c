
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

/* ============================ Constantes ============================ */

#define MAX_NAME        64
#define MAX_CATEGORY    64
#define MAX_SUPPLIER    64
#define MAX_DATE        20
#define MAX_CODE        999999
#define MAX_QUANTITY    1000000
#define LOW_STOCK_THRESHOLD 10

/* ============================ Estructuras ============================ */

typedef struct {
    int codigo;
    char nombre[MAX_NAME];
    char categoria[MAX_CATEGORY];
    double precio;
    int cantidad;
    char proveedor[MAX_SUPPLIER];
} Product;

typedef struct {
    int id;
    int producto_codigo;
    int cantidad;
    double precio_total;
    char fecha[MAX_DATE];
} Sale;

typedef struct {
    Product *data;
    size_t count;
    size_t cap;
} ProductList;

typedef struct {
    Sale *data;
    size_t count;
    size_t cap;
} SaleList;

/* ============================ Utilidades de entrada ============================ */

/* Copia segura de cadenas */
void copy_str(char *dest, const char *src, size_t dest_size) {
    if (dest_size == 0) return;
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

/* Revisa si una cadena tiene contenido visible */
bool has_text(const char *s) {
    if (!s) return false;

    while (*s) {
        if (!isspace((unsigned char)*s)) return true;
        s++;
    }

    return false;
}

/* Lee una linea desde stdin */
bool read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    fflush(stdout);

    if (!fgets(buffer, (int)size, stdin)) {
        return false;
    }

    char *nl = strchr(buffer, '\n');
    if (nl) {
        *nl = '\0';
    } else {
        /* Si la linea supera el buffer, se descarta el resto */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
    }

    char *cr = strchr(buffer, '\r');
    if (cr) *cr = '\0';

    return true;
}

/* Quita espacios al inicio y al final */
char *trim_spaces(char *s) {
    if (!s) return s;

    while (isspace((unsigned char)*s)) s++;

    if (*s == '\0') return s;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    return s;
}

/* Deja la cadena sin espacios extremos en el mismo buffer */
void trim_inplace(char *s) {
    if (!s) return;

    char *trimmed = trim_spaces(s);

    if (trimmed != s) {
        memmove(s, trimmed, strlen(trimmed) + 1);
    }
}

/* Convierte una cadena a minusculas */
void lower_str(const char *src, char *dst, size_t dst_size) {
    if (dst_size == 0) return;

    size_t i = 0;
    for (; src[i] && i < dst_size - 1; i++) {
        dst[i] = (char)tolower((unsigned char)src[i]);
    }

    dst[i] = '\0';
}

/* Parsea un entero dentro de un rango */
bool parse_int_range(const char *s, int min, int max, int *out) {
    char *end = NULL;
    long value = strtol(s, &end, 10);

    if (end == s) return false;

    while (*end && isspace((unsigned char)*end)) end++;

    if (*end != '\0') return false;
    if (value < min || value > max) return false;

    *out = (int)value;
    return true;
}

/* Parsea un double dentro de un rango */
bool parse_double_range(const char *s, double min, double max, double *out) {
    char *end = NULL;
    double value = strtod(s, &end);

    if (end == s) return false;

    while (*end && isspace((unsigned char)*end)) end++;

    if (*end != '\0') return false;
    if (value < min || value > max) return false;

    *out = value;
    return true;
}

/* Lee un entero dentro de un rango */
bool read_int(const char *prompt, int min, int max, int *out) {
    char buffer[64];

    while (true) {
        if (!read_line(prompt, buffer, sizeof(buffer))) {
            return false;
        }

        int value = 0;
        if (parse_int_range(buffer, min, max, &value)) {
            *out = value;
            return true;
        }

        printf("Ingrese un numero entero valido entre %d y %d.\n", min, max);
    }
}

/* Lee un double dentro de un rango */
bool read_double(const char *prompt, double min, double max, double *out) {
    char buffer[64];

    while (true) {
        if (!read_line(prompt, buffer, sizeof(buffer))) {
            return false;
        }

        double value = 0.0;
        if (parse_double_range(buffer, min, max, &value)) {
            *out = value;
            return true;
        }

        printf("Ingrese un numero valido entre %.2f y %.2f.\n", min, max);
    }
}

/* Lee un campo obligatorio */
bool read_required(const char *prompt, char *buffer, size_t size) {
    while (true) {
        if (!read_line(prompt, buffer, size)) {
            return false;
        }

        if (has_text(buffer)) {
            trim_inplace(buffer);
            return true;
        }

        printf("Este campo es obligatorio.\n");
    }
}

/* Lee un entero opcional; si se presiona Enter, conserva el valor actual */
bool read_optional_int(const char *prompt, int current, int min, int max, int *out) {
    char buffer[64];

    while (true) {
        if (!read_line(prompt, buffer, sizeof(buffer))) {
            return false;
        }

        if (!has_text(buffer)) {
            *out = current;
            return true;
        }

        int value = 0;
        if (parse_int_range(buffer, min, max, &value)) {
            *out = value;
            return true;
        }

        printf("Ingrese un numero entero valido entre %d y %d, o presione Enter para conservar.\n",
               min, max);
    }
}

/* Lee un double opcional; si se presiona Enter, conserva el valor actual */
bool read_optional_double(const char *prompt, double current, double min, double max, double *out) {
    char buffer[64];

    while (true) {
        if (!read_line(prompt, buffer, sizeof(buffer))) {
            return false;
        }

        if (!has_text(buffer)) {
            *out = current;
            return true;
        }

        double value = 0.0;
        if (parse_double_range(buffer, min, max, &value)) {
            *out = value;
            return true;
        }

        printf("Ingrese un numero valido entre %.2f y %.2f, o presione Enter para conservar.\n",
               min, max);
    }
}

/* Genera fecha y hora actual */
void current_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    if (!tm_info) {
        copy_str(buffer, "0000-00-00 00:00", size);
        return;
    }

    strftime(buffer, size, "%Y-%m-%d %H:%M", tm_info);
}

/* ============================ Listas dinamicas ============================ */

bool products_ensure(ProductList *list) {
    if (list->count < list->cap) return true;

    size_t new_cap = list->cap ? list->cap * 2 : 8;
    Product *tmp = realloc(list->data, new_cap * sizeof(Product));

    if (!tmp) return false;

    list->data = tmp;
    list->cap = new_cap;

    return true;
}

bool sales_ensure(SaleList *list) {
    if (list->count < list->cap) return true;

    size_t new_cap = list->cap ? list->cap * 2 : 8;
    Sale *tmp = realloc(list->data, new_cap * sizeof(Sale));

    if (!tmp) return false;

    list->data = tmp;
    list->cap = new_cap;

    return true;
}

void free_products(ProductList *list) {
    free(list->data);
    list->data = NULL;
    list->count = 0;
    list->cap = 0;
}

void free_sales(SaleList *list) {
    free(list->data);
    list->data = NULL;
    list->count = 0;
    list->cap = 0;
}

/* ============================ Busqueda ============================ */

Product *find_product(ProductList *list, int code) {
    for (size_t i = 0; i < list->count; i++) {
        if (list->data[i].codigo == code) {
            return &list->data[i];
        }
    }

    return NULL;
}

int find_product_index(ProductList *list, int code) {
    for (size_t i = 0; i < list->count; i++) {
        if (list->data[i].codigo == code) {
            return (int)i;
        }
    }

    return -1;
}

bool product_code_exists(ProductList *list, int code) {
    return find_product(list, code) != NULL;
}

int next_sale_id(const SaleList *list) {
    int max_id = 0;

    for (size_t i = 0; i < list->count; i++) {
        if (list->data[i].id > max_id) {
            max_id = list->data[i].id;
        }
    }

    return max_id + 1;
}

/* ============================ Impresion ============================ */

void print_product_header(void) {
    printf("%-8s %-24s %-16s %10s %10s %-20s\n",
           "Codigo", "Nombre", "Categoria", "Precio", "Cantidad", "Proveedor");
}

void print_product_row(const Product *p) {
    printf("%-8d %-24.24s %-16.16s %10.2f %10d %-20.20s\n",
           p->codigo,
           p->nombre,
           p->categoria,
           p->precio,
           p->cantidad,
           p->proveedor);
}

void print_products(ProductList *list) {
    if (list->count == 0) {
        printf("No hay productos registrados.\n");
        return;
    }

    print_product_header();

    for (size_t i = 0; i < list->count; i++) {
        print_product_row(&list->data[i]);
    }
}

void print_sale_header(void) {
    printf("%-6s %-10s %10s %14s %-18s\n",
           "ID", "Producto", "Cantidad", "Precio total", "Fecha");
}

void print_sale_row(const Sale *s) {
    printf("%-6d %-10d %10d %14.2f %-18s\n",
           s->id,
           s->producto_codigo,
           s->cantidad,
           s->precio_total,
           s->fecha);
}

/* ============================ Ordenamiento ============================ */

/* Ordena por precio de menor a mayor */
int cmp_price_asc(const void *a, const void *b) {
    const Product *pa = (const Product *)a;
    const Product *pb = (const Product *)b;

    if (pa->precio < pb->precio) return -1;
    if (pa->precio > pb->precio) return 1;

    return pa->codigo - pb->codigo;
}

/* Ordena por cantidad disponible de mayor a menor */
int cmp_quantity_desc(const void *a, const void *b) {
    const Product *pa = (const Product *)a;
    const Product *pb = (const Product *)b;

    if (pb->cantidad != pa->cantidad) {
        return pb->cantidad - pa->cantidad;
    }

    return strcmp(pa->nombre, pb->nombre);
}

/* ============================ Modulo de productos ============================ */

void add_product(ProductList *products) {
    Product p;
    memset(&p, 0, sizeof(p));

    printf("\n--- Registrar producto ---\n");

    int code = 0;
    if (!read_int("Codigo del producto (1-999999): ", 1, MAX_CODE, &code)) {
        return;
    }

    if (product_code_exists(products, code)) {
        printf("Ya existe un producto con ese codigo.\n");
        return;
    }

    p.codigo = code;

    if (!read_required("Nombre: ", p.nombre, sizeof(p.nombre))) return;
    if (!read_required("Categoria: ", p.categoria, sizeof(p.categoria))) return;
    if (!read_double("Precio: ", 0.0, 1000000.0, &p.precio)) return;
    if (!read_int("Cantidad disponible: ", 0, MAX_QUANTITY, &p.cantidad)) return;
    if (!read_required("Proveedor: ", p.proveedor, sizeof(p.proveedor))) return;

    if (!products_ensure(products)) {
        printf("No hubo memoria disponible para registrar el producto.\n");
        return;
    }

    products->data[products->count] = p;
    products->count++;

    printf("Producto registrado.\n");
}

void modify_product(ProductList *products) {
    printf("\n--- Modificar producto ---\n");

    int code = 0;
    if (!read_int("Codigo del producto a modificar (0 para cancelar): ", 0, MAX_CODE, &code)) {
        return;
    }

    if (code == 0) return;

    Product *p = find_product(products, code);
    if (!p) {
        printf("Producto no encontrado.\n");
        return;
    }

    char buffer[MAX_NAME];

    printf("Nombre actual: %s\n", p->nombre);
    if (read_line("Nuevo nombre (Enter para conservar): ", buffer, sizeof(buffer)) && has_text(buffer)) {
        trim_inplace(buffer);
        copy_str(p->nombre, buffer, sizeof(p->nombre));
    }

    printf("Categoria actual: %s\n", p->categoria);
    if (read_line("Nueva categoria (Enter para conservar): ", buffer, sizeof(buffer)) && has_text(buffer)) {
        trim_inplace(buffer);
        copy_str(p->categoria, buffer, sizeof(p->categoria));
    }

    double new_price = p->precio;
    if (!read_optional_double("Nuevo precio (Enter para conservar): ",
                              p->precio, 0.0, 1000000.0, &new_price)) {
        return;
    }
    p->precio = new_price;

    int new_quantity = p->cantidad;
    if (!read_optional_int("Nueva cantidad disponible (Enter para conservar): ",
                           p->cantidad, 0, MAX_QUANTITY, &new_quantity)) {
        return;
    }
    p->cantidad = new_quantity;

    printf("Proveedor actual: %s\n", p->proveedor);
    if (read_line("Nuevo proveedor (Enter para conservar): ", buffer, sizeof(buffer)) && has_text(buffer)) {
        trim_inplace(buffer);
        copy_str(p->proveedor, buffer, sizeof(p->proveedor));
    }

    printf("Producto modificado.\n");
}

void delete_product(ProductList *products) {
    printf("\n--- Eliminar producto ---\n");

    int code = 0;
    if (!read_int("Codigo del producto a eliminar (0 para cancelar): ", 0, MAX_CODE, &code)) {
        return;
    }

    if (code == 0) return;

    int index = find_product_index(products, code);
    if (index < 0) {
        printf("Producto no encontrado.\n");
        return;
    }

    int confirm = 0;
    if (!read_int("Confirmar? 1=Si, 0=No: ", 0, 1, &confirm)) {
        return;
    }

    if (confirm != 1) {
        printf("Operacion cancelada.\n");
        return;
    }

    /* Se desplazan los elementos para cubrir el espacio del producto borrado */
    for (size_t i = (size_t)index; i + 1 < products->count; i++) {
        products->data[i] = products->data[i + 1];
    }

    products->count--;

    printf("Producto eliminado del inventario.\n");
}

/* ============================ Busqueda y ordenamiento ============================ */

void search_by_code(ProductList *products) {
    int code = 0;

    if (!read_int("Codigo a buscar (0 para cancelar): ", 0, MAX_CODE, &code)) {
        return;
    }

    if (code == 0) return;

    Product *p = find_product(products, code);
    if (!p) {
        printf("Producto no encontrado.\n");
        return;
    }

    print_product_header();
    print_product_row(p);
}

void search_by_name(ProductList *products) {
    char query[MAX_NAME];
    char query_lower[MAX_NAME];
    char name_lower[MAX_NAME];

    if (!read_required("Nombre a buscar: ", query, sizeof(query))) {
        return;
    }

    lower_str(query, query_lower, sizeof(query_lower));

    bool found = false;

    print_product_header();

    for (size_t i = 0; i < products->count; i++) {
        lower_str(products->data[i].nombre, name_lower, sizeof(name_lower));

        if (strstr(name_lower, query_lower) != NULL) {
            print_product_row(&products->data[i]);
            found = true;
        }
    }

    if (!found) {
        printf("No se encontraron productos con ese nombre.\n");
    }
}

void search_product_menu(ProductList *products) {
    int op = 0;

    do {
        printf("\n--- Buscar y ordenar productos ---\n");
        printf("1. Buscar por codigo\n");
        printf("2. Buscar por nombre\n");
        printf("3. Ordenar por precio\n");
        printf("4. Ordenar por cantidad disponible\n");
        printf("0. Volver\n");

        if (!read_int("Seleccione una opcion: ", 0, 4, &op)) {
            return;
        }

        switch (op) {
            case 1:
                search_by_code(products);
                break;

            case 2:
                search_by_name(products);
                break;

            case 3:
                if (products->count > 1) {
                    qsort(products->data, products->count, sizeof(Product), cmp_price_asc);
                }
                printf("Productos ordenados por precio.\n");
                print_products(products);
                break;

            case 4:
                if (products->count > 1) {
                    qsort(products->data, products->count, sizeof(Product), cmp_quantity_desc);
                }
                printf("Productos ordenados por cantidad disponible.\n");
                print_products(products);
                break;

            case 0:
                break;

            default:
                break;
        }
    } while (op != 0);
}

/* ============================ Control de inventario ============================ */

void register_entry(ProductList *products) {
    printf("\n--- Registrar entrada de producto ---\n");

    int code = 0;
    if (!read_int("Codigo del producto (0 para cancelar): ", 0, MAX_CODE, &code)) {
        return;
    }

    if (code == 0) return;

    Product *p = find_product(products, code);
    if (!p) {
        printf("Producto no encontrado.\n");
        return;
    }

    int max_entry = MAX_QUANTITY - p->cantidad;

    if (max_entry <= 0) {
        printf("El producto ya alcanzo la cantidad maxima permitida.\n");
        return;
    }

    int quantity = 0;
    if (!read_int("Cantidad de entrada: ", 1, max_entry, &quantity)) {
        return;
    }

    p->cantidad += quantity;

    printf("Entrada registrada. Cantidad actual: %d\n", p->cantidad);
}

void register_exit(ProductList *products) {
    printf("\n--- Registrar salida de producto ---\n");

    int code = 0;
    if (!read_int("Codigo del producto (0 para cancelar): ", 0, MAX_CODE, &code)) {
        return;
    }

    if (code == 0) return;

    Product *p = find_product(products, code);
    if (!p) {
        printf("Producto no encontrado.\n");
        return;
    }

    if (p->cantidad <= 0) {
        printf("No hay existencias disponibles para registrar una salida.\n");
        return;
    }

    int quantity = 0;
    if (!read_int("Cantidad de salida: ", 1, p->cantidad, &quantity)) {
        return;
    }

    p->cantidad -= quantity;

    printf("Salida registrada. Cantidad actual: %d\n", p->cantidad);
}

void inventory_menu(ProductList *products) {
    int op = 0;

    do {
        printf("\n--- Actualizar inventario ---\n");
        printf("1. Registrar entrada\n");
        printf("2. Registrar salida\n");
        printf("0. Volver\n");

        if (!read_int("Seleccione una opcion: ", 0, 2, &op)) {
            return;
        }

        switch (op) {
            case 1:
                register_entry(products);
                break;

            case 2:
                register_exit(products);
                break;

            case 0:
                break;

            default:
                break;
        }
    } while (op != 0);
}

void show_low_stock(ProductList *products) {
    printf("\n--- Productos con bajo stock ---\n");
    printf("Umbral de stock bajo: %d unidades\n\n", LOW_STOCK_THRESHOLD);

    if (products->count == 0) {
        printf("No hay productos registrados.\n");
        return;
    }

    bool found = false;

    print_product_header();

    for (size_t i = 0; i < products->count; i++) {
        Product *p = &products->data[i];

        if (p->cantidad <= LOW_STOCK_THRESHOLD) {
            print_product_row(p);

            if (p->cantidad == 0) {
                printf("Estado: Agotado.\n");
            } else {
                printf("Estado: Stock bajo, requiere reposicion.\n");
            }

            printf("\n");
            found = true;
        }
    }

    if (!found) {
        printf("No hay productos con bajo stock.\n");
    }
}

/* ============================ Registro de ventas ============================ */

void register_sale(ProductList *products, SaleList *sales) {
    printf("\n--- Registrar venta ---\n");

    int code = 0;
    if (!read_int("Codigo del producto (0 para cancelar): ", 0, MAX_CODE, &code)) {
        return;
    }

    if (code == 0) return;

    Product *p = find_product(products, code);
    if (!p) {
        printf("Producto no encontrado.\n");
        return;
    }

    printf("Producto: %s\n", p->nombre);
    printf("Precio: %.2f\n", p->precio);
    printf("Cantidad disponible: %d\n", p->cantidad);

    if (p->cantidad <= 0) {
        printf("No hay stock disponible para vender.\n");
        return;
    }

    int quantity = 0;
    if (!read_int("Cantidad vendida: ", 1, p->cantidad, &quantity)) {
        return;
    }

    /* Actualiza automaticamente el inventario */
    p->cantidad -= quantity;

    Sale s;
    memset(&s, 0, sizeof(s));

    s.id = next_sale_id(sales);
    s.producto_codigo = code;
    s.cantidad = quantity;
    s.precio_total = p->precio * (double)quantity;

    current_timestamp(s.fecha, sizeof(s.fecha));

    if (!sales_ensure(sales)) {
        printf("No hubo memoria disponible para registrar la venta.\n");
        /*
          En una aplicacion real, si la venta no se puede guardar,
          se deberia revertir la cantidad descontada.
        */
        p->cantidad += quantity;
        return;
    }

    sales->data[sales->count] = s;
    sales->count++;

    printf("Venta registrada. Precio total: %.2f\n", s.precio_total);
    printf("Cantidad restante de %s: %d\n", p->nombre, p->cantidad);
}

/* ============================ Reportes ============================ */

void generate_reports(ProductList *products, SaleList *sales) {
    printf("\n--- Reportes ---\n");

    size_t total_products = products->count;
    size_t available_products = 0;
    size_t out_of_stock_products = 0;
    Product *max_product = NULL;

    for (size_t i = 0; i < products->count; i++) {
        Product *p = &products->data[i];

        if (p->cantidad > 0) {
            available_products++;
        } else {
            out_of_stock_products++;
        }

        if (!max_product || p->cantidad > max_product->cantidad) {
            max_product = p;
        }
    }

    printf("Total de productos registrados: %zu\n", total_products);
    printf("Productos disponibles: %zu\n", available_products);
    printf("Productos agotados: %zu\n", out_of_stock_products);

    if (max_product) {
        printf("Producto con mayor cantidad: %s (Codigo %d, Cantidad %d)\n",
               max_product->nombre,
               max_product->codigo,
               max_product->cantidad);
    } else {
        printf("Producto con mayor cantidad: No hay productos.\n");
    }

    /* Productos con mayor cantidad: se usa una copia para no modificar el orden original */
    if (products->count > 0) {
        Product *copy = malloc(sizeof(Product) * products->count);

        if (copy) {
            memcpy(copy, products->data, sizeof(Product) * products->count);

            if (products->count > 1) {
                qsort(copy, products->count, sizeof(Product), cmp_quantity_desc);
            }

            printf("\nProductos con mayor cantidad:\n");
            print_product_header();

            size_t top = products->count < 5 ? products->count : 5;

            for (size_t i = 0; i < top; i++) {
                print_product_row(&copy[i]);
            }

            free(copy);
        } else {
            printf("\nNo hubo memoria disponible para mostrar el top de productos.\n");
        }
    }

    /* Reporte de ventas */
    double total_revenue = 0.0;

    for (size_t i = 0; i < sales->count; i++) {
        total_revenue += sales->data[i].precio_total;
    }

    printf("\nVentas realizadas: %zu\n", sales->count);
    printf("Total recaudado: %.2f\n", total_revenue);

    if (sales->count > 0) {
        printf("\nDetalle de ventas:\n");
        print_sale_header();

        for (size_t i = 0; i < sales->count; i++) {
            print_sale_row(&sales->data[i]);
        }
    }
}

/* ============================ Archivos ============================ */

bool save_products(const ProductList *list) {
    FILE *f = fopen("productos.dat", "wb");

    if (!f) return false;

    int count = (int)list->count;

    if (fwrite(&count, sizeof(count), 1, f) != 1) {
        fclose(f);
        return false;
    }

    if (count > 0) {
        if (fwrite(list->data, sizeof(Product), (size_t)count, f) != (size_t)count) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

bool load_products(ProductList *list) {
    FILE *f = fopen("productos.dat", "rb");

    if (!f) return false;

    int count = 0;

    if (fread(&count, sizeof(count), 1, f) != 1 || count < 0) {
        fclose(f);
        return false;
    }

    for (int i = 0; i < count; i++) {
        if (!products_ensure(list)) break;

        if (fread(&list->data[list->count], sizeof(Product), 1, f) != 1) {
            break;
        }

        list->count++;
    }

    fclose(f);
    return true;
}

bool save_sales(const SaleList *list) {
    FILE *f = fopen("ventas.dat", "wb");

    if (!f) return false;

    int count = (int)list->count;

    if (fwrite(&count, sizeof(count), 1, f) != 1) {
        fclose(f);
        return false;
    }

    if (count > 0) {
        if (fwrite(list->data, sizeof(Sale), (size_t)count, f) != (size_t)count) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

bool load_sales(SaleList *list) {
    FILE *f = fopen("ventas.dat", "rb");

    if (!f) return false;

    int count = 0;

    if (fread(&count, sizeof(count), 1, f) != 1 || count < 0) {
        fclose(f);
        return false;
    }

    for (int i = 0; i < count; i++) {
        if (!sales_ensure(list)) break;

        if (fread(&list->data[list->count], sizeof(Sale), 1, f) != 1) {
            break;
        }

        list->count++;
    }

    fclose(f);
    return true;
}

bool save_all(const ProductList *products, const SaleList *sales) {
    bool ok = true;

    if (!save_products(products)) ok = false;
    if (!save_sales(sales)) ok = false;

    return ok;
}

void load_all(ProductList *products, SaleList *sales) {
    free_products(products);
    free_sales(sales);

    load_products(products);
    load_sales(sales);
}

/* ============================ Menu principal ============================ */

int main(void) {
    ProductList products = {0};
    SaleList sales = {0};

    load_all(&products, &sales);

    printf("StockControl C iniciado.\n");
    printf("Productos cargados: %zu\n", products.count);
    printf("Ventas cargadas: %zu\n", sales.count);

    int op = 11;

    do {
        printf("\n=== StockControl C ===\n");
        printf("1. Registrar producto\n");
        printf("2. Mostrar productos\n");
        printf("3. Buscar producto\n");
        printf("4. Modificar producto\n");
        printf("5. Eliminar producto\n");
        printf("6. Registrar venta\n");
        printf("7. Actualizar inventario\n");
        printf("8. Mostrar productos con bajo stock\n");
        printf("9. Generar reportes\n");
        printf("10. Guardar informacion\n");
        printf("11. Salir\n");

        if (!read_int("Seleccione una opcion: ", 1, 11, &op)) {
            op = 11;
            break;
        }

        switch (op) {
            case 1:
                add_product(&products);
                break;

            case 2:
                printf("\n--- Productos registrados ---\n");
                print_products(&products);
                break;

            case 3:
                search_product_menu(&products);
                break;

            case 4:
                modify_product(&products);
                break;

            case 5:
                delete_product(&products);
                break;

            case 6:
                register_sale(&products, &sales);
                break;

            case 7:
                inventory_menu(&products);
                break;

            case 8:
                show_low_stock(&products);
                break;

            case 9:
                generate_reports(&products, &sales);
                break;

            case 10:
                if (save_all(&products, &sales)) {
                    printf("Informacion guardada.\n");
                } else {
                    printf("No fue posible guardar la informacion.\n");
                }
                break;

            case 11: {
                int save_choice = 0;

                if (read_int("Guardar antes de salir? 1=Si, 0=No: ", 0, 1, &save_choice) &&
                    save_choice == 1) {
                    if (save_all(&products, &sales)) {
                        printf("Informacion guardada.\n");
                    } else {
                        printf("No fue posible guardar la informacion.\n");
                    }
                }

                printf("Fin del programa.\n");
                break;
            }

            default:
                break;
        }
    } while (op != 11);

    free_products(&products);
    free_sales(&sales);

    return 0;
}
