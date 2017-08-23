#include "rbtree.h"

#include <stdint.h>

#include "common.h"

typedef enum colour_e
{
  RED = 0,
  BLACK = 1
} colour;

typedef struct rbt_node_t
{
  struct rbt_node_t *parent;
  struct rbt_node_t *left;
  struct rbt_node_t *right;
  colour colour;
  uint64_t size;
  int64_t key;
  size_t data_size;
  void * data;
} rbt_node;


static rbt_node * left(rbt_node *);
static rbt_node * right(rbt_node *);
static rbt_node * parent(rbt_node *);

static SB_bool    is_red(rbt_node *);
static uint64_t   size(rbt_node *);
static void       flip_colours(rbt_node *);
static rbt_node * rotate_left(rbt_node *);
static rbt_node * rotate_right(rbt_node *);
static rbt_node * move_red_left(rbt_node *);
static rbt_node * move_red_right(rbt_node *);
static rbt_node * balance(rbt_node *);
static rbt_node * put_node(rbt_node *, rbt_node *, void **);
static rbt_node * remove_node(rbt_node *, int64_t, void **);
static rbt_node * remove_first(rbt_node *);
static void       node_free(rbt_node *, void (*)(void *));

// --- Private ---

rbt_node * left(rbt_node *node)
{
  return node ? node->left : NULL;
}

rbt_node * right(rbt_node *node)
{
  return node ? node->right : NULL;
}

rbt_node * parent(rbt_node *node)
{
  return node ? node->parent : NULL;
}

rbt_node * find(rbt_node *root, int64_t key)
{
  rbt_node *n = root;
  while (n)
  {
    if (key == n->key)
      break;
    if (key < n->key)
      n = left(n);
    else
      n = right(n);
  }
  return n;
}

SB_bool is_red(rbt_node *node)
{
  return (node && node->colour == RED);
}

uint64_t size(rbt_node *node)
{
  return node ? node->size : 0;
}

void flip_colours(rbt_node *node)
{
  rbt_node *_left = NULL;
  rbt_node *_right = NULL;
  
  if (is_red(node))
    node->colour = BLACK;
  else
    node->colour = RED;

  _left = left(node);
  if (_left)
  {
    if (is_red(_left))
      _left->colour = BLACK;
    else
      _left->colour = RED;
    node->left = _left;
  }
  
  _right = right(node);
  if (_right)
  {
    if (is_red(_right))
      _right->colour = BLACK;
    else
      _right->colour = RED;
    node->right = _right;
  }
}

rbt_node * rotate_left(rbt_node *node)
{
  rbt_node *n = node;
  if (right(node))
  {
    rbt_node *_parent = parent(node);
    rbt_node *old = right(node); // old right

    node->right = old->left;
    old->left = node;

    if (is_red(left(old)))
      old->colour = RED;
    else
      old->colour = BLACK;

    left(old)->colour = RED;
    old->size = node->size;
    node->size = size(left(node)) + size(right(node)) + 1;
    
    old->parent = _parent;
    if (left(_parent) == node)
      _parent->left = old;
    else
      _parent->right = old;
    node->parent = old;

    n = old;
  }
  return n;
}

rbt_node * rotate_right(rbt_node *node)
{
  rbt_node *n = node;
  if (left(node))
  {
    rbt_node *_parent = parent(node);
    rbt_node *old = left(node); // old left

    node->left = old->right;
    old->right = node;

    if (is_red(right(old)))
      old->colour = RED;
    else
      old->colour = BLACK;

    right(old)->colour = RED;
    old->size = node->size;
    node->size = size(right(node)) + size(left(node)) + 1;
    
    old->parent = _parent;
    if (right(_parent) == node)
      _parent->right = old;
    else
      _parent->left = old;
    node->parent = old;

    n = old;
  }
  return n;
}

rbt_node * move_red_left(rbt_node *node)
{
  flip_colours(node);
  if (right(node) && is_red(left(right(node))))
  {
    node->right = rotate_right(right(node));
    node = rotate_left(node);
    flip_colours(node);
  }
  return node;
}

rbt_node * move_red_right(rbt_node *node)
{
  flip_colours(node);
  if (left(node) && is_red(left(left(node))))
  {
    node = rotate_right(node);
    flip_colours(node);
  }
  return node;
}

rbt_node * balance(rbt_node *root)
{
  rbt_node *node = root;
  if (is_red(right(node)))
    node = rotate_left(node);
  if (left(node) && is_red(left(node)) && is_red(left(left(node))))
    node = rotate_right(node);
  if (is_red(left(node)) && is_red(right(node)))
    flip_colours(node);

  node->size = size(left(node)) + size(right(node)) + 1;
  return node;
}

rbt_node * put_node(rbt_node *root, rbt_node *node, void **old_data)
{
  rbt_node *r = root;
  if (r)
  {
    if (r->key < node->key)
    {
      r->right = put_node(right(r), node, old_data);
    }
    else if (node->key < r->key)
    {
      r->left = put_node(left(r), node, old_data);
    }
    else
    {
      if (!r->data || r->data_size != node->data_size ||
          (r->data != node->data &&
           memcmp(r->data,node->data,min(r->data_size, node->data_size)) != 0))
      {
        void *old = node->data;
        r->data = node->data;
        r->data_size = node->data_size;

        if (old_data)
          (*old_data) = old;
      }

      node->data = NULL;
      node->data_size = 0;
      node_free(node, NULL);
    }

    if (is_red(right(node)) && !is_red(left(node)))
      node = rotate_left(node);
    if (left(node) && is_red(left(node)) && is_red(left(left(node))))
      node = rotate_right(node);
    if (is_red(left(node)) && is_red(right(node)))
      flip_colours(node);

    node->size = size(left(node)) + size(right(node)) + 1;
  }
  else
  {
    node->colour = RED;
    r = node;
  }

  return r;
}

rbt_node * remove_node(rbt_node *root, int64_t key, void **data)
{
  rbt_node *old = NULL;
  rbt_node *node = root;
  if (root->key == key && data)
  {
    (*data) = root->data;
    root->data = NULL;
  }

  if (key < node->key)
  {
    rbt_node *_left = NULL;
    
    old = left(node); // old left
    if (!old)
      return NULL;

    if (!is_red(old) && (!old || !is_red(left(old))))
    {
      node = move_red_left(node);
      old = left(node);
    }

    _left = remove_node(old, key, data);

    _left->parent = node;
    node->left = _left;
  }
  else
  {
    if (is_red(left(node)))
      node = rotate_right(node);
    if (key == node->key && right(node))
      return NULL; // Should not happen

    if (!is_red(right(node)) && (!right(node) || !is_red(left(right(node)))))
      node = move_red_right(node);

    if (key == node->key)
    {
      rbt_node *smallest = remove_first(right(node));
      old = node;
      node = smallest;
    }
    else
    {
      rbt_node *_right = remove_node(right(node), key, data);
      node->right = _right;
    }
  }

  node = balance(node);

  if (old && key == old->key)
  {
    node_free(old, NULL);
    old = NULL;
  }

  return node;
}

rbt_node * remove_first(rbt_node *root)
{
  rbt_node *node = NULL;
  rbt_node *_left = NULL;
  if (!left(root))
  {
    root->parent = NULL;
    return root;
  }

  node = root;
  if (left(node) && !is_red(left(node)) && !is_red(left(left(node))))
    node = move_red_left(node);

  _left = remove_first(left(node));
  node->left = _left;
  return balance(node);
}

void node_free(rbt_node *node, void (*data_free)(void *))
{
  void *data = node->data;
  node->data = NULL;

  if (data && data_free)
    data_free(data);

  node->parent = NULL;
  node->left = NULL;
  node->right = NULL;
  node->colour = RED;
  node->key = 0;
  node->data_size = 0;

  free(node);
}

// --- Public ---

void * rbt_get(rbt_node *root, int64_t key)
{
  rbt_node *n = find(root, key);
  return n ? n->data : NULL;
}

size_t rbt_get_all(rbt_node *root, void ***data, int64_t **keys)
{
  size_t count = 0;
  rbt_node *node = NULL;
  void **data_array = NULL;
  int64_t *key_array = NULL;
  if (root)
  {
    data_array = (void **)calloc(count,sizeof(void *));
    key_array = (int64_t *)calloc(count,sizeof(int64_t));

    node = rbt_get_first(root, NULL, NULL);
    while (node)
    {
      data_array[count] = node->data;
      key_array[count] = node->key;
      count += 1;
      node = rbt_get_next(node, NULL, NULL);
    }
  }
  
  if (data)
    (*data) = data_array;
  else
    free(data_array);
  if (keys)
    (*keys) = key_array;
  else
    free(key_array);

  return count;
}

rbt_node * rbt_get_first(rbt_node *root, int64_t *key, void **data)
{
  rbt_node *n = root;
  while (n && left(n))
    n = left(n);
  if (key)
  {
    if (n)
      (*key) = n->key;
    else
      key = NULL;
  }
  if (data)
  {
    if (n)
      (*data) = n->data;
    else
      key = NULL;
  }
  return n;
}

rbt_node * rbt_get_last(rbt_node *root, int64_t *key, void **data)
{
  rbt_node *n = root;
  while (n && right(n))
    n = right(n);
  if (key)
  {
    if (n)
      (*key) = n->key;
    else
      key = NULL;
  }
  if (data)
  {
    if (n)
      (*data) = n->data;
    else
      key = NULL;
  }
  return n;
}

rbt_node * rbt_get_next(rbt_node *node, int64_t *key, void **data)
{
  rbt_node *n = node;
  rbt_node *p = NULL;
  rbt_node *r = NULL;
  while (n)
  {
    p = parent(n);
    r = right(n);
    if (r && node->key < r->key)
    {
      n = r;
      while (left(n))
        n = left(n);
      break;
    }
    n = p;
    if (node->key < p->key)
      break;
  }

  if (key)
  {
    if (n)
      (*key) = n->key;
    else
      key = NULL;
  }
  if (data)
  {
    if (n)
      (*data) = n->data;
    else
      key = NULL;
  }
  return n;
}

rbt_node * rbt_get_previous(rbt_node *node, int64_t *key, void **data)
{
  rbt_node *n = node;
  rbt_node *p = NULL;
  rbt_node *l = NULL;
  while (n)
  {
    p = parent(n);
    l = left(n);
    if (l && node->key < l->key)
    {
      n = l;
      while (right(n))
        n = right(n);
      break;
    }
    n = p;
    if (node->key < p->key)
      break;
  }

  if (key)
  {
    if (n)
      (*key) = n->key;
    else
      key = NULL;
  }
  if (data)
  {
    if (n)
      (*data) = n->data;
    else
      key = NULL;
  }
  return n;
}

rbt_node * rbt_put(rbt_node *root, int64_t key, void *data, size_t data_size,
                   void **old_data)
{
  rbt_node *r = NULL;
  rbt_node *new_node = NULL;

  new_node = (rbt_node *)calloc(1,sizeof(rbt_node));
  new_node->key = key;
  new_node->data = data;
  new_node->data_size = data_size;

  if (old_data)
    (*old_data) = NULL;

  r = put_node(root, new_node, old_data);

  if (0 < rbt_size(r))
    r->colour = BLACK;
    
  return r;
}

rbt_node * rbt_remove(rbt_node *root, int64_t key, void **data)
{
  rbt_node *r = NULL;
  if (find(root, key))
  {
    r = root;
    if (!is_red(left(r)) && !is_red(right(r)))
      r->colour = RED;

    r = remove_node(r, key, data);
    if (0 < size(r))
      r->colour = BLACK;
  }
  return r;
}

size_t rbt_size(rbt_node *node)
{
  return size(node);
}

size_t rbt_data_size(rbt_node *root, int64_t key)
{
  rbt_node *n = find(root, key);
  if (!n)
    return -1;
  return n->data_size;
}

void rbt_free(rbt_node *root, void (*data_free)(void *))
{
  rbt_node *_parent = root;
  rbt_node *_left = NULL;
  rbt_node *_right = NULL;
  rbt_node *_tmp = NULL;
  while (_parent)
  {
    _left = left(_parent);
    _right = right(_parent);
    if (_left || _right)
    {
      while (left(_left) || right(_left) || left(_right) || right(_right))
      {
	if (left(_left))
	  _parent = left(_left);
	else if (right(_left))
	  _parent = right(_left);
	else if (left(_right))
	  _parent = left(_right);
	else if (right(_right))
	  _parent = right(_right);
	
        _left = left(_parent);
        _right = right(_parent);
      }

      if (_left)
      {
	_parent->left = NULL;
	_left->parent = NULL;
	node_free(_left, data_free);
	_left = NULL;
      }
      
      if (_right)
      {
	_parent->right = NULL;
	_right->parent = NULL;
	node_free(_right, data_free);
	_right = NULL;
      }
    }
    else
    {
      _tmp = _parent;
      _parent = parent(_parent);

      _tmp->parent = NULL;
      node_free(_tmp, data_free);
    }
  }
}
