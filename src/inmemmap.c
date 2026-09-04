#include "inmemmap.h"
#include "pkverr.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define INDEXWRAP(key) (hash_fnv_1a(key) % CAPACITY)

typedef struct Node {
  char *key;
  char *value;
  struct Node *next;
} Node;

typedef struct HashMap {
  Node **buckets;
} HashMap;

uint64_t hash_fnv_1a(const char *c) {
  uint64_t h = 14695981039346656037ULL;
  while (*c) {
    h ^= (unsigned char)*c++;
    h *= 1099511628211ULL;
  }
  return h;
}

HashMap *create_map() {
  HashMap *map = malloc(sizeof *map);
  if (!map)
    return NULL;

  map->buckets = calloc(CAPACITY, sizeof *map->buckets);
  if (!map->buckets) {
    free(map);
    return NULL;
  }
  return map;
}

int map_put(HashMap *map, const char *key, const char *value) {
  size_t index = INDEXWRAP(key);
  Node *head = map->buckets[index];

  // check if key already exists
  while (head != NULL) {
    if (strcmp(head->key, key) == 0) {
      char *new_val = strdup(value);

      if (!new_val)
        return PICOKV_ERR_NOMEM;

      free(head->value);
      head->value = new_val;

      return 0;
    }
    head = head->next;
  }

  // if key doesnt exist
  Node *new = malloc(sizeof(Node));
  if (!new)
    return PICOKV_ERR_NOMEM;

  new->key = strdup(key);
  new->value = strdup(value);

  if (!new->key || !new->value) {
    free(new->key);
    free(new->value);
    free(new);
    return PICOKV_ERR_NOMEM;
  }

  new->next = map->buckets[index];
  map->buckets[index] = new;

  return 0;
}

const char *map_get(HashMap *map, const char *key) {
  size_t index = INDEXWRAP(key);
  Node *head = map->buckets[index];

  while (head != NULL) {
    if (strcmp(head->key, key) == 0) {
      return head->value;
    }
    head = head->next;
  }
  // notfound
  return NULL;
}

int map_del(HashMap *map, const char *key) {
  size_t index = INDEXWRAP(key);
  Node *curr = map->buckets[index];
  Node *prev = NULL;

  // if key already exists
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      if (prev == NULL) {
        // first node ever
        map->buckets[index] = curr->next;
      } else {
        prev->next = curr->next;
      }
      // yeet curr to oblivion
      free(curr->key);
      free(curr->value);
      free(curr);

      return 0;
    }

    prev = curr;
    curr = curr->next;
  }

  return PICOKV_ERR_NOTFOUND;
}

void free_map(HashMap *map) {
  for (size_t i = 0; i < CAPACITY; i++) {
    Node *head = map->buckets[i];
    while (head != NULL) {
      Node *temp = head;
      head = head->next;
      free(temp->key);
      free(temp->value);
      free(temp);
    }
  }
  free(map->buckets);
  free(map);
}
