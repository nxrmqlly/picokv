# PicoKV

A simple append-only key value store and file format, written in C!

## Using the REPL

The `picokv` REPL is very simple and ships with minimal commands.
You can download a prebuilt binary from the
[Releases Page](https://github.com/nxrmqlly/picokv/releases) or build it from scratch (recommended, see below)

```sh
picokv [filename]

# for example,
picokv mycooldata.picokv
```

`filename` defaults to `data.picokv` if not specified.

Inside the REPL, you can use the following commands:
(commands are case insensitive)

| command               | use                                    |
| --------------------- | -------------------------------------- |
| `help`                | prints help                            |
| `quit` or `exit`      | exits the the repl                     |
| `get <key>`           | returns the value associated with key  |
| `set <key> = <value>` | sets or updates key's value            |
| `del <key>`           | removes a key and the associated value |

## Building from source

Clone the repo and use `make` to build.

Prerequisites:

- `clang`, or `gcc`
- `make`
- `git`

```sh
git clone https://github.com/nxrmqlly/picokv.git
cd picokv
make all # or "make CC=gcc all" if using gcc
```

## PicoKV as a library

I'd recommend you not to use PicoKV as a library in production code, as its still
pretty much a toy project, however if you still want to, header files are available:

- `include/picokv.h`: the API level functions
- `include/picokv_version.h`: source version of picokv
- `include/pkverr.h`: error definitions

## File Format

PicoKV data is canonically stored in `.picokv` or `.pkv` files.
All multi-byte integers are **little-endian** ONLY.

The contents are:

### Header (16 bytes)

```
  [ magic:    4 bytes  ]
  [ version:  2 bytes  ]
  [ reserved: 10 bytes ]
  ^ 16 bytes for file header
```

The magic number represents the first 4 bytes of any PicoKV file.
For picokv files is strictly `p1co` (ascii) as the bytes `0x70 0x31 0x63 0x6F`

And, the version occupies 2 bytes, it currently is `1` (decimal) as the bytes `0x01 0x00`

Ten bytes are reserved for future use, and they MUST be zero.

### Records (16 byte record header + data)

And following the header, "records" follow which describe an operation,
some metadata and then the data itself.

```
  [ operation:        1 byte  ]
  [ padding:          3 bytes ]
  [ crc32:            4 bytes ]
  [ k_size:           4 bytes ]
  [ v_size:           4 bytes ]
  ^ 16 bytes for the record header

  [ key:       <k_size> bytes ]
  [ value:     <v_size> bytes ]
  ^ k_size + v_size bytes for data
```

#### operation:

| operation | bytes  |
| --------- | ------ |
| SET       | `0x01` |
| DEL       | `0x02` |

Other operation codes should be parsed as invalid

Following 3 padding bytes MUST be zero.

#### crc32:

Next, 4 bytes are **CRC-32/ISO-HDLC** and it is used to verify the integrity of the data,
It is _little-endian_ and the value depends on the operation:

| operation | crc32 value             |
| --------- | ----------------------- |
| SET       | `crc32(key \|\| value)` |
| DEL       | `crc32(key)`            |

The `crc` field ONLY covers the key and value data bytes.

#### size of data:

Next, `k_size` and `v_size` are little-endian numbers of 4 bytes each (`uint32`),
they describe the LENGTH of the key and value. The `\0` NUL-terminator must not be added here.

The MAXIMUM values are:

```
  k_size: 4 * 1024 bytes        // 4 KiB
  v_size: 1 * 1024 * 1024 bytes // 1 MiB
```

| operation |       k_size       |        v_size         |
| --------- | :----------------: | :-------------------: |
| SET       | 0 < k_size <= 4096 | 0 < v_size <= 1048576 |
| DEL       | 0 < k_size <= 4096 |      v_size = 0       |

#### data:

Followed immediately by `k_size` bytes of the contents of `key` and `v_size` bytes of
the contents of `value`.

#### note:

> 1. For DEL operations: `v_size` MUST be zero, and hence `value` can be just left empty.
> 2. Any invalid CRC, makes the whole file invalid.

### Validity

A PicoKV file is invalid if:

- file header is malformed;
- its version is unsupported;
- magic number doesn't match;
- header's reserved space is not zero
- a record contains an unknown operation;
- a record is incomplete;
- a record's CRC does not match its data;
- a SET record has k_size == 0 || k_size > 4096 or v_size == 0 || v_size > 1048576;
- a DEL record has k_size == 0 || k_size > 4096 or v_size != 0;

### Record Semantics

Records are applied sequentially; for each key, the most recent `SET` or `DEL`
operation determines the key's current state.

- A SET operation creates or replaces a key's value.
- A DEL operation removes the key.
- A DEL operation for a key that doesn't exist is valid and has no effect.

**Subsequent records can be appended, following the same format as a record.**

## License

PicoKV is licensed under [GNU GPL v3 or later](./LICENSE)

![gplv3](https://www.gnu.org/graphics/gplv3-or-later.png)

---

[![Made by Human](https://madebyhuman.iamjarl.com/badges/made-white.svg)](https://madebyhuman.iamjarl.com)
