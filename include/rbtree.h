#ifndef __RBT_H__
#define __RBT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define RBTREE_NULL_KEY (INT64_MIN)

typedef enum colour_e colour;
typedef struct rbt_node_t rbt_node;

void *     rbt_get(rbt_node *, int64_t);
size_t     rbt_get_all(rbt_node *, void ***, int64_t **);
rbt_node * rbt_get_first(rbt_node *, int64_t *, void **);
rbt_node * rbt_get_last(rbt_node *, int64_t *, void **);
rbt_node * rbt_get_next(rbt_node *, int64_t *, void **);
rbt_node * rbt_get_previous(rbt_node *, int64_t *, void **);
rbt_node * rbt_put(rbt_node *, int64_t, void *, size_t, void **);
rbt_node * rbt_remove(rbt_node *, int64_t, void **);
size_t     rbt_size(rbt_node *);
size_t     rbt_data_size(rbt_node *, int64_t);
void       rbt_free(rbt_node *, void (*)(void *));

#endif // __RBT_H__
