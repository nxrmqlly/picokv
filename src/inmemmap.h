#pragma once

#include <stdint.h>
#define CAPACITY 500000

typedef struct Node Node;
typedef struct HashMap HashMap;

uint64_t hash_fnv_1a(const char *c);

HashMap *create_map();
int map_put(HashMap *map, const char *key, const char *value);
const char *map_get(HashMap *map, const char *key);
int map_del(HashMap *map, const char *key);
void free_map(HashMap *map);
