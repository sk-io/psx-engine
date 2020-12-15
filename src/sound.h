#ifndef _SOUND_H
#define _SOUND_H

extern char* snd_footstep;

void init_sound();
void set_note(short on, short channel, unsigned short pitch);

#endif
