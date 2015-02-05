#include "pvec.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/**
 * These functions are the internal API used to interact with the 
 * vector metadata, and internal bitpartitioned trie.
 */
PVecNode *pvec_node_new(void);
PVecNode *pvec_node_copy(PVecNode *node);
PersistentVector *pvec_copy(PersistentVector *vec);
PersistentVector *pvec_set(PersistentVector *vec, uint64_t key, void *data, bool insert);


PVecNode *pvec_node_new(void) {
    PVecNode *new = NULL;
    if((new = malloc(sizeof(PVecNode))) == NULL) {
        perror("malloc");
        goto pvec_node_new_return;
    }
    for(uint64_t i = 0; i < WIDTH; i++) {
        new->children[i] = NULL;
        new->elements[i] = NULL;
    }
pvec_node_new_return:
    return new;
}

PVecNode *pvec_node_copy(PVecNode *node) {
    PVecNode *new_node = NULL;
    if((new_node = malloc(sizeof(PVecNode))) == NULL) {
        perror("malloc");
        goto pvec_node_copy_return;
    }
    for(uint64_t i = 0; i < WIDTH; i++) {
        new_node->children[i] = node->children[i];
        new_node->elements[i] = node->elements[i];
    }
pvec_node_copy_return:
    return new_node;
}

PersistentVector *pvec_copy(PersistentVector *vec) {
    PersistentVector *copy = NULL;
    if((copy = malloc(sizeof(PersistentVector))) == NULL) {
        perror("malloc");
        goto pvec_copy_return;
    }
    copy->depth = vec->depth;
    copy->length = vec->length;
    if((copy->head = pvec_node_copy(vec->head)) == NULL) {
        perror("pvec_node_copy");
        free(copy);
        copy = NULL;
        goto pvec_copy_return;
    }
pvec_copy_return:
    return copy;
}

PersistentVector *pvec_set(PersistentVector *vec, uint64_t key, void *data, bool insert) {
    uint64_t shift = BITS * (vec->depth + 1);
    uint64_t max_size = 1 << (5 * shift); //TODO(rossdylan) This needs to be verified
    PersistentVector *copy = NULL;

    // Check our bounds for non-insert sets
    if(key > vec->length && !insert) {
        goto pvec_set_return;
    }

    // Create a copy of the root node from which we will base our insert/update
    // operations off of
    if((copy = pvec_copy(vec)) == NULL) {
        perror("pvec_copy");
        goto pvec_set_return;
    }

    // Check for root overflow, and if so expand the trie by 1 level
    if(copy->length == max_size) {
        PVecNode *new_root = NULL;
        if((new_root = pvec_node_new()) == NULL) {
            perror("pvec_node_new");
            free(copy->head);
            free(copy);
            copy = NULL;
            goto pvec_set_return;
        }
        new_root->children[0] = copy->head;
        copy->head = new_root;
        copy->depth++;
        shift = BITS * (vec->depth + 1);
    }

    // Now that we have dealt with our edgecases and checks do the set
    // operation.
    PVecNode *cur = NULL;
    PVecNode *prev = copy->head;
    uint64_t index = 0;

    for(uint64_t level = shift; level > 0; level -= BITS) {
        index = (key >> level) & MASK;
        cur = prev->children[index];
        if(cur == NULL) {
            // We are doing an insert, so we need to make new nodes if we need them
            if(insert) {
                if((prev->children[index] = pvec_node_new()) == NULL) {
                    perror("pvec_node_new");
                    //TODO malloc failed somwhere, clean our shit up
                    goto pvec_set_return;
                }
                cur = prev->children[index];
                prev = cur;
                continue;
            }
            else {
                // TODO(rossdylan) This is an error path, clean this shit up
                printf("I reached an error path\n");
                goto pvec_set_return;
            }
        }
        prev->children[index] = pvec_node_copy(cur);
        prev = prev->children[index];
    }
    prev->elements[key & MASK] = data;
    if(insert) {
        copy->length++;
    }
pvec_set_return:
    return copy;
}

PersistentVector *pvec_new(void) {
    PersistentVector *vec = NULL;
    if((vec = malloc(sizeof(PersistentVector))) == NULL) {
        perror("malloc");
        goto pvec_new_return;
    }
    if((vec->head = pvec_node_new()) == NULL) {
        perror("pvec_node_new");
        free(vec);
        vec = NULL;
        goto pvec_new_return;
    }
    vec->depth = 0;
    vec->length = 0;
pvec_new_return:
    return vec;
}


/**
 * The following implementations are a part of the externally available
 * PersistentVector API
 */

PersistentVector *pvec_cons(PersistentVector *vec, void *data) {
    return pvec_set(vec, vec->length, data, true);
}

PersistentVector *pvec_assoc(PersistentVector *vec, uint64_t key, void *data) {
    return pvec_set(vec, key, data, false);
}

void *pvec_nth(PersistentVector *vec, uint64_t key) {
    uint64_t shift = BITS * (vec->depth + 1);
    PVecNode *next = NULL;
    PVecNode *prev = vec->head;
    uint64_t index = 0;
    for(uint64_t level = shift; level > 0; level -= BITS) {
        index = (key >> level) & MASK;
        next = prev->children[index];
        prev = next;
    }
    return prev->elements[key & MASK];
}

/**
 * Test functions
 */
void **pvec_to_array(PersistentVector *vec) {
    void **array = NULL;
    if((array = calloc(sizeof(void *), vec->length)) == NULL) {
        perror("calloc");
        goto pvec_to_array_return;
    }
    for(uint64_t i = 0; i < vec->length; i++) {
        array[i] = pvec_nth(vec, i);
    }
pvec_to_array_return:
    return array;
}

void print_pvec(PersistentVector *vec) {
    void **array = pvec_to_array(vec);
    printf("[");
    for(uint64_t i = 0; i < vec->length; i++) {
        printf("%s, ", (char *)array[i]);
    }
    printf("]\n");
    free(array);
}

int main(int argc, char **argv) {
    PersistentVector **stages = NULL;
    if((stages = calloc(sizeof(PersistentVector), argc + 1)) == NULL) {
        perror("calloc");
        return 1;
    }
    stages[0] = pvec_new();
    for(uint64_t i = 1; i < argc+1; i++) {
        stages[i] = pvec_cons(stages[i-1], argv[i-1]);
    }

    for(uint64_t i = 0; i < argc+1; i++) {
        print_pvec(stages[i]);
    }
}
