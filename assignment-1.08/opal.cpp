#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <thread>

#include <getopt.h>

#include "cerr.h"
#include "gen.h"
#include "globs.h"
#include "parse.h"
#include "turn.h"

static void	usage(int const, std::string const &);

static bool	colors();

static void	print_deathscreen(WINDOW *const);
static void	print_winscreen(WINDOW *const);

static bool	is_number(std::string const &);

static char const *const PROGRAM_NAME = "opal";

static struct option const long_opts[] = {
	{"nodescs", no_argument, NULL, 'd'},
	{"help", no_argument, NULL, 'h'},
	{"load", no_argument, NULL, 'l'},
	{"numnpcs", required_argument, NULL, 'n'},
	{"numobjs", required_argument, NULL, 'o'},
	{"save", no_argument, NULL, 's'},
	{"seed", required_argument, NULL, 'z'},
	{NULL, 0, NULL, 0}
};

int
main(int const argc, char *const argv[])
{
	WINDOW *win;
	char *end;
	int ch;
	bool load = false;
	bool save = false;
	bool no_descs = false;
	unsigned int numnpcs = std::numeric_limits<unsigned int>::max();
	unsigned int numobjs = std::numeric_limits<unsigned int>::max();
	std::string const name = (argc == 0) ? PROGRAM_NAME : argv[0];

	while ((ch = getopt_long(argc, argv, "dhln:o:sz:", long_opts, NULL)) != -1) {
		switch(ch) {
		case 'd':
			no_descs = true;
			break;
		case 'h':
			usage(EXIT_SUCCESS, name);
			break;
		case 'l':
			load = true;
			break;
		case 'n':
			numnpcs = (unsigned int)strtoul(optarg, &end, 10);

			if (optarg == end || errno == EINVAL || errno == ERANGE) {
				cerr(1, "numnpcs invalid");
			}
			break;
		case 'o':
			numobjs = (unsigned int)strtoul(optarg, &end, 10);

			if (optarg == end || errno == EINVAL || errno == ERANGE) {
				cerr(1, "numobjs invalid");
			}
			break;
		case 's':
			save = true;
			break;
		case 'z':
			if (is_number(optarg)) {
				rr = ranged_random(strtoul(optarg, &end, 10));

				if (optarg == end || errno == EINVAL
					|| errno == ERANGE) {
					cerr(1, "seed %s invalid", optarg);
				}
			} else {
				rr = ranged_random(optarg);
			}
			break;
		default:
			usage(EXIT_FAILURE, name);
		}
	}

	if (numnpcs == std::numeric_limits<unsigned int>::max()) {
		numnpcs = rr.rrand<unsigned int>(3, 5);
	}

	if (numobjs == std::numeric_limits<unsigned int>::max()) {
		numobjs = rr.rrand<unsigned int>(10, 15);
	}

	(void)initscr();

	if (!colors()) {
		cerrx(1, "color init");
	}

	/* requires colors initialized */
	if (!no_descs) {
		parse_npc_file();
		parse_obj_file();
	}

	if (refresh() == ERR) {
		cerrx(1, "refresh from initscr");
	}

	if ((win = newwin(HEIGHT, WIDTH, 0, 0)) == NULL) {
		cerrx(1, "newwin");
	}

	(void)box(win, 0, 0);

	if (curs_set(0) == ERR) {
		cerrx(1, "curs_set");
	}

	if (noecho() == ERR) {
		cerrx(1, "noecho");
	}

	if (raw() == ERR) {
		cerrx(1, "raw");
	}

	if (keypad(win, true) == ERR) {
		cerrx(1, "keypad");
	}

	clear_tiles();

	if (load) {
		if (!load_dungeon()) {
			cerrx(1, "loading dungeon");
		}

		arrange_loaded();
	} else {
		arrange_new();
	}

	retry:
	switch(turn_engine(win, numnpcs, numobjs)) {
	case TURN_DEATH:
		std::this_thread::sleep_for(std::chrono::seconds(1));

		print_deathscreen(win);

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		break;
	case TURN_NEXT:
		if (wclear(win) == ERR) {
			cerrx(1, "arrange_renew clear");
		}

		(void)box(win, 0, 0);

		arrange_renew();

		goto retry;

		break;
	case TURN_NONE:
		cerrx(1, "turn_engine return value invalid");
		break;
	case TURN_QUIT:
		break;
	case TURN_WIN:
		std::this_thread::sleep_for(std::chrono::seconds(1));

		print_winscreen(win);

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		break;
	}

	if (delwin(win) == ERR) {
		cerrx(1, "delwin");
	}

	if (endwin() == ERR) {
		cerrx(1, "endwin");
	}

	std::cout << "seed: " << rr.seed << '\n';

	if (save && !save_dungeon()) {
		cerrx(1, "saving dungeon");
	}

	return EXIT_SUCCESS;
}

static void
usage(int const status, std::string const &name)
{
	std::cout << "Usage: " << name << " [OPTION]... \n\n";

	if (status != EXIT_SUCCESS) {
		std::cerr << "Try '" << name <<
			" --help' for more information.\n";
	} else {
		std::cout << "OPAL's Playable Almost Indefectibly.\n\n"
			<< "Traverse a generated dungeon.\n\n"
			<< "Options:\n\
  -d, --nodescs         don't parse description files\n\
  -h, --help            display this help text and exit\n\
  -l, --load            load dungeon file\n\
  -n, --numnpcs=[NUM]   number of npcs per floor\n\
  -o, --numobjs=[NUM]   number of objs per floor\n\
  -s, --save            save dungeon file\n\
  -z, --seed=[SEED]     set rand seed, takes integer or string\n";
	}

	exit(status);
}

static bool
colors()
{
	if (start_color() == ERR) {
		return false;
	}

	if (init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK) == ERR) {
		return false;
	}

	if (init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK) == ERR) {
		return false;
	}

	if (init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK) == ERR) {
		return false;
	}

	if (init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK) == ERR) {
		return false;
	}

	if (init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK) == ERR) {
		return false;
	}

	if (init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK) == ERR) {
		return false;
	}

	if (init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK) == ERR) {
		return false;
	}

	return true;
}

static void
print_deathscreen(WINDOW *const win)
{
	if (wclear(win) == ERR) {
		cerrx(1, "clear on deathscreen");
	}

	(void)box(win, 0, 0);
	(void)mvwprintw(win, HEIGHT / 2 - 1, WIDTH / 4, "You're dead, Jim.");
	(void)mvwprintw(win, HEIGHT / 2 + 0, WIDTH / 4,
		"\t\t-- McCoy, stardate 3468.1");
	(void)mvwprintw(win, HEIGHT / 2 + 2, WIDTH / 4,
		"You've died. Game over.");
	(void)mvwprintw(win, HEIGHT - 1, 2, "[ press any key to exit ]");

	if (wrefresh(win) == ERR) {
		cerrx(1, "wrefresh on deathscreen");
	}

	(void)wgetch(win);
}

static void
print_winscreen(WINDOW *const win)
{
	if (wclear(win) == ERR) {
		cerrx(1, "clear on winscreen");
	}

	(void)box(win, 0, 0);
	(void)mvwprintw(win, HEIGHT / 2 - 3, WIDTH / 12,
		"[War] is instinctive. But the insinct can be fought. We're human");
	(void)mvwprintw(win, HEIGHT / 2 - 2, WIDTH / 12,
		"beings with the blood of a million savage years on our hands! But we");
	(void)mvwprintw(win, HEIGHT / 2 - 1, WIDTH / 12,
		"can stop it. We can admit that we're killers ... but we're not going");
	(void)mvwprintw(win, HEIGHT / 2 + 0, WIDTH / 12,
		"to kill today. That's all it takes! Knowing that we're not going to");
	(void)mvwprintw(win, HEIGHT / 2 + 1, WIDTH / 12, "kill today!");
	(void)mvwprintw(win, HEIGHT / 2 + 2, WIDTH / 12,
		"\t\t-- Kirk, \"A Taste of Armageddon\", stardate 3193.0");
	(void)mvwprintw(win, HEIGHT / 2 + 4, WIDTH / 12, "You've won. Game over.");
	(void)mvwprintw(win, HEIGHT - 1, 2, "[ press any key to exit ]");

	if (wrefresh(win) == ERR) {
		cerrx(1, "wrefresh on winscreen");
	}

	(void)wgetch(win);
}

bool
is_number(std::string const &s)
{
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char const c) {
		return !std::isdigit(c);
	}) == s.end();
}
