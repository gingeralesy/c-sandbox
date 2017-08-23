#ifndef __OBJ_H__
#define __OBJ_H__

#include <stdint.h>

#define MAX_NAME_LEN (128)
#define MAX_CONSTRUCTORS (128)
#define MAX_SUPERCLASSES (128)
#define MAX_FIELDS (2048)

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

typedef struct object_base_class_t
{
  Class class;
  char name[MAX_NAME_LEN];
  void (*constructors)(void *)[MAX_CONSTRUCTORS];
  void (*destructor)(void);
} OTrue;

typedef struct object_number_class_t
{
  struct object_base_class_t;
  union {
    sint64_t i;
    uint64_t u;
    double f;
  } value;
} ONumber;

typedef struct object_variable_t
{
  OType type;
  char name[MAX_NAME_LEN];
  union {
    sint64_t i;
    uint64_t u;
    double f;
    char *str;
    void *(*func)(void *);
    OTrue obj;
  } data;
} OVar;

typedef struct object_class_t
{
  Class superclasses[MAX_SUPERCLASSES];
  char name[MAX_NAME_LEN];
  OVar fields[2048];
} Class;
  
#endif // __OBJ_H__
