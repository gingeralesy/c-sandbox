#ifndef STRING_H
#define STRING_H

#include "common.h"
#include "object.h"

typedef struct string_t
{
  OTrue obj;
  uint32_t len;
  const char *str;
} OString;

OString *    string_create(const char *str, uint32_t length);
uint64_t     string_hash  (OString *string);
const char * string_raw   (OString *string);

#endif // STRING_H
