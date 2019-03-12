#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include <endian.h>
#undef _BSD_SOURCE
#undef _DEFAULT_SOURCE

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include <iostream>

#include "cerr.h"
#include "globs.h"

#define DIRECTORY	"/.rlg327"
#define FILEPATH	"/dungeon"
int const DF_L = 8;

#define MARK	"RLG327-S2019"
int const MARK_L = 12;

static int
write_things(FILE *const f)
{
	uint32_t const ver = htobe32(0);
	uint32_t const filesize = htobe32((uint32_t)(1708 + (room_count * 4)
		+ (stair_up_count * 2) + (stair_dn_count * 2)));
	int i, j;

	/* type marker */
	if (fwrite(MARK, MARK_L, 1, f) != 1) {
		return -1;
	}

	/* version */
	if (fwrite(&ver, sizeof(uint32_t), 1, f) != 1) {
		return -1;
	}

	/* size */
	if (fwrite(&filesize, sizeof(uint32_t), 1, f) != 1) {
		return -1;
	}

	/* player coords */
	if (fwrite(&player.x, sizeof(uint8_t), 1, f) != 1) {
		return -1;
	}
	if (fwrite(&player.y, sizeof(uint8_t), 1, f) != 1) {
		return -1;
	}

	/* hardness */
	for (i = 0; i < HEIGHT; ++i) {
		for (j = 0; j < WIDTH; ++j) {
			if (fwrite(&tiles[i][j].h, sizeof(uint8_t), 1, f) != 1) {
				return -1;
			}
		}
	}

	/* room num */
	room_count = htobe16(room_count);
	if (fwrite(&room_count, sizeof(uint16_t), 1, f) != 1) {
		return -1;
	}
	room_count = be16toh(room_count);

	/* room data */
	for (struct room const &r : rooms) {
		if (fwrite(&r, sizeof(struct room), 1, f) != 1) {
			return -1;
		}
	}

	/* stairs_up num */
	stair_up_count = htobe16(stair_up_count);
	if (fwrite(&stair_up_count, sizeof(uint16_t), 1, f) != 1) {
		return -1;
	}
	stair_up_count = be16toh(stair_up_count);

	/* stars_up coords */
	for (struct stair const &s : stairs_up) {
		if (fwrite(&s, sizeof(struct stair), 1, f) != 1) {
			return -1;
		}
	}

	/* stairs_dn num */
	stair_dn_count = htobe16(stair_dn_count);
	if (fwrite(&stair_dn_count, sizeof(uint16_t), 1, f) != 1) {
		return -1;
	}
	stair_dn_count = be16toh(stair_dn_count);

	/* stairs_dn coords */
	for (struct stair const &s : stairs_dn) {
		if (fwrite(&s, sizeof(struct stair), 1, f) != 1) {
			return -1;
		}
	}

	return 0;
}

int
save_dungeon(void)
{
	struct stat st;
	FILE *f;
	char *home;
	int ret;
	std::string path;

#ifdef _GNU_SOURCE
	home = secure_getenv("HOME");
#else
	home = getenv("HOME");
#endif

	if (home == NULL) {
		return -1;
	}

	path = std::string(home) + DIRECTORY;

	if (stat(path.c_str(), &st) == -1) {
		if (errno == ENOENT) {
			if (mkdir(path.c_str(), 0700) == -1) {
				cerr(1, "save mkdir");
			}
		} else {
			cerr(1, "save stat");
		}
	}

	path += FILEPATH;

	if (!(f = fopen(path.c_str(), "w"))) {
		cerr(1, "save fopen");
	}

	ret = write_things(f);

	if (fclose(f) == EOF) {
		cerr(1, "save fclose, (write_things=%d)", ret);
	}

	return ret;
}

static int
load_things(FILE *const f) {
	int i, j;

	/* skip type marker, version, and size */
	if (fseek(f, MARK_L + 2 * sizeof(uint32_t), SEEK_SET) == -1) {
		return -1;
	}

	/* player coords */
	if (fread(&player.x, sizeof(uint8_t), 1, f) != 1) {
		return -1;
	}
	if (fread(&player.y, sizeof(uint8_t), 1, f) != 1) {
		return -1;
	}

	/* hardness */
	for (i = 0; i < HEIGHT; ++i) {
		for (j = 0; j < WIDTH; ++j) {
			if (fread(&tiles[i][j].h, sizeof(uint8_t), 1, f) != 1) {
				return -1;
			}
		}
	}

	/* room num */
	if (fread(&room_count, sizeof(uint16_t), 1, f) != 1) {
		return -1;
	}
	room_count = be16toh(room_count);

	rooms.resize(room_count);

	/* room data */
	for (struct room &r : rooms) {
		if (fread(&r, sizeof(struct room), 1, f) != 1) {
			return -1;
		}
	}

	/* stair_up num */
	if (fread(&stair_up_count, sizeof(uint16_t), 1, f) != 1) {
		return -1;
	}
	stair_up_count = be16toh(stair_up_count);

	stairs_up.resize(stair_up_count);

	/* stair_up coords */
	for (struct stair &s : stairs_up) {
		if (fread(&s, sizeof(struct stair), 1, f) != 1) {
			return -1;
		}
	}

	/* stair_dn num */
	if (fread(&stair_dn_count, sizeof(uint16_t), 1, f) != 1) {
		return -1;
	}
	stair_dn_count = be16toh(stair_dn_count);

	stairs_dn.resize(stair_dn_count);

	/* stair_dn_coords */
	for (struct stair &s : stairs_dn) {
		if (fread(&s, sizeof(struct stair), 1, f) != 1) {
			return -1;
		}
	}

	return 0;
}

int
load_dungeon(void)
{
	FILE *f;
	char *home;
	int ret;
	std::string path;

#ifdef _GNU_SOURCE
	home = secure_getenv("HOME");
#else
	home = getenv("HOME");
#endif

	if (home == NULL) {
		return -1;
	}

	path = std::string(home) + DIRECTORY + FILEPATH;

	if (!(f = fopen(path.c_str(), "r"))) {
		cerr(1, "load fopen");
	}

	ret = load_things(f);

	if (fclose(f) == EOF) {
		cerr(1, "load fclose, (load_things=%d)", ret);
	}

	return ret;
}