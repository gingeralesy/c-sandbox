#ifndef STRING_H
#define STRING_H

#include "common.h"
#include "object.h"

typedef struct string_t
{
  Object obj;
  uint32_t len;
  const char *str;
} String;

String *     string_create(const char *str, uint32_t length);
uint64_t     string_hash  (String *string);
const char * string_raw   (String *string);

#endif // STRING_H
