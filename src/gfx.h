#ifndef _GFX_H
#define _GFX_H

#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>

#define OTLEN 11 // 10
#define OTSIZE (1 << OTLEN)
#define PBSIZE 32767

#define CLUT_X 0
#define CLUT_Y 480

typedef struct {
	DRAWENV draw;
	DISPENV disp;
	u_long ot[OTSIZE];
	u_char pb[PBSIZE];
} DB;

typedef struct {
	DB* db;
	u_char *pb_ptr;
	RECT clip;
	DIVPOLYGON4 *div;
	MATRIX *view_persp;
} draw_state_t;

extern u_short tpage;
extern u_short cluts[16];

#endif
