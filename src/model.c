#include "model.h"

model_t *model_lurker;

void load_model(model_t *mdl) {
    int i, f;
    SVECTOR *frame_pos;

    printf("num vertices: %d\n", mdl->num_vertices);
    printf("num frames: %d\n", mdl->num_frames);
    printf("num tris: %d\n", mdl->num_tris);
    // update pointers
    mdl->pos = (SVECTOR *) ((u_long) mdl->pos + (u_long) mdl);
    mdl->uvs = (mdl_uv *) ((u_long) mdl->uvs + (u_long) mdl);
    mdl->tris = (mdl_tri *) ((u_long) mdl->tris + (u_long) mdl);

    for (f = 0; f < mdl->num_frames; f++) {
        frame_pos = &mdl->pos[f * mdl->num_vertices];

        for (i = 0; i < mdl->num_vertices; i++) {
            //printf("%d, %d, %d\n", frame_pos[i].vx, frame_pos[i].vy, frame_pos[i].vz);
        }
    }
}

void draw_model(model_t *mdl, draw_state_t *ds, int frame) {
    int i;
    long ret, otz, flags, p;
    POLY_F3 *poly = (POLY_F3*) ds->pb_ptr;
    SVECTOR *pos_arr = &mdl->pos[frame * mdl->num_vertices];

    CVECTOR col;
    CVECTOR near = {200, 200, 200, 44};

    for (i = 0; i < mdl->num_tris; i++) {
        ret = RotAverageNclip3(
            (SVECTOR *) &pos_arr[mdl->tris[i].pos[1]],
            (SVECTOR *) &pos_arr[mdl->tris[i].pos[0]],
            (SVECTOR *) &pos_arr[mdl->tris[i].pos[2]],
            (long*) &poly->x0,
            (long*) &poly->x1,
            (long*) &poly->x2,
            &p, &otz, &flags);

        if (ret <= 0)
            continue;

        // if (p >= 4095) // obscured by fog
        //     continue;

        otz = otz >> 1;
        
        if (otz <= 0 || otz >= OTSIZE)
            continue;
        
        if (tri_clip(&ds->clip,
            (DVECTOR *)&poly->x0,
            (DVECTOR *)&poly->x1,
            (DVECTOR *)&poly->x2))
                continue;

        DpqColor(&near, p, &col);

        setPolyF3(poly);
        setRGB0(poly, col.r, col.g, col.b);

        addPrim(ds->db->ot + otz, poly);
        poly++;
        debug_num_polys++;
    }

    ds->pb_ptr = (u_char*) poly;
}
