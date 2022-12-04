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
#include <time.h> // Generating seed for rand()

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

// Structure for storing objects for swap in K-means algorithm
typedef struct swap_obj {
    obj_t *obj;
    int old_idx;
    int new_idx;
} swap_obj;

typedef enum ClusteringMethod {
    UNKNOWN,
    SINGLE_LINKAGE,
    COMPLETE_LINKAGE,
    K_MEANS,
    AVERAGE_LINKAGE
} ClusteringMethod;

typedef struct {
    ClusteringMethod method;
    int final_clusters;
    char *filename;
} Config;

#define BUFFER_LENGTH 15 
#define LINE_LENGTH BUFFER_LENGTH * 3 + 10 // id + x + y + extra space
#define CENTROID_ID -1 // ID for special centroid objects
#define INVALID_IDX -1 //fwfew
#define NO_CLUSTERS_LOADED 0

void clear_cluster(struct cluster_t *c);

/**********************************************************************/
/* Vlastni funkce*/

void print_clusters(struct cluster_t *carr, int narr);

// Print error message and return error code
int print_error(char *msg) {
    fprintf(stderr, "%s", msg);
    return EXIT_FAILURE;
}

// // Check if number is an integer
// bool is_integer(float num) {
//     return (num == (int)num);
// }

// Check if int value exists in an array
bool array_contains(int *arr, int size, int val) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == val) {
            return true;
        }
    }
    return false;
}

// Clears an array of clusters
void clear_clusters(cluster_t *clusters, int size) {
    if (clusters == NULL) {
        return;
    }

    for (int i = 0; i < size; i++) {
        clear_cluster(&clusters[i]);
    }
}

// Cleans everything in main
void final_cleanup(cluster_t *clusters, obj_t *objects, int cluster_count) {
    clear_clusters(clusters, cluster_count);
    free(clusters);
    free(objects);
}

// Takes pointer to array of clusters, free's all the clusters and the array
// Then it free's array of ID's nad closes a file
void raise_error(FILE *file_ptr, cluster_t **clusters_ptr, obj_t **objects_ptr, int clusters_size, char *msg) {
    if (clusters_ptr && *clusters_ptr) {
        clear_clusters(*clusters_ptr, clusters_size);
        free(*clusters_ptr);
        *clusters_ptr = NULL;
    }

    if (objects_ptr) {
        free(*objects_ptr);
        *objects_ptr = NULL;
    }

    if (file_ptr) {
        fclose(file_ptr);
    }

    fprintf(stderr, "%s\n", msg);
}

// Check if object exists in an array of clusters
bool objets_exists(obj_t *objects, int len, int obj_id) {
    for (int i = 0; i < len; i++)
    {
        if (objects[i].id == obj_id)
        {
            return true;
        }
    }
    
    return false;
}

// Check if object exists in any of the clusters
bool is_unique(cluster_t *clusters, int cluster_count, int obj_id) {
    for (int i = 0; i < cluster_count; i++)
    {
        if (objets_exists(clusters[i].obj, clusters[i].size, obj_id))
        {
            return false;
        }
    }
    
    return true;
}

// Check if string is valid integer and parse it
bool is_integer(float float_value) {

    long long_value = (long)float_value;

    if (long_value > INT_MAX || long_value < INT_MIN) {
        return false;
    }

    return float_value == (int) float_value;

    // char *endptr = NULL;
    // errno = 0;
    // long val = strtol(str, &endptr, 10);

    // if (errno != 0 || *endptr != '\0' || val > INT_MAX || val < INT_MIN) {
    //     return false;
    // }

    // // Check if number is an integer, because strtol doesn't check it
    // if (!is_integer(atof(str))) {
    //     return false;
    // }
    // *num = val;

    // return true;
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
        return;
    }

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
    init_cluster(c, 0);
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

// Function that removes object from a cluster by it's ID
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

    for (int i = idx; i < c->size - 1; i++)
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

// Computation of distance between two clusters for complete linkage
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

// Computation of distance between two clusters for complete linkage
float cluster_distance_average_linkage(cluster_t *c1, cluster_t *c2) {
    assert(c1 != NULL);
    assert(c1->size > 0);
    assert(c2 != NULL);
    assert(c2->size > 0);

    float distances_sum = 0;

    for (int i = 0; i < c1->size; i++)
    {
        for (int j = 0; j < c2->size; j++)
        {
            distances_sum += obj_distance(&c1->obj[i], &c1->obj[j]);
        }
        
    }

    return distances_sum / (c1->size + c2->size);
}

/*
 Funkce najde dva nejblizsi shluky. V poli shluku 'carr' o velikosti 'narr'
 hleda dva nejblizsi shluky. Nalezene shluky identifikuje jejich indexy v poli
 'carr'. Funkce nalezene shluky (indexy do pole 'carr') uklada do pameti na
 adresu 'c1' resp. 'c2'.
*/
// Pozn. Pridal jsem si zde pointer na funkci, ktery porovnava vzdalenosti shluku.
// Dal jsem si ji zde abych nemusel zbytecne kopirovat kod pro bonusovde metody.
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
// Pozn. Pridal jsem si zde Enum pro urceni ktera metoda se pouziva pro shlukovani.
// Dale jsem pridal take ukazatel na pole objektu, kvuli metode k-means, jelikoz ta na zacatku neprideluje kazdemu objektu vlastni shluk.
int load_clusters(char *filename, struct cluster_t **arr, ClusteringMethod method, obj_t **objects)
{
    assert(arr != NULL);

    FILE *file = fopen(filename, "r");
    // Character for checking if there is something after valid data
    char garbage_test;
    int cluster_count;
    char cluster_count_str[15], line[BUFFER_LENGTH * 3 + 10];
    float cluster_count_float;
    *arr = NULL;
    *objects = NULL;

    if (file == NULL) {
        raise_error(file, arr, objects, 0, "Error: File could not be opened.");
        return NO_CLUSTERS_LOADED;
    }

    // if (
    //     fgets(line, LINE_LENGTH, file) == NULL ||
    //     sscanf(line, "count=%s%[^\n]", cluster_count_str, &garbage_test) != 1
    //     ) 
    // {
    //     fclose(file);



    //     raise_error(NULL, arr, objects, 0, "Error: Couldn't parse a file.");
    //     return NO_CLUSTERS_LOADED;
    // }
    if (fscanf(file, "count=%f%[^\n]", &cluster_count_float, &garbage_test) != 1 || !is_integer(cluster_count_float)) {
        raise_error(file, arr, objects, 0, "Error: Couldn't parse a file.");
        return NO_CLUSTERS_LOADED;
    }


    // if (!parse_int(cluster_count_str, &cluster_count)) {
    //     raise_error(file, arr, objects, 0, "Error: Count has to be an integer.");
    //     return NO_CLUSTERS_LOADED;
    // }

    cluster_count = (int) cluster_count_float;

    if (cluster_count < 0) {
        raise_error(file, arr, objects, 0, "Error: Invalid cluster count.");
        return NO_CLUSTERS_LOADED;
    }

    (*arr) = malloc(cluster_count * sizeof(struct cluster_t));

    if (*arr == NULL) {
        raise_error(file, arr, objects, 0, "Error: Memory allocation failed.");
        return NO_CLUSTERS_LOADED;
    }

    if (method == K_MEANS) {
        (*objects) = malloc(cluster_count * sizeof(struct obj_t));

        if (*objects == NULL) {
            raise_error(file, arr, objects, 0, "Error: Memory allocation failed.");
            return NO_CLUSTERS_LOADED;
        }
    }

    for (int i = 0; i < cluster_count; i++) {
        float id, x, y;
        // char id_str[BUFFER_LENGTH], x_str[BUFFER_LENGTH], y_str[BUFFER_LENGTH];

        if (fscanf(file, "%f %f %f%[^\n]", &id, &x, &y, &garbage_test) != 3) {
            raise_error(file, arr, objects, i, "Error: Invalid format of input.");
            return NO_CLUSTERS_LOADED;
        }

        // if (
        //     fgets(line, LINE_LENGTH, file) == NULL || 
        //     sscanf(line, "%s %s %s%[^\n]", id_str, x_str, y_str, &garbage_test) != 3
        //     ) 
        // {
        //     raise_error(file, arr, objects, i, "Error: Invalid format of input.");
        //     return NO_CLUSTERS_LOADED;
        // }

        if (!is_integer(id) || !is_integer(x) || !is_integer(y)) {
            raise_error(file, arr, objects, i, "Error: Invalid format of input. Numbers have to be integers.");
            return NO_CLUSTERS_LOADED;
        }

        // if (!parse_int(id_str, &id) || !parse_int(x_str, &x) || !parse_int(y_str, &y)) {
        //     raise_error(file, arr, objects, i, "Error: Invalid format of input. Numbers have to be integers.");
        //     return NO_CLUSTERS_LOADED;
        // }

        // Checks an uniqueness of id, if we are using k-means method, we search array of objects.
        // Otherwise we search array of clusters.
        bool id_is_unique = method == K_MEANS ? !objets_exists(*objects, i, id) : is_unique(*arr, i, id);

        if (!id_is_unique) {
            raise_error(file, arr, objects, i, "Error: Object ID is not unique.");
            return NO_CLUSTERS_LOADED;
        }

        if (x < 0 || x > 1000 || y < 0 || y > 1000) {
            raise_error(file, arr, objects, i, "Error: Invalid coordinates. Coordinates must be in range <0, 1000>.");
            return NO_CLUSTERS_LOADED;
        }

        obj_t obj = { (int)id, x, y};

        // K-means method loads objects into array of objects, no cluster initialization is needed
        if (method == K_MEANS) {
            (*objects)[i] = obj;
            continue;
        } 

        init_cluster(&((*arr)[i]), !(method == K_MEANS));
        append_cluster(&((*arr)[i]), obj);
    }

    fclose(file);

    return cluster_count;
}

// Calcultes centroid object for a cluster.
void calculate_centroid(cluster_t *c, obj_t *centroid) {
    if (c == NULL || centroid == NULL) {
        return;
    }

    for (int i = 0; i < c->size; i++)
    {   
        centroid->x += c->obj[i].x;
        centroid->y += c->obj[i].y;
    }

    centroid->x /= c->size;
    centroid->y /= c->size;
}

// Picks random centroid from objects
void generate_random_ints(obj_t *objects, obj_t *centroids, int obj_count, int final_clusters) {
    if (obj_count < final_clusters) {
        return;
    }

    // Setting a seed for random generator
    srand(time(NULL));

    // Already generated idx's
    int *generated = malloc(final_clusters * sizeof(int));
    
    if (generated == NULL) {
        // If we can't allocate memory, we just pick first N objects as centroids
        for (int i = 0; i < final_clusters; i++)
        {
            centroids[i] = objects[i];
        }
        
        return;
    }

    int random_number;

    for (int i = 0; i < final_clusters; i++) {
        do
        {
            random_number = rand() % obj_count;
        } 
        while (array_contains(generated, i, random_number));

        generated[i] = random_number;
        centroids[i] = objects[random_number];
    }

    free(generated);
}

// Functions takes array of clusters and objects, index of current object, and number of clusters
int assign_to_cluster(obj_t *centroids, obj_t *obj, int cluster_count) {
    float min = INFINITY;
    int new_cluster_idx = INVALID_IDX;

    // Finds the nearest cluster based on the distance from centroid
    for (int i = 0; i < cluster_count; i++)
    {
        // If cluster is centroid, we skip him
        if (obj->id == centroids[i].id) {
            return INVALID_IDX;
        }

        float distance = obj_distance(obj, &centroids[i]);
        if (distance < min) {
            min = distance;
            new_cluster_idx = i;
        }
    }

    return new_cluster_idx;
}

// K-means clustering method
void k_means(cluster_t *clusters, obj_t *objects, int obj_count, int cluster_count) {
    // Array of indexes from array of objects, contains centroids for each cluster.
    // First centroid is centroid of first cluster in array of clusters etc.
    obj_t *centroids = malloc(cluster_count * sizeof(*centroids));

    // Generate k random indexes for centroids of each cluster
    generate_random_ints(objects, centroids, obj_count, cluster_count);

    // Assign these centroid objects to each cluster
    for (int i = 0; i < cluster_count; i++)
    {
        init_cluster(&clusters[i], 1);
        append_cluster(&clusters[i], centroids[i]);
    }

    // Loop over all objects and assign each objects to it's nearest cluster.
    // Distance is measured by distance to cluster's centroid.
    for (int i = 0; i < obj_count; i++)
    {
        int cluster_idx = assign_to_cluster(centroids, &objects[i], cluster_count);
        if (cluster_idx != INVALID_IDX) {
            append_cluster(&clusters[cluster_idx], objects[i]);
        }
    }

    // Allocating array of objects for swapping to another cluster
    swap_obj *swaps= malloc(obj_count * sizeof(*swaps));
    int swaps_count = 0;

    // Iterate over clusters, find new centeroids and reassign clusters, until no changes are made.
    while (true) {

        // recalculates centroids
        for (int i = 0; i < cluster_count; i++)
        {
            obj_t centroid = { .id = CENTROID_ID };
            calculate_centroid(&clusters[i], &centroid);
            centroids[i] = centroid;
        }

        // Reassign all objects after generating new centroids
        for (int i = 0; i < cluster_count; i++)
        {
            sort_cluster(&clusters[i]);
            for (int j = 0; j < clusters[i].size; j++)
            {
                obj_t *obj = &clusters[i].obj[j];
                int cluster_idx = assign_to_cluster(centroids, obj, cluster_count);

                if (cluster_idx != INVALID_IDX && cluster_idx != i) {
                    swaps[swaps_count++] = (swap_obj) { .obj=obj, .old_idx = i, .new_idx = cluster_idx };
                }
            }
        }

        // If there is nothing to swap, clustering is done
        if (!swaps_count) {
            break;
        }

        // Swapping objects from one cluster to another
        for (int i = 0; i < swaps_count; i++)
        {
            swap_obj *swap = &swaps[i];
            append_cluster(&clusters[swap->new_idx], *swap->obj);
            remove_object(&clusters[swap->old_idx], swap->obj->id);
        }

        swaps_count = 0;
    }

    free(centroids);
    free(swaps);
}

// Single linkage clustering method
void single_linkage(struct cluster_t *clusters, int cluster_count, int final_clusters)
{
    while (cluster_count > final_clusters) {
        int c1, c2;
        find_neighbours(clusters, cluster_count, &c1, &c2, &cluster_distance);
        merge_clusters(&clusters[c1], &clusters[c2]);
        cluster_count = remove_cluster(clusters, cluster_count, c2);
    }
}

// Complete linkage clustering method
void complete_linkage(struct cluster_t *clusters, int cluster_count, int final_clusters)
{
    while (cluster_count > final_clusters) {
        int c1, c2;
        find_neighbours(clusters, cluster_count, &c1, &c2, &cluster_distance_complete_linkage);
        merge_clusters(&clusters[c1], &clusters[c2]);
        cluster_count = remove_cluster(clusters, cluster_count, c2);
    }
}

// Average linkage clustering method
void average_linkage(struct cluster_t *clusters, int cluster_count, int final_clusters)
{
    while (cluster_count > final_clusters) {
        int c1, c2;
        find_neighbours(clusters, cluster_count, &c1, &c2, &cluster_distance_average_linkage);
        merge_clusters(&clusters[c1], &clusters[c2]);
        cluster_count = remove_cluster(clusters, cluster_count, c2);
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

ClusteringMethod parse_method(char *argument) {
    if (strcmp(argument, "-k") == 0) {
        return K_MEANS;
    }

    if (strcmp(argument, "-c") == 0) {
        return COMPLETE_LINKAGE;
    } 

    if (strcmp(argument, "-a") == 0) {
        return AVERAGE_LINKAGE; 
    } 

    return UNKNOWN;
}

int parse_arguments(char *argv[], int argc, Config *config) 
{
    // Helper variable for checking if floating point number is an integer
    float num;  
    ClusteringMethod method = UNKNOWN;

    switch (argc)
    {
    case 1:
        return print_error("Error: You have to specify the filename.\n");
    case 2:
        // This is here just for not triggering default case when N is not specified
        break;
    case 3:
        // First we check if third argument is a method parameter
        if ((method = parse_method(argv[2])) != UNKNOWN) {
            config->method = method;
            break;
        }

        // If it's not a method parameter, we try to parse it as N.
        // If argument is not a valid number, we convert it to 0 and program will throw an error.
        num = atof(argv[2]);
        if (is_integer(num)) {
            config->final_clusters = (int) num;
            break;
        }

        return print_error("Error: N has to be an integer.\n");
    case 4:
        // If method differs from default, than we dont need more arguments
        if (config->method != SINGLE_LINKAGE) {
            return print_error("Error: You Unknown argument.\n");
        }

        num = atof(argv[2]);
        if (is_integer(num)) {
            config->final_clusters = (int) num;
        }

        if ((method = parse_method(argv[2])) != UNKNOWN) {
            config->method = method;
            break;
        }

        return print_error("Error: Unknown argument.\n");
    default:
        return print_error("Error: Unknown argument.\n");
    }

    config->filename = argv[1];
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    struct cluster_t *clusters;
    // Array of objects for K-means method
    obj_t *objects;
    Config config = { .method = SINGLE_LINKAGE, .final_clusters = 1 };

    if (parse_arguments(argv, argc, &config) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    if (config.final_clusters < 1) {
        return print_error("Error: N parameter has to be integer greater than 0.\n");
    }

    int cluster_count = load_clusters(config.filename, &clusters, config.method, &objects);

    // If cluster array is NULL pointer, or method is k-means and objects array is NULL pointer, we throw an error.
    if (clusters == NULL || (config.method == K_MEANS && objects == NULL)) { 
        return EXIT_FAILURE;
    }

    if (config.final_clusters > cluster_count) {
        final_cleanup(clusters, objects, cluster_count);
        return print_error("Error: N parameter has to smaller or equal to count parameter.\n");
    }

    switch (config.method)
    {
    case SINGLE_LINKAGE:
        single_linkage(clusters, cluster_count, config.final_clusters);
        break;
    case COMPLETE_LINKAGE:
        complete_linkage(clusters, cluster_count, config.final_clusters);
        break;
    case K_MEANS:
        k_means(clusters, objects, cluster_count, config.final_clusters);
        break;
    case AVERAGE_LINKAGE:
        average_linkage(clusters, cluster_count, config.final_clusters);
        break; 
    default:
        final_cleanup(clusters, objects, cluster_count);
        return print_error("Error: Unknown method.\n");
    }

    print_clusters(clusters, config.final_clusters);

    final_cleanup(clusters, objects, config.final_clusters);
}
