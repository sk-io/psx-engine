#include "level.h"

#include <libapi.h>
#include <libetc.h>

#include <string.h>

#include "gfx.h"
#include "entity.h"
#include "player.h"
#include "clip.h"

struct level_ level = { 0 };

static void draw_level_part(lvl_part *part, draw_state_t *ds);
static void draw_level_part_quads(lvl_part *part, draw_state_t *ds);

static void set_player_pos_rot(long x, long y, long z, long rx, long ry) {
    the_player.pos.vx = x << 12;
    the_player.pos.vy = y << 12;
    the_player.pos.vz = z << 12;
    the_player.rot.vx = rx;
    the_player.rot.vy = ry;
}

void change_level(int id) {
    if (level.current != 0)
        free3(level.current);

    ent_unload_all();

    switch (id) {
    case LEVEL_LEVEL1:
        level.current = (level_t*) cd_read_file("\\LEVEL1.LVL;1", NULL);
        set_player_pos_rot(194, 0, 4628, 0, 2026);
        break;
    }
    level.id = id;

    load_level(level.current);
}

void load_level(level_t* file) {
    int i, j, k;
    u_long offset;
    lvl_part *part;

    printf("level has %u parts\n", file->num_parts);
    printf("file is at %p\n", file);

    // update header pointers
    file->parts          = (lvl_part**) ((u_long) file->parts + (u_long) file);
    file->bounding_boxes = (lvl_bbox*) ((u_long) file->bounding_boxes + (u_long) file);
    // update part table
    for (i = 0; i < file->num_parts; i++) {
        file->parts[i] = (lvl_part*) ((u_long) file->parts[i] + (u_long) file);
    }

    // update the pointers in all the parts
    for (i = 0; i < file->num_parts; i++) {
        part = file->parts[i];
        offset = (u_long) part;
        
        part->tris = (lvl_tri*) ((u_long) part->tris + offset);
        part->quads = (lvl_quad*) ((u_long) part->quads + offset);

        part->positions = (lvl_vec*) ((u_long) part->positions + offset);
        part->uvs = (lvl_uv*) ((u_long) part->uvs + offset);
        part->normals = (lvl_vec*) ((u_long) part->normals + offset);

        part->col_cells = (lvl_col_cell*) ((u_long) part->col_cells + offset);
        part->col_heap = (u_short*) ((u_long) part->col_heap + offset);
    }
    
    memset(level.active_parts, 0, LEVEL_MAX_PARTS);
}

static int test_bb(SVECTOR *point, lvl_bbox *bbox) {
    if ((long) point->vx < bbox->x0)
        return 0;
    if ((long) point->vy < bbox->y0)
        return 0;
    if ((long) point->vz < bbox->z0)
        return 0;
    
    if ((long) point->vx > bbox->x1)
        return 0;
    if ((long) point->vy > bbox->y1)
        return 0;
    if ((long) point->vz > bbox->z1)
        return 0;
    
    return 1;
}

void level_update_active_parts(VECTOR *player_pos) {
    int i;
    lvl_bbox *bbox;
    SVECTOR svec;

    svec.vx = player_pos->vx >> 12;
    svec.vy = player_pos->vy >> 12;
    svec.vz = player_pos->vz >> 12;

    for (i = 0; i < level.current->num_parts; i++) {
        bbox = &level.current->bounding_boxes[i];
        
        level.active_parts[i] = test_bb(&svec, bbox);
        // level.active_parts[i] = 1;
    }
}

void draw_level(draw_state_t *ds) {
    int i;
    ds->div = (DIVPOLYGON4*) getScratchAddr(0);
    ds->div->pih = ds->clip.w;
    ds->div->piv = ds->clip.h;

    for (i = 0; i < level.current->num_parts; i++) {
        if (level.active_parts[i])
            draw_level_part(level.current->parts[i], ds);
    }
}

static void draw_level_part(lvl_part *part, draw_state_t *ds) {
    MATRIX mat = {
        {{ONE, 0, 0},
         {0, ONE, 0},
         {0, 0, ONE}},
        { -part->offset_x, -part->offset_y, -part->offset_z }
    };
    // printf("%d %d %d\n", part->offset_x, part->offset_y, part->offset_z);

    CompMatrixLV(ds->view_persp, &mat, &mat);

    PushMatrix();
    SetRotMatrix(&mat);
    SetTransMatrix(&mat);
    draw_level_part_quads(part, ds);
    PopMatrix();
}

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define CLAMP_EXTENT_X 127
#define CLAMP_EXTENT_Y 127

static void clamp_poly_ft4(POLY_FT4* poly) {
    // poly->x0 = CLAMP(poly->x0, -CLAMP_EXTENT_X, SCR_W + CLAMP_EXTENT_X);
    // poly->x1 = CLAMP(poly->x1, -CLAMP_EXTENT_X, SCR_W + CLAMP_EXTENT_X);
    // poly->x2 = CLAMP(poly->x2, -CLAMP_EXTENT_X, SCR_W + CLAMP_EXTENT_X);
    // poly->x3 = CLAMP(poly->x3, -CLAMP_EXTENT_X, SCR_W + CLAMP_EXTENT_X);

    poly->y0 = CLAMP(poly->y0, -CLAMP_EXTENT_Y, SCR_H + CLAMP_EXTENT_Y);
    poly->y1 = CLAMP(poly->y1, -CLAMP_EXTENT_Y, SCR_H + CLAMP_EXTENT_Y);
    poly->y2 = CLAMP(poly->y2, -CLAMP_EXTENT_Y, SCR_H + CLAMP_EXTENT_Y);
    poly->y3 = CLAMP(poly->y3, -CLAMP_EXTENT_Y, SCR_H + CLAMP_EXTENT_Y);
}

static void draw_level_part_quads(lvl_part *part, draw_state_t *ds) {
    // NOTE: draws in strange order:
    // 1 0 2 3
    // might change order when exporting instead
    // but the model is also used for collision so...

    int i;
    long ret, otz, flags, p;
    POLY_FT4 *poly;
    CVECTOR col;
    CVECTOR near = {128, 128, 128, 44};
    lvl_quad* quad;

    SVECTOR pos[4];

    poly = (POLY_FT4*) ds->pb_ptr;
    
    for (i = 0; i < part->num_quads; i++) {
        quad = &part->quads[i];

        ret = RotAverageNclip4(
            (SVECTOR*) &part->positions[quad->pos[1]],
            (SVECTOR*) &part->positions[quad->pos[0]],
            (SVECTOR*) &part->positions[quad->pos[2]],
            (SVECTOR*) &part->positions[quad->pos[3]],
            (long *)&poly->x0,
            (long *)&poly->x1,
            (long *)&poly->x2,
            (long *)&poly->x3,
            &p, &otz, &flags);

        if (ret <= 0)
            continue;

        otz = otz >> 1;
        
        if (otz <= 0 || otz >= OTSIZE)
            continue;

        if (quad_clip(&ds->clip,
            (DVECTOR *)&poly->x1,
            (DVECTOR *)&poly->x0,
            (DVECTOR *)&poly->x2,
            (DVECTOR *)&poly->x3))
                continue;
        
        DpqColor(&near, p, &col);
        
        setPolyFT4(poly);
        setShadeTex(poly, 0);
        setRGB0(poly, col.r, col.g, col.b);
        //setSemiTrans(poly, 0);
        poly->clut = quad->clut;
        poly->tpage = tpage;
        
        poly->u0 = part->uvs[quad->uv[1]].u;
        poly->v0 = part->uvs[quad->uv[1]].v;

        poly->u1 = part->uvs[quad->uv[0]].u;
        poly->v1 = part->uvs[quad->uv[0]].v;

        poly->u2 = part->uvs[quad->uv[2]].u; // NOTE CHANGED ORDER
        poly->v2 = part->uvs[quad->uv[2]].v;

        poly->u3 = part->uvs[quad->uv[3]].u;
        poly->v3 = part->uvs[quad->uv[3]].v;
        
        if (otz <= 250) {
            // ds->div->ndiv = (otz <= 150) ? 2 : 1;
            ds->div->ndiv = 1;

            ret = DivideFT4(
                (SVECTOR*) &part->positions[quad->pos[1]].x,
                (SVECTOR*) &part->positions[quad->pos[0]].x,
                (SVECTOR*) &part->positions[quad->pos[2]].x,
                (SVECTOR*) &part->positions[quad->pos[3]].x,
                (u_long*) &poly->u0,
                (u_long*) &poly->u1,
                (u_long*) &poly->u2,
                (u_long*) &poly->u3,
                &col, poly, ds->db->ot + otz, ds->div);
            
            clamp_poly_ft4(poly);
            clamp_poly_ft4(poly + 1);
            clamp_poly_ft4(poly + 2);
            clamp_poly_ft4(poly + 3);

            poly = (POLY_FT4*) ret;
            debug_num_polys += (ds->div->ndiv == 2) ? 8 : 4;
        } else {
            clamp_poly_ft4(poly);
            addPrim(ds->db->ot + otz, poly);
            poly++;
            debug_num_polys++;
        }
    }

    ds->pb_ptr = (u_char*) poly;
}
