#ifndef OPAL_H
#define OPAL_H

#include "floor.h"

extern struct player p;

extern uint16_t room_count;
extern struct room *rooms;

extern uint16_t stair_up_count;
extern uint16_t stair_dn_count;

extern struct stair *stairs_up;
extern struct stair *stairs_dn;

struct tile {
	uint8_t	h; /* hardness */
	chtype	c; /* character */

	/* dijstra */
	int32_t	d; /* non-tunneling distance cost */
	int32_t dt; /* tunneling distance cost */
	uint8_t	x; /* x position */
	uint8_t	y; /* y position */
};

#define WIDTH	80
#define HEIGHT	21

extern struct tile tiles[HEIGHT][WIDTH];

#endif /* OPAL_H */
