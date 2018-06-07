#ifndef HASHMAP_H
#define HASHMAP_H

#include "common.h"
#include "types.h"

typedef struct hash_map_t HashMap;

HashMap * hashmap_create (uint64_t (*hash_f)  (Pointer),
                          bool     (*equals_f)(Pointer, Pointer),
                          void     (*free_f)  (Pointer, Pointer),
                          uint32_t initial_capacity,
                          float load_factor);
void      hashmap_destroy(HashMap *map);
Pointer   hashmap_get    (HashMap *map, Pointer key);
Pointer   hashmap_put    (HashMap *map, Pointer key, Pointer value);
uint32_t  hashmap_size   (HashMap *map);

#endif // HASHMAP_H
