
#include <libgte.h>

static void update_test_ent(entity_t *e) {
    long dx, dz;
    long len;

    dx = (the_player.pos.vx - e->pos.vx) >> 16;
    dz = (the_player.pos.vz - e->pos.vz) >> 16;
    len = (dx * dx) + (dz * dz);
    
    if (len > 1000) {
        len = csqrt(len) >> 6;
        e->rot.vy = ratan2(dx, dz) - 4096 / 4;
        dx = (dx * 400000) / len;
        dz = (dz * 400000) / len;
        e->pos.vx += dx;
        e->pos.vz += dz;
    } else {
        e->pos.vx = (rand() & 0x7FFF - 0x3FFF) << 12;
        e->pos.vy = the_player.pos.vy;
        e->pos.vz = (rand() & 0x7FFF - 0x3FFF) << 12;
    }
}
