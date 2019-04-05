#ifndef NPC_H
#define NPC_H

#include <ncurses.h>

enum turn_exit {
	TURN_DEATH,
	TURN_NEXT,
	TURN_NONE,
	TURN_QUIT,
	TURN_WIN
};

enum turn_exit	turn_engine(WINDOW *const, unsigned int const);

#endif /* NPC_H */
