#include "col.h"

#include <sys/types.h>

#define COL_EXTRA -20000
#define MAX_FALL_DIST 600
#define MAX_STEP_DIST 600

static long sign(long p1x, long p1y, long p2x, long p2y, long p3x, long p3y) {
    return (p1x - p3x) * (p2y - p3y) - (p2x - p3x) * (p1y - p3y);
}

static u_char is_inside_tri(short x, short z, lvl_tri *tri) {
    // u_char neg, pos;
    // long d0 = sign(x, z, tri->verts[0].x, tri->verts[0].z, tri->verts[1].x, tri->verts[1].z);
    // long d1 = sign(x, z, tri->verts[1].x, tri->verts[1].z, tri->verts[2].x, tri->verts[2].z);
    // long d2 = sign(x, z, tri->verts[2].x, tri->verts[2].z, tri->verts[0].x, tri->verts[0].z);

    // neg = (d0 < COL_EXTRA) || (d1 < COL_EXTRA) || (d2 < COL_EXTRA);
    // pos = (d0 > COL_EXTRA) || (d1 > COL_EXTRA) || (d2 > COL_EXTRA);

    // return !(neg && pos);
}

static u_char is_inside_quad(short x, short z, lvl_part *part, lvl_quad *quad) {
    u_char neg, pos;
    long d0 = sign(x, z, 
        part->positions[quad->pos[0]].x, part->positions[quad->pos[0]].z,
        part->positions[quad->pos[1]].x, part->positions[quad->pos[1]].z);
    long d1 = sign(x, z, 
        part->positions[quad->pos[1]].x, part->positions[quad->pos[1]].z,
        part->positions[quad->pos[2]].x, part->positions[quad->pos[2]].z);
    long d2 = sign(x, z, 
        part->positions[quad->pos[2]].x, part->positions[quad->pos[2]].z,
        part->positions[quad->pos[3]].x, part->positions[quad->pos[3]].z);
    long d3 = sign(x, z, 
        part->positions[quad->pos[3]].x, part->positions[quad->pos[3]].z,
        part->positions[quad->pos[0]].x, part->positions[quad->pos[0]].z);

    neg = (d0 < COL_EXTRA) || (d1 < COL_EXTRA) || (d2 < COL_EXTRA) || (d3 < COL_EXTRA);
    pos = (d0 > COL_EXTRA) || (d1 > COL_EXTRA) || (d2 > COL_EXTRA) || (d3 < COL_EXTRA);

    return !(neg && pos);
}

static short get_height_on_plane(short x, short z, lvl_vec *vert, lvl_vec *norm) {
    return (norm->x * (x - vert->x) + norm->z * (z - vert->z)) / -norm->y + vert->y;
}

static int test_col_part(lvl_part *part, long x, long z, long current_height, long *height) {
    short cell_x, cell_z;
    long h, highest;
    int i;
    char hit_something = 0;
    u_short face;
    lvl_col_cell *cell;

    x += part->offset_x;
    z += part->offset_z;

    if (x < part->colgrid_x || z < part->colgrid_z) {
        return 0;
    }

    cell_x = (x - part->colgrid_x) >> part->colgrid_shift;
    cell_z = (z - part->colgrid_z) >> part->colgrid_shift;

    if (cell_x >= part->colgrid_w || cell_z >= part->colgrid_h) {
        return 0;
    }

    // might wanna make width power of two and use bshifts

    cell = &part->col_cells[cell_x + cell_z * part->colgrid_w];

    // printf("%d, %d has %u tris starting at %u: ", cell_x, cell_z, cell->num_tris, cell->tri_offset);
    highest = 999999999;

    // only checks quads for now
    for (i = 0; i < cell->num_quads; i++) {
        face = part->col_heap[cell->quad_offset + i];
        
        if (is_inside_quad(x, z, part, &part->quads[face])) {
            h = get_height_on_plane(x, z, &part->positions[part->quads[face].pos[0]], &part->normals[part->quads[face].norm]);
            h -= part->offset_y;

            if ((h - current_height < MAX_FALL_DIST && h - current_height > -MAX_STEP_DIST) && h < highest) {
                *height = h;
                highest = h;
                hit_something = 1;
            }
        }
    }

    return hit_something;
}

int test_col(long x, long z, long current_height, long *height) {
    int i;

    for (i = 0; i < level.current->num_parts; i++) {
        if (level.active_parts[i]) {
            if (test_col_part(level.current->parts[i], x, z, current_height, height))
                return 1;
        }
    }
    return 0;
}
