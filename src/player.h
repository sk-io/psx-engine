#ifndef _INPUT_H
#define _INPUT_H

#include "global.h"
#include "level.h"

typedef struct {
    VECTOR pos;
    SVECTOR rot;
} Player;

extern Player the_player;

void player_update(Player *p, level_t* lvl);

#endif
