#ifndef _TEXTURE_H
#define _TEXTURE_H

#define MAX_TEXTURES 64

#define CLUT_X 0
#define CLUT_Y 480

typedef struct {
    long tpage;
    long clut;
} texture_t;

extern texture_t textures[MAX_TEXTURES];

#endif
