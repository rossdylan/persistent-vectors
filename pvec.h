#ifndef _PVEC_H
#define _PVEC_H

#include <stdint.h>
#define BITS 5
#define WIDTH (1 << BITS)
#define MASK (WIDTH - 1)

struct PVecNode_ {
    struct PVecNode_ *children[WIDTH];
    void *elements[WIDTH];
};

typedef struct PVecNode_ PVecNode;

typedef struct{
    PVecNode *head;
    uint64_t length;
    uint64_t depth;
} PersistentVector;

PersistentVector *pvec_new(void);
void pvec_free(PersistentVector *vec);
PersistentVector *pvec_cons(PersistentVector *vec, void *data);
PersistentVector *pvec_assoc(PersistentVector *vec, uint64_t key, void *data);
void *pvec_nth(PersistentVector *vec, uint64_t key);
#endif
