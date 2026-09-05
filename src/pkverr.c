// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026-present Ritam Das

#include "../include/pkverr.h"

const char *picokv_strerror(int err) {
  switch (err) {
  case 0:
    return "success";
  case PICOKV_ERR_SIZE:
    return "buffer size too small";
  case PICOKV_ERR_NOMEM:
    return "out of memory";
  case PICOKV_ERR_MAGIC:
    return "invalid file magic";
  case PICOKV_ERR_BADVER:
    return "invalid version";
  case PICOKV_ERR_BADOP:
    return "invalid operation";
  case PICOKV_ERR_BADCRC:
    return "invalid crc";
  case PICOKV_ERR_IO:
    return "I/O error";
  case PICOKV_ERR_EOF:
    return "reached EOF";
  case PICOKV_ERR_NOTFOUND:
    return "key not found";
  case PICOKV_INVALID:
    return "invalid data";
  case PICOKV_ERR_SYNTAX:
    return "syntax error";
  default:
    return "unknown error";
  }
}
