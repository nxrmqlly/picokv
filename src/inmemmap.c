#include "inmemmap.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct HashMap {
  Node **buckets;
} HashMap;

typedef struct Node {
  char *key;
  char *value;
  struct Node *next;
} Node;

uint64_t hash_fnv_1a(const char *c) {
  uint64_t h = 14695981039346656037ULL;
  while (*c) {
    h ^= (unsigned char)*c++;
    h *= 1099511628211ULL;
  }
  return h;
};

HashMap *create_map() {
  HashMap *map = malloc(sizeof *map);
  map->buckets = (Node **)calloc(CAPACITY, sizeof(Node *));
  return map;
}

void map_put(HashMap *map, const char *key, const char *value) {
  uint64_t index = hash_fnv_1a(key);
  Node *head = map->buckets[index];

  // check if key already exists
  while (head != NULL) {
    if (strcmp(head->key, key) == 0) {
      free(head->value);
      head->value = strdup(value);
      return;
    }
    head = head->next;
  }

  // if key doesnt exist
  Node *new = malloc(sizeof(Node));
  new->key = strdup(key);
  new->value = strdup(value);
  new->next = map->buckets[index];
  map->buckets[index] = new;
}

const char *map_get(HashMap *map, const char *key) {
  uint64_t index = hash_fnv_1a(key);
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

void free_map(HashMap *map) {
  for (int i = 0; i < CAPACITY; i++) {
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
