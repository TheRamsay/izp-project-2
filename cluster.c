/**
 * Kostra programu pro 2. projekt IZP 2022/23
 *
 * Jednoducha shlukova analyza: 2D nejblizsi soused.
 * Single linkage
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h> // sqrtf
#include <limits.h> // INT_MAX
#include <stdbool.h>

/*****************************************************************
 * Ladici makra. Vypnout jejich efekt lze definici makra
 * NDEBUG, napr.:
 *   a) pri prekladu argumentem prekladaci -DNDEBUG
 *   b) v souboru (na radek pred #include <assert.h>
 *      #define NDEBUG
 */
#ifdef NDEBUG
#define debug(s)
#define dfmt(s, ...)
#define dint(i)
#define dfloat(f)
#else

// vypise ladici retezec
#define debug(s) printf("- %s\n", s)

// vypise formatovany ladici vystup - pouziti podobne jako printf
#define dfmt(s, ...) printf(" - "__FILE__":%u: "s"\n",__LINE__,__VA_ARGS__)

// vypise ladici informaci o promenne - pouziti dint(identifikator_promenne)
#define dint(i) printf(" - " __FILE__ ":%u: " #i " = %d\n", __LINE__, i)

// vypise ladici informaci o promenne typu float - pouziti
// dfloat(identifikator_promenne)
#define dfloat(f) printf(" - " __FILE__ ":%u: " #f " = %g\n", __LINE__, f)

#endif

/*****************************************************************
 * Deklarace potrebnych datovych typu:
 *
 * TYTO DEKLARACE NEMENTE
 *
 *   struct obj_t - struktura objektu: identifikator a souradnice
 *   struct cluster_t - shluk objektu:
 *      pocet objektu ve shluku,
 *      kapacita shluku (pocet objektu, pro ktere je rezervovano
 *          misto v poli),
 *      ukazatel na pole shluku.
 */

struct obj_t {
    int id;
    float x;
    float y;
};

struct cluster_t {
    int size;
    int capacity;
    struct obj_t *obj;
};

typedef struct obj_t obj_t;
typedef struct cluster_t cluster_t;

/**********************************************************************/
/* Vlastni funkce*/

void raise_error(FILE *f, cluster_t **arr, char *msg) {
    if (arr) {
        free(arr);
    }

    if (f) {
        fclose(f);
    }

    fprintf(stderr, "%s\n", msg);
}

bool array_contains(int *arr, int len, int num) {
    for (int i = 0; i < len; i++)
    {
        if (arr[i] == num) {
            return true;
        }
    }
    
    return false;
}

/*****************************************************************
 * Deklarace potrebnych funkci.
 *
 * PROTOTYPY FUNKCI NEMENTE
 *
 * IMPLEMENTUJTE POUZE FUNKCE NA MISTECH OZNACENYCH 'TODO'
 *
 */

/*
 Inicializace shluku 'c'. Alokuje pamet pro cap objektu (kapacitu).
 Ukazatel NULL u pole objektu znamena kapacitu 0.
*/
void init_cluster(struct cluster_t *c, int cap)
{
    assert(c != NULL);
    assert(cap >= 0);

    c->size = 0;
    c->obj = malloc(cap * sizeof(struct obj_t));

    if (c->obj == NULL) {
        cap = 0;
    }

    c->capacity = cap;
}

/*
 Odstraneni vsech objektu shluku a inicializace na prazdny shluk.
 */
void clear_cluster(struct cluster_t *c)
{
    assert(c != NULL);

    free(c->obj);
    c->size = 0;
    c->capacity = 0;
}

/// Chunk of cluster objects. Value recommended for reallocation.
const int CLUSTER_CHUNK = 10;

/*
 Zmena kapacity shluku 'c' na kapacitu 'new_cap'.
 */
struct cluster_t *resize_cluster(struct cluster_t *c, int new_cap)
{
    // TUTO FUNKCI NEMENTE
    assert(c);
    assert(c->capacity >= 0);
    assert(new_cap >= 0);

    if (c->capacity >= new_cap)
        return c;

    size_t size = sizeof(struct obj_t) * new_cap;

    void *arr = realloc(c->obj, size);
    if (arr == NULL)
        return NULL;

    c->obj = (struct obj_t*)arr;
    c->capacity = new_cap;
    return c;
}

/*
 Prida objekt 'obj' na konec shluku 'c'. Rozsiri shluk, pokud se do nej objekt
 nevejde.
 */
// Append object to the end of cluster. If the cluster is full, resize it.
void append_cluster(struct cluster_t *c, struct obj_t obj)
{
    assert(c != NULL);

    if (c->size == c->capacity) {
        resize_cluster(c, c->capacity + CLUSTER_CHUNK);
    }

    c->obj[c->size++] = obj;
}

/*
 Seradi objekty ve shluku 'c' vzestupne podle jejich identifikacniho cisla.
 */
void sort_cluster(struct cluster_t *c);

/*
 Do shluku 'c1' prida objekty 'c2'. Shluk 'c1' bude v pripade nutnosti rozsiren.
 Objekty ve shluku 'c1' budou serazeny vzestupne podle identifikacniho cisla.
 Shluk 'c2' bude nezmenen.
 */
void merge_clusters(struct cluster_t *c1, struct cluster_t *c2)
{
    assert(c1 != NULL);
    assert(c2 != NULL);

    if (c1->capacity <= c1->size + c2->size) {
        resize_cluster(c1, c1->size + c2->size);
    }

    for (int i = 0; i < c2->size; i++) {
        append_cluster(c1, c2->obj[i]);
    }

    sort_cluster(c1);
}

/**********************************************************************/
/* Prace s polem shluku */

/*
 Odstrani shluk z pole shluku 'carr'. Pole shluku obsahuje 'narr' polozek
 (shluku). Shluk pro odstraneni se nachazi na indexu 'idx'. Funkce vraci novy
 pocet shluku v poli.
*/
int remove_cluster(struct cluster_t *carr, int narr, int idx)
{
    assert(carr != NULL);
    assert(narr > 0);
    assert(idx >= 0 && idx < narr);

    clear_cluster(&carr[idx]);

    for (int i = idx; i < narr - 1; i++) {
        carr[i] = carr[i + 1];
    }

    return narr - 1;
}

/*
 Pocita Euklidovskou vzdalenost mezi dvema objekty.
 */
float obj_distance(struct obj_t *o1, struct obj_t *o2)
{
    assert(o1 != NULL);
    assert(o2 != NULL);

    return sqrt(pow(o1->x - o2->x, 2) + pow(o1->y - o2->y, 2));
}

/*
 Pocita vzdalenost dvou shluku.
*/
float cluster_distance(struct cluster_t *c1, struct cluster_t *c2)
{
    assert(c1 != NULL);
    assert(c1->size > 0);
    assert(c2 != NULL);
    assert(c2->size > 0);

    float min = INFINITY;

    for (int i = 0; i < c1->size; i++) {
        for (int j = 0; j < c2->size; j++) {
            float distance = obj_distance(&c1->obj[i], &c2->obj[j]);

            if (distance < min) {
                min = distance;
            }
        }
    }

    return min;
}

/*
 Funkce najde dva nejblizsi shluky. V poli shluku 'carr' o velikosti 'narr'
 hleda dva nejblizsi shluky. Nalezene shluky identifikuje jejich indexy v poli
 'carr'. Funkce nalezene shluky (indexy do pole 'carr') uklada do pameti na
 adresu 'c1' resp. 'c2'.
*/
void find_neighbours(struct cluster_t *carr, int narr, int *c1, int *c2)
{
    assert(carr != NULL);
    assert(narr > 0);

    float min = INFINITY;

    for (int i = 0; i < narr - 1; i++) {
        for (int j = i + 1; j < narr; j++) {
            float distance = cluster_distance(&carr[i], &carr[j]);

            if (distance < min) {
                min = distance;
                *c1 = i;
                *c2 = j;
            }
        }
    }
}

// pomocna funkce pro razeni shluku
static int obj_sort_compar(const void *a, const void *b)
{
    // TUTO FUNKCI NEMENTE
    const struct obj_t *o1 = (const struct obj_t *)a;
    const struct obj_t *o2 = (const struct obj_t *)b;
    if (o1->id < o2->id) return -1;
    if (o1->id > o2->id) return 1;
    return 0;
}

/*
 Razeni objektu ve shluku vzestupne podle jejich identifikatoru.
*/
void sort_cluster(struct cluster_t *c)
{
    // TUTO FUNKCI NEMENTE
    qsort(c->obj, c->size, sizeof(struct obj_t), &obj_sort_compar);
}

/*
 Tisk shluku 'c' na stdout.
*/
void print_cluster(struct cluster_t *c)
{
    // TUTO FUNKCI NEMENTE
    for (int i = 0; i < c->size; i++)
    {
        if (i) putchar(' ');
        printf("%d[%g,%g]", c->obj[i].id, c->obj[i].x, c->obj[i].y);
    }
    putchar('\n');
}

/*
 Ze souboru 'filename' nacte objekty. Pro kazdy objekt vytvori shluk a ulozi
 jej do pole shluku. Alokuje prostor pro pole vsech shluku a ukazatel na prvni
 polozku pole (ukalazatel na prvni shluk v alokovanem poli) ulozi do pameti,
 kam se odkazuje parametr 'arr'. Funkce vraci pocet nactenych objektu (shluku).
 V pripade nejake chyby uklada do pameti, kam se odkazuje 'arr', hodnotu NULL.
*/
int load_clusters(char *filename, struct cluster_t **arr)
{
    assert(arr != NULL);

    FILE *file = fopen(filename, "r");
    int n;
    int *ids;

    if (file == NULL) {
        raise_error(NULL, NULL, "Error: File could not be opened.");
        return 0;
    }

    fscanf(file, "count=%d", &n);

    if (n < 1) {
        raise_error(file, arr, "Error: Invalid cluster count.");
        return 0;
    }

    ids = malloc(n * sizeof(*ids));
    (*arr) = malloc(n * sizeof(struct cluster_t));

    if (*arr == NULL || ids == NULL) {
        raise_error(file, arr, "Error: Memory allocation failed.");
        return 0;
    }

    for (int i = 0; i < n; i++) {
        int id;
        float x, y;

        if (array_contains(ids, i, id)) {
            raise_error(file, arr, "Error: Object ID is not unique.");
            return 0;
        }

        if (fscanf(file, "%d %f %f", &id, &x, &y) != 3) {
            raise_error(file, arr, "Error: Invalid format of input. Required format is ID[int] x[float] y[float].");
            return 0;
        }

        if (x < 0 || x > 1000 || y < 0 || y > 1000) {
            raise_error(file, arr, "Error: Invalid coordinates. Coordinates must be in range <0, 1000>.");
            return 0;
        }

        obj_t obj = {id, x, y};
        init_cluster(&((*arr)[i]), 1);
        append_cluster(&((*arr)[i]), obj);

        // ids[i] = id;
    }

    fclose(file);

    return n;
}

void process(struct cluster_t *clusters, int n, int k)
{
    while (n > k) {
        int c1, c2;
        find_neighbours(clusters, n, &c1, &c2);
        merge_clusters(&clusters[c1], &clusters[c2]);
        n = remove_cluster(clusters, n, c2);
    }
}

/*
 Tisk pole shluku. Parametr 'carr' je ukazatel na prvni polozku (shluk).
 Tiskne se prvnich 'narr' shluku.
*/
void print_clusters(struct cluster_t *carr, int narr)
{
    printf("Clusters:\n");
    for (int i = 0; i < narr; i++)
    {
        printf("cluster %d: ", i);
        print_cluster(&carr[i]);
    }
}

int main(int argc, char *argv[])
{
    struct cluster_t *clusters;
    int k;

    if (argc == 1) {
        fprintf(stderr, "Error: You have to specify the filename.\n");
        return EXIT_FAILURE;
    }

    int n = load_clusters(argv[1], &clusters);

    if (clusters == NULL) {
        return EXIT_FAILURE;
    }

    if (argc < 3) {
        k = 1;
    } else {
        k = atoi(argv[2]);
        if (k < 1) {
            fprintf(stderr, "Error: N parameter has to be greater than 0.\n");
            return EXIT_FAILURE;
        }
    }

    if (k > n) {
        k = n;
    }

    process(clusters, n, k);

    print_clusters(clusters, k);

    for (int i = 0; i < k; i++) {
        clear_cluster(&clusters[i]);
    }

    free(clusters);
}