#include "entity.h"

#include "player.h"

entity_t entities[MAX_ENTITIES] = {0};

#include "testent.inc.c"

void (*update_ent_func[])(entity_t* e) = {
    update_test_ent
};

static void update_entity(entity_t* e) {
    //update_ent_func[ENT_TEST](e);
}

void ent_unload_all() {
    memset(entities, 0, sizeof(entities));
}

void ent_update() {
    int i;
    entity_t *e;

    for (i = 0; i < MAX_ENTITIES; i++) {
        e = entities + i;

        if (e->type != 0) {
            update_entity(e);
        }
    }
}

static void draw_entity(draw_state_t *ds, entity_t *e) {
    MATRIX mat;
    VECTOR pos;
    SVECTOR spos;

    spos.vx = pos.vx = e->pos.vx >> 12;
    spos.vy = pos.vy = e->pos.vy >> 12;
    spos.vz = pos.vz = e->pos.vz >> 12;

    // printf("%d %d %d\n", spos.vx, spos.vy, spos.vz);

    RotMatrix(&e->rot, &mat);
    TransMatrix(&mat, &pos);
    CompMatrixLV(ds->view_persp, &mat, &mat);

    PushMatrix();
    SetRotMatrix(&mat);
    SetTransMatrix(&mat);
    if (e->model) {
        draw_model(e->model, ds, (frames >> 2) % 11);
    } else {
        printf("warning: entity has no model\n");
    }
    PopMatrix();
}

void ent_draw(draw_state_t *ds) {
    int i;
    entity_t *e;

    for (i = 0; i < MAX_ENTITIES; i++) {
        e = entities + i;

        if (e->type != 0) {
            draw_entity(ds, e);
        }
    }
}

