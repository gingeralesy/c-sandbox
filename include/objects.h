#ifndef __OBJ_H__
#define __OBJ_H__

#include "common.h"

typedef enum object_type_e OType;
typedef struct object_class_t Class;
typedef struct object_base_class_t OTrue;
typedef struct object_variable_t OVar;

typedef enum object_type_e
{
  TYPE_INTEGER = 0,
  TYPE_UNSIGNED_INTEGER,
  TYPE_FLOAT,
  TYPE_STRING,
  TYPE_FUNCTION,
  TYPE_OBJECT
} OType;

typedef struct object_class_t
{
  Class *superclasses;
  char *name;
  OVar *fields;
} Class;

typedef struct object_base_class_t
{
  Class class;
  char *name;
  void (**constructors)(void *);
  void (*destructor)(void);
} OTrue;

typedef struct object_number_class_t
{
  struct object_base_class_t;
  union {
    int64_t i;
    uint64_t u;
    double f;
  } value;
} ONumber;

typedef struct object_variable_t
{
  OType type;
  char *name;
  union {
    int64_t i;
    uint64_t u;
    double f;
    char *str;
    void *(*func)(void *);
    OTrue obj;
  } data;
} OVar;
  
#endif // __OBJ_H__
