#include "player.h"

#include <LIBSPU.H>
#include <LIBGTE.H>
#include <LIBETC.H>

#include <stdlib.h>

#include "col.h"

#include "sound.h"
#include "entity.h"
#include "util.h"

// #define DEBUG_NOCLIP

#define MOVE_SPEED 25
//#define RUN_SPEED 40
#define RUN_SPEED 100
#define ROTATE_SPEED 30
#define COL_POINT_OFFSET 300
#define MAX_X_ROT 500
#define COL_SIZE 16

static int walk_counter = 0;

Player the_player = { 0 };

static int holding = 0;
static int noclip = 0;

void player_update(Player *p, level_t* lvl) {
	u_long pad;
	VECTOR dest;
	long dest_sx, dest_sz;
	long dest_height;
	int did_move = 0;
	int running = 0;
	int speed;

	int tsin = rsin(-p->rot.vy);
	int tcos = rcos(-p->rot.vy);

	pad = PadRead(0);

	dest = p->pos;

	if (pad & PADRdown)
		running = 1;

	speed = running ? RUN_SPEED : MOVE_SPEED;

	if (pad & PADLup) {
		dest.vz += tcos * speed;
		dest.vx += tsin * speed;
		did_move = 1;
	}
	if (pad & PADLdown) {
		dest.vz -= tcos * speed;
		dest.vx -= tsin * speed;
		did_move = 1;
	}

	if (pad & PADLright)
		p->rot.vy -= ROTATE_SPEED;
	if (pad & PADLleft)
		p->rot.vy += ROTATE_SPEED;
	
	p->rot.vy &= 0xFFF;

	if (pad & PADRright) {
		p->rot.vx += ROTATE_SPEED;
	} else if (pad & PADRup) {
		p->rot.vx -= ROTATE_SPEED;
	} else { // move back to 0
		if (abs(p->rot.vx) < 50) {
			p->rot.vx = 0;
		} else {
			p->rot.vx += (p->rot.vx > 0) ? -ROTATE_SPEED : ROTATE_SPEED;
		}
	}
	
	if (p->rot.vx < -MAX_X_ROT)
		p->rot.vx = -MAX_X_ROT;
	if (p->rot.vx > MAX_X_ROT)
		p->rot.vx = MAX_X_ROT;
	
	if (pad & PADRleft) {
		if (!holding) {
			holding = 1;
		}
	} else {
		holding = 0;
	}
	
	dest_sx = dest.vx >> 12;
	dest_sz = dest.vz >> 12;

	if (!noclip) {
		if (test_col(dest_sx - COL_POINT_OFFSET, dest_sz - COL_POINT_OFFSET, p->pos.vy >> 12, &dest_height) &&
			test_col(dest_sx + COL_POINT_OFFSET, dest_sz - COL_POINT_OFFSET, p->pos.vy >> 12, &dest_height) &&
			test_col(dest_sx + COL_POINT_OFFSET, dest_sz + COL_POINT_OFFSET, p->pos.vy >> 12, &dest_height) &&
			test_col(dest_sx - COL_POINT_OFFSET, dest_sz + COL_POINT_OFFSET, p->pos.vy >> 12, &dest_height)) {
			p->pos.vx = dest.vx;
			p->pos.vz = dest.vz;
			p->pos.vy = dest_height << 12;

			if (did_move) {
				walk_counter++;
				if (walk_counter >= (running ? 15 : 25)) {
					walk_counter = 0;
					set_note(1, SPU_0CH, (util_rand() & 0x1FF) + 0x1000);
				}
			} else {
				walk_counter = 10;
			}
		}
	} else {
		p->pos.vx = dest.vx;
		p->pos.vz = dest.vz;
	}
	
	if (pad & PADselect)
		noclip = !noclip;
}
