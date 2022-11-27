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
#include <errno.h>
#include <string.h>

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

#define LINE_LENGTH 100 
#define CENTROID_ID -1 // ID for special centroid objects
#define INVALID_IDX -1

void clear_cluster(struct cluster_t *c);

/**********************************************************************/
/* Vlastni funkce*/
bool is_integer(float num) {
    return (int)num == num;
}

bool array_contains(int *arr, int size, int val) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == val) {
            return true;
        }
    }
    return false;
}

void clear_clusters(cluster_t *clusters, int size) {
    assert(clusters != NULL);

    for (int i = 0; i < size; i++) {
        clear_cluster(&clusters[i]);
    }
}

// Takes pointer to array of clusters, free's all the clusters and the array
// Then it free's array of ID's nad closes a file
void raise_error(FILE *f, cluster_t **clusters_ptr, int clusters_size, char *msg) {
    if (clusters_ptr && *clusters_ptr) {
        clear_clusters(*clusters_ptr, clusters_size);
        free(*clusters_ptr);
        *clusters_ptr = NULL;
    }

    if (f) {
        fclose(f);
    }

    fprintf(stderr, "%s\n", msg);
}

bool obj_exists(cluster_t *clusters, int len, int obj_id) {
    for (int i = 0; i < len; i++)
    {
        for (int j = 0; j < clusters[i].size; j++)
        {
            if (clusters[i].obj[j].id == obj_id)
            {
                return true;
            }
        }
    }
    
    return false;
}

bool parse_int(char *str, int *num) {
    char *endptr = NULL;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if (errno != 0 || *endptr != '\0' || val > INT_MAX || val < INT_MIN) {
        return false;
    }

    *num = val;

    return true;
}

void split(char *str, char *delim, char *tokens[], int max_splits, int *tokens_len) {
    char *token = strtok(str, delim);
    *tokens_len = 0;

    while (token != NULL || tokens_len != max_splits) {
        tokens[(*tokens_len)++] = token;
        token = strtok(NULL, delim);
    }
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

    if (cap == 0) {
        c->obj = NULL;
    } else {
        c->obj = malloc(cap * sizeof(struct obj_t));
    }

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
    init_cluster(c, 0);
    // c->capacity = 0;
    // c->size = 0;
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

void remove_object(cluster_t *c, int obj_id) {
    int idx = INVALID_IDX;

    for (int i = 0; i < c->size; i++)
    {
        if (c->obj[i].id == obj_id) {
            idx = i;
            break;
        }
    }
    
    if (idx == INVALID_IDX) {
        return;
    }

    for (size_t i = idx; i < c->size - 1; i++)
    {
        c->obj[i] = c->obj[i + 1];
    }

    c->size--;
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

float cluster_distance_complete_linkage(cluster_t *c1, cluster_t *c2) {
    assert(c1 != NULL);
    assert(c1->size > 0);
    assert(c2 != NULL);
    assert(c2->size > 0);

    float max = 0.0;

    for (int i = 0; i < c1->size; i++)
    {
        for (int j = 0; j < c2->size; j++)
        {
            float distance = obj_distance(&c1->obj[i], &c1->obj[j]);

            if (distance > max) {
                max = distance;
            }
        }
        
    }

    return max;
}

/*
 Funkce najde dva nejblizsi shluky. V poli shluku 'carr' o velikosti 'narr'
 hleda dva nejblizsi shluky. Nalezene shluky identifikuje jejich indexy v poli
 'carr'. Funkce nalezene shluky (indexy do pole 'carr') uklada do pameti na
 adresu 'c1' resp. 'c2'.
*/
void find_neighbours(struct cluster_t *carr, int narr, int *c1, int *c2, float (*comparator)(cluster_t *, cluster_t*))
{
    assert(carr != NULL);
    assert(narr > 0);

    float min = INFINITY;

    for (int i = 0; i < narr - 1; i++) {
        for (int j = i + 1; j < narr; j++) {
            float distance = (*comparator)(&carr[i], &carr[j]);

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
    char buff[LINE_LENGTH];
    char test_char;
    int n;
    float raw_n;
    *arr = NULL;

    if (file == NULL) {
        raise_error(NULL, NULL, 0, "Error: File could not be opened.");
        return 0;
    }

    if (fscanf(file, "count=%f%[^\n]", &raw_n, &test_char) != 1) {
        raise_error(file, NULL, 0, "Error: Couldn't parse a file.");
        return 0;
    }

    if (!is_integer(raw_n)) {
        raise_error(file, NULL, 0, "Error: Count has to be an integer.");
        return 0;
    }

    n = (int) raw_n;

    if (n < 0) {
        raise_error(file, NULL, 0, "Error: Invalid cluster count.");
        return 0;
    }

    (*arr) = malloc(n * sizeof(struct cluster_t));

    if (*arr == NULL) {
        raise_error(file, arr, 0, "Error: Memory allocation failed.");
        return 0;
    }

    for (int i = 0; i < n; i++) {
        float id, x, y;

        if (fscanf(file, "%f %f %f%[^\n]", &id, &x, &y, &test_char) != 3) {
            raise_error(file, arr, i, "Error: Invalid format of input.");
            return 0;
        }

        if (!is_integer(id) || !is_integer(x) || !is_integer(y)) {
            raise_error(file, arr, i, "Error: Invalid format of input. Numbers have to be integers.");
            return 0;
        }

        if (obj_exists(*arr, i, id)) {
            raise_error(file, arr, i, "Error: Object ID is not unique.");
            return 0;
        }

        if (x < 0 || x > 1000 || y < 0 || y > 1000) {
            raise_error(file, arr, i, "Error: Invalid coordinates. Coordinates must be in range <0, 1000>.");
            return 0;
        }

        obj_t obj = { (int)id, x, y};
        init_cluster(&((*arr)[i]), 1);
        append_cluster(&((*arr)[i]), obj);
    }

    fclose(file);

    return n;
}

void calculate_centroid(cluster_t *c, obj_t *centroid) {
    assert(c);

    for (int i = 0; i < c->size; i++)
    {   
        centroid->x += c->obj[i].x;
        centroid->y += c->obj[i].y;
    }

    centroid->x /= c->size;
    centroid->y /= c->size;
}

void generate_random_ints(obj_t *objects, obj_t **centroids, int size, int count) {
    int *generated = malloc(count * sizeof(int));

    if (generated == NULL) {
        // TODO null handling
        return;
    }

    int random;

    for (int i = 0; i < count; i++) {
        do
        {
            random = rand() % size;
        } 
        while (array_contains(generated, count, random));

        generated[i] = random;
        centroids[i] = &objects[random];
    }

    free(generated);
}

// Functions takes array of clusters and objects, index of current object, and number of clusters
int assign_to_cluster(obj_t *centroids, obj_t *obj, int k) {
    float min = INFINITY;
    int new_cluster_idx = INVALID_IDX;

    // If object is special centroid type, we skip him
    if (obj->id == CENTROID_ID) {
        return INVALID_IDX;
    }

    // Finds the nearest cluster based on the distance from centroid
    for (int i = 0; i < k; i++)
    {
        float distance = obj_distance(obj, &centroids[i]);
        if (distance < min) {
            min = distance;
            new_cluster_idx = i;
        }
    }

    // Appending object to different cluster and removing it from previous.
    // append_cluster(&clusters[min_idx], objects[object_idx]);
    // remove_object(&clusters[previous_cluster], objects[object_idx].id);
    return new_cluster_idx;
}

void k_means(cluster_t *clusters, obj_t *objects, int size, int k) {
    // Array of indexes from array of objects, contains centroids for each cluster.
    // First centroid is centroid of first cluster in array of clusters etc.
    obj_t **centroids = calloc(k, sizeof(*centroids));

    // Generate k random indexes for centroids of each cluster
    generate_random_ints(objects, centroids, size, k);

    // Assign these centroid objects to each cluster
    for (int i = 0; i < k; i++)
    {
        init_cluster(&clusters[i], 1);
        append_cluster(&clusters[i], *centroids[i]);
    }

    // Loop over all objects and assign each objects to it's nearest cluster.
    // Distance is measured by distance to cluster's centroid.
    for (int i = 0; i < size; i++)
    {
        int cluster_idx = assign_to_cluster(centroids, &objects[i], k);
        append_cluster(&clusters[cluster_idx], objects[i]);
    }

    // Iterate over clusters,find new centeroids and reassign clusters, until no changes are made.
    while (true) {
        int changed = false;

        // recalculates centroids
        for (int i = 0; i < k; i++)
        {
            obj_t centroid = { .id = CENTROID_ID };
            calculate_centroid(&clusters[i], &centroid);
        }

        // Reassign all objects
        for (int i = 0; i < k; i++)
        {
            for (int j = 0; j < clusters[i].size; j++)
            {
                int cluster_idx = assign_to_cluster(clusters, objects, k);
                if (cluster_idx != INVALID_IDX) {
                    changed = true;
                }
            }
        }

        if (!changed) {
            break;
        }
    }

    free(centroids);
}

void single_linkage(struct cluster_t *clusters, int n, int k)
{
    while (n > k) {
        int c1, c2;
        find_neighbours(clusters, n, &c1, &c2, &cluster_distance);
        merge_clusters(&clusters[c1], &clusters[c2]);
        n = remove_cluster(clusters, n, c2);
    }
}

void complete_linkage(struct cluster_t *clusters, int n, int k)
{
    while (n > k) {
        int c1, c2;
        find_neighbours(clusters, n, &c1, &c2, &cluster_distance_complete_linkage);
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
    double raw_k;
    int k = 0;

    switch (argc)
    {
    case 1:
        fprintf(stderr, "Error: You have to specify the filename.\n");
        return EXIT_FAILURE;
    case 2:
        k = 1;
        break;
    case 3:
        raw_k = atof(argv[2]);
        if (!is_integer(raw_k)) {
            fprintf(stderr, "Error: N has to be a integer.\n");
            return EXIT_FAILURE;
        }

        k = (int) raw_k;
        break;
    default:
        fprintf(stderr, "Error: Unknown parameter.\n");
        return EXIT_FAILURE;
    }

    if (k < 1) {
        fprintf(stderr, "Error: N parameter has to be integer greater than 0.\n");
        return EXIT_FAILURE;
    }

    int n = load_clusters(argv[1], &clusters);

    if (clusters == NULL) {
        return EXIT_FAILURE;
    }

    if (k > n) {
        k = n;
    }

    complete_linkage(clusters, n, k);
    print_clusters(clusters, k);

    for (int i = 0; i < k; i++) {
        clear_cluster(&clusters[i]);
    }

    free(clusters);
}
