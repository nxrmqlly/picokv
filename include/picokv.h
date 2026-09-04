#pragma once

#include <stddef.h>

typedef struct PicoKV PicoKV;

int picokv_open(PicoKV *pkv, const char *path);
void picokv_close(PicoKV *pkv);

int picokv_set(PicoKV *pkv, const char *key, const char *val);
int picokv_get(PicoKV *pkv, const char *key, char *val, size_t sz);
int picokv_del(PicoKV *pkv, const char *key);
