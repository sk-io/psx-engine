#ifndef _ENTITIES_H
#define _ENTITIES_H

#include <sys/types.h>
#include <libgte.H>

#include "model.h"
#include "gfx.h"

#define MAX_ENTITIES 16

#define ENT_TEST 1

typedef struct {
    int type;
    VECTOR pos;
    SVECTOR rot;
    model_t *model;
} entity_t;

extern entity_t entities[];

void ent_unload_all();
void ent_update();
void ent_draw(draw_state_t *ds);

#endif
