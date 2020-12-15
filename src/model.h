#ifndef _MODEL_H
#define _MODEL_H

#include "global.h"
#include "gfx.h"

typedef struct {
    u_char u, v;
} mdl_uv;

typedef struct {
    u_short pos[3];
    u_short uvs[3];
} mdl_tri;

typedef struct {
    u_short num_vertices;
    u_short num_tris;
    u_short num_frames;
    u_short pad;
    
    SVECTOR *pos;  // [num_frames][num_vertices]
    mdl_uv *uvs;    // [num_vertices]
    mdl_tri *tris; // [num_tris]
} model_t;

extern model_t *model_lurker;

void load_model(model_t *mdl);
void draw_model(model_t *mdl, draw_state_t *draw, int frame);

#endif
