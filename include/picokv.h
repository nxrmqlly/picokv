// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026-present Ritam Das

#pragma once

#include <stddef.h>

typedef struct PicoKV PicoKV;

PicoKV *picokv_new(void);
int picokv_open(PicoKV *pkv, const char *path);
void picokv_close(PicoKV *pkv);

int picokv_set(PicoKV *pkv, const char *key, const char *val);
int picokv_get(PicoKV *pkv, char *out, const char *key, size_t sz);
int picokv_get_size(PicoKV *pkv, size_t *out_sz, const char *key);
int picokv_del(PicoKV *pkv, const char *key);
