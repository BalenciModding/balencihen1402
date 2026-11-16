#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct bh_db bh_db;
typedef int (*bh_mem_fn)(void *, uint64_t, unsigned char *, size_t, int);
bh_db *bh_db_open(const char *, const char *, const char *, const char *, char *, size_t);
void bh_db_close(bh_db *);
size_t bh_db_count(const bh_db *);
const char *bh_db_name(const bh_db *, size_t);
int bh_db_supported(const bh_db *, size_t);
int bh_db_enabled(const bh_db *, size_t);
int bh_db_toggle(bh_db *, size_t, bh_mem_fn, void *, char *, size_t);
