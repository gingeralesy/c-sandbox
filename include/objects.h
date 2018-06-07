#ifndef __OBJ_H__
#define __OBJ_H__

#include "common.h"

typedef enum object_type_e OType;
typedef struct object_class_t Class;
typedef struct object_base_class_t OTrue;
typedef struct object_variable_t OVar;

typedef enum object_type_e
{
  TYPE_UNDEFINED = 0,
  TYPE_INTEGER,
  TYPE_UNSIGNED_INTEGER,
  TYPE_FLOAT,
  TYPE_FUNCTION,
  TYPE_OBJECT
} OType;

// Hold up, something is way off here. I think classes and objects are getting mixed up.

typedef struct object_class_t
{
  Class *superclasses;
  const char *class_name;
  OVar *fields;
} Class;

typedef struct object_base_class_t
{
  Class class;
  void (**constructors)(OTrue *, ONumber *argc, OVar *argv[]);
  void (*destructor)(OTrue *);
} OTrue;

typedef struct object_number_class_t
{
  OTrue base;
  union {
    int64_t i;
    uint64_t u;
    double f;
  } value;
} ONumber;

typedef struct object_variable_t
{
  OType type;
  union {
    int64_t i;
    uint64_t u;
    double f;
    Pointer (*func)(Pointer);
    OTrue *obj;
  } data;
} OVar;

#endif // __OBJ_H__
