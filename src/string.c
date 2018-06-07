#include "string.h"


static uint64_t djb2(const uint8_t *str);


uint64_t djb2(const uint8_t *str)
{
  uint64_t hash = 5381UL;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) ^ c; // hash(i - 1) * 33 ^ str[i]

  return hash;
}


String * string_create(const char *str, uint32_t length)
{
  String *self = (String *)calloc(1, sizeof(String));
  object_fill((Object *)self, djb2((const uint8_t *)str));
  self->len = (0 < length ? length : strlen(str));
  self->str = str;

  return self;
}

uint64_t string_hash(String *string)
{
  if (string == NULL)
    return 0UL;
  return ((Object *)string)->hash;
}

const char * string_raw(String *string)
{
  if (string == NULL)
    return NULL;
  return string->str;
}
