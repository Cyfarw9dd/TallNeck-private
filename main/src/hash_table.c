/*
 * Copyright 2025 Cyfarwydd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hash_table.h"

uint32_t hash_function(const void* key) {
    // 简单的整数哈希函数
    return *((int*)key) % HASH_SIZE;
}

bool compare_keys(const void* key1, const void* key2) {
    return *((int*)key1) == *((int*)key2);
}

hash_table_t* hash_table_create(void (*free_value)(void*)) {
    hash_table_t* ht = malloc(sizeof(hash_table_t));
    if (ht) {
        for (int i = 0; i < HASH_SIZE; i++) {
            ht->buckets[i] = NULL;
        }
        ht->free_value = free_value;
    }
    return ht;
}

void hash_table_destroy(hash_table_t* ht) {
    if (ht) {
        for (int i = 0; i < HASH_SIZE; i++) {
            hash_node_t* current = ht->buckets[i];
            while (current) {
                hash_node_t* next = current->next;
                if (ht->free_value) {
                    ht->free_value(current->value);
                }
                free(current->key);
                free(current);
                current = next;
            }
        }
        free(ht);
    }
}

bool hash_table_insert(hash_table_t* ht, void* key, void* value) {
    if (!ht || !key || !value) return false;
    
    uint32_t index = hash_function(key);
    hash_node_t* node = malloc(sizeof(hash_node_t));
    if (!node) return false;
    
    node->key = key;
    node->value = value;
    node->next = ht->buckets[index];
    ht->buckets[index] = node;
    
    return true;
}

void* hash_table_lookup(hash_table_t* ht, void* key) {
    if (!ht || !key) return NULL;
    
    uint32_t index = hash_function(key);
    hash_node_t* current = ht->buckets[index];
    
    while (current) {
        if (compare_keys(current->key, key)) {
            return current->value;
        }
        current = current->next;
    }
    
    return NULL;
}