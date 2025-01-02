#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// 哈希表节点
typedef struct hash_node {
    void* key;
    void* value;
    struct hash_node* next;
} hash_node_t;

// 哈希表
#define HASH_SIZE 256

typedef struct {
    hash_node_t* buckets[HASH_SIZE];
    void (*free_value)(void*);
} hash_table_t;

uint32_t hash_function(const void* key);

bool compare_keys(const void* key1, const void* key2);

hash_table_t* hash_table_create(void (*free_value)(void*));

void hash_table_destroy(hash_table_t* ht);

bool hash_table_insert(hash_table_t* ht, void* key, void* value);

void* hash_table_lookup(hash_table_t* ht, void* key);