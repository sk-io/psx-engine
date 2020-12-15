#include "util.h"


long util_rand() {
    static int seed = 0xbe67acff;
    seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;
    return seed;
}
