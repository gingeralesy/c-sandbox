#include "hashmap.h"

// --- Private ---

typedef struct hash_map_node_t Node;

typedef struct hash_map_node_t
{
  uint64_t hash;
  Pointer key;
  Pointer value;
  Node *next;
} Node;

typedef struct hash_map_t
{
  uint32_t size;
  uint32_t capacity;
  uint32_t treshold;
  float load_factor;
  Node *nodes;
  uint64_t (*hash_f)  (Pointer);
  bool     (*equals_f)(Pointer, Pointer);
  void     (*free_f)  (Pointer, Pointer);
} HashMap;


static const uint32_t HASHMAP_MAX_CAPACITY = (1 << 30);
static const uint32_t HASHMAP_DEFAULT_INITIAL_CAPACITY = 16;
static const uint32_t HASHMAP_DEFAULT_LOAD_FACTOR = 0.75f;


static void     add_node     (HashMap *map, uint64_t hash,
                              Pointer key, Pointer value,
                              uint32_t index);
static Node *   create_node  (uint64_t hash, Pointer key, Pointer value,
                              Node *next);
static Pointer  get_for_null (HashMap *map);
static uint64_t truncate_hash(uint64_t key);
static uint32_t index_for    (uint64_t hash, uint32_t size);
static Pointer  put_for_null (HashMap *map, Pointer value);
static void     resize       (HashMap *map, uint32_t new_size);


void add_node(HashMap *map, uint64_t hash,
              Pointer key, Pointer value,
              uint32_t index)
{
  Node *new = NULL, *tmp = NULL;

  if (map == NULL)
    return;

  tmp = &(map->nodes[index]);
  new = create_node(tmp->hash, tmp->key, tmp->value, NULL);

  tmp->hash = hash;
  tmp->key = key;
  tmp->value = value;
  tmp->next = new;

  if (map->treshold <= map->size++)
    resize(map, 2 * map->capacity);
}

Node * create_node(uint64_t hash, Pointer key, Pointer value, Node *next)
{
  Node *node = (Node *)calloc(1, sizeof(Node));
  node->hash = hash;
  node->key = key;
  node->value = value;
  node->next = next;
  return node;
}

Pointer get_for_null(HashMap *map)
{
  Node *tmp = NULL;
  if (map == NULL)
    return NULL;

  for (tmp = &(map->nodes[0]); tmp != NULL; tmp = tmp->next)
  {
    if (tmp->key == NULL)
      return tmp->value;
  }
  return NULL;
}

uint64_t truncate_hash(uint64_t key)
{
  uint64_t hash = key;
  hash ^= (hash >> 20) ^ (hash >> 12);
  return hash ^ (hash >> 7) ^ (hash >> 4);
}

uint32_t index_for(uint64_t hash, uint32_t size)
{
  return hash & (size - 1);
}

Pointer put_for_null(HashMap *map, Pointer value)
{
  Node *tmp = NULL;
  if (map == NULL)
    return NULL;

  for (tmp = &(map->nodes[0]); tmp != NULL; tmp = tmp->next)
  {
    if (tmp->key == NULL)
    {
      Pointer old = tmp->value;
      tmp->value = value;
      return old;
    }
  }

  add_node(map, 0U, NULL, value, 0U);
  return NULL;
}

void resize(HashMap *map, uint32_t new_capacity)
{
  Node *new_nodes = NULL, *old_nodes = NULL;
  uint32_t old_capacity = 0U;
  if (map->capacity == HASHMAP_MAX_CAPACITY)
  {
    map->treshold = UINT_MAX;
    return;
  }

  old_nodes = map->nodes;
  old_capacity = map->capacity;
  map->nodes = NULL;

  new_nodes = (Node *)calloc(new_capacity, sizeof(Node));
  memcpy(new_nodes, old_nodes, sizeof(Node) * old_capacity);
  free(old_nodes);
  
  map->nodes = new_nodes;
  map->treshold = new_capacity * map->load_factor;
}

// --- Public ---

HashMap * hashmap_create(uint64_t (*hash_f)  (Pointer),
                         bool     (*equals_f)(Pointer, Pointer),
                         void     (*free_f)  (Pointer, Pointer),
                         uint32_t initial_capacity,
                         float load_factor)
{
  float lf = 0.f;
  uint32_t init = 0, capacity = 0;
  HashMap *map = NULL;

  if (load_factor == load_factor && 0.f < fabsf(load_factor))
    lf = load_factor;
  else
    lf = HASHMAP_DEFAULT_LOAD_FACTOR;

  if (0 < initial_capacity && initial_capacity <= HASHMAP_MAX_CAPACITY)
    init = initial_capacity;
  else if (HASHMAP_MAX_CAPACITY < initial_capacity)
    init = HASHMAP_MAX_CAPACITY;
  else
    init = HASHMAP_DEFAULT_INITIAL_CAPACITY;

  capacity = 1;
  while (capacity < init)
    capacity <<= 1;
  
  map = (HashMap *)calloc(1, sizeof(HashMap));
  map->hash_f = hash_f;
  map->equals_f = equals_f;
  map->free_f = free_f;
  map->capacity = capacity;
  map->load_factor = lf;
  map->treshold = (uint32_t)(capacity * lf);
  map->nodes = (Node *)calloc(capacity, sizeof(Node));

  return map;
}

void hashmap_destroy(HashMap *map)
{
  Node *node = NULL, *sibling = NULL, *next = NULL;
  uint32_t i = 0U;
  if (map == NULL)
    return;

  for (i = 0U; i < map->capacity; i++)
  {
    node = &(map->nodes[i]);
    sibling = node->next;
    while (next != NULL)
    {
      if (map->free_f)
        map->free_f(sibling->key, sibling->value);
      sibling->key = NULL;
      sibling->value = NULL;
      next = sibling->next;
    }

    if (map->free_f)
      map->free_f(node->key, node->value);
    node->key = NULL;
    node->value = NULL;
  }
}

Pointer hashmap_get(HashMap *map, Pointer key)
{
  Node *tmp = NULL;
  uint64_t hash = 0U;
  if (map == NULL)
    return NULL;
  if (key == NULL)
    return get_for_null(map);

  hash = truncate_hash(map->hash_f(key));
  for (tmp = &(map->nodes[index_for(hash, map->capacity)]);
       tmp != NULL;
       tmp = tmp->next)
  {
    if (tmp->hash == hash && (key == tmp->key || map->equals_f(key, tmp->key)))
      return tmp->value;
  }
  return NULL;
}

Pointer hashmap_put(HashMap *map, Pointer key, Pointer value)
{
  Node *node = NULL, *tmp = NULL;
  uint64_t hash = 0U;
  uint32_t index = 0U;

  if (map == NULL)
    return NULL;
  if (key == NULL)
    return put_for_null(map, value);

  node = (Node *)calloc(1, sizeof(Node));
  node->key = key;
  node->value = value;
  hash = truncate_hash(map->hash_f(key));

  index = index_for(hash, map->capacity);
  for (tmp = &(map->nodes[index]); tmp != NULL; tmp = tmp->next)
  {
    if (tmp->hash == hash && (key == tmp->key || map->equals_f(key, tmp->key)))
    {
      // Old value found
      Pointer old = tmp->value;
      tmp->value = value;
      return old;
    }
  }
  add_node(map, hash, key, value, index);
  return NULL;
}

uint32_t hashmap_size(HashMap *map)
{
  if (map == NULL)
    return 0U;
  return map->size;
}
