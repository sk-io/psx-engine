#ifndef _LEVEL_H
#define _LEVEL_H

#include "global.h"
#include "gfx.h"

#define LEVEL_MAX_PARTS 16
#define LEVEL_LEVEL1 1

// needs to be 4-aligned

typedef struct {
    short x, y, z, pad;
} lvl_vec;

typedef struct {
    u_char u, v;
} lvl_uv;

typedef struct {
    u_char num_tris;
    u_char num_quads;
    u_short tri_offset;
    u_short quad_offset;
} lvl_col_cell;

typedef struct {
    u_short pos[3];
    u_short uv[3];
    u_short norm;
    u_short clut;
} lvl_tri;

typedef struct {
    u_short pos[4];
    u_short uv[4];
    u_short norm;
    u_short clut;
} lvl_quad;

typedef struct {
    u_short byte_size;
    short colgrid_x;

    short colgrid_z;
    u_short colgrid_w;

    u_short colgrid_h;
    u_char colgrid_shift;
    u_char pad;

    u_short num_tris;
    u_short num_quads;

    u_short num_positions;
    u_short num_uvs;

    u_short num_normals;
    u_short pad1;

    long offset_x;
    long offset_y;
    long offset_z;

    lvl_tri *tris;
    lvl_quad *quads;
    lvl_vec *positions;
    lvl_uv *uvs;
    lvl_vec *normals;

    lvl_col_cell *col_cells;
    u_short *col_heap;
} lvl_part;

typedef struct {
    long x0, y0, z0;
    long x1, y1, z1;
} lvl_bbox;

typedef struct {
    u_long num_parts;
    lvl_part **parts; // part table, array of pointers since parts have variable size
    lvl_bbox *bounding_boxes; // bbox table, kept separate for caching reasons
} level_t;


struct level_ {
    level_t *current;
    int id;
    u_char active_parts[LEVEL_MAX_PARTS];
};

extern struct level_ level;

void change_level(int id);
void load_level(level_t* file);
void level_update_active_parts(VECTOR *player_pos);
void draw_level(draw_state_t *ds);

#endif
