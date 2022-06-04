#include <err.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>

/* Mostly cribbed from lighttpd algo_md.h and http_etag.c */

/* Donald E. Knuth
 * The Art Of Computer Programming Volume 3
 * Chapter 6.4, Topic: Sorting and Search */
/*(len should be passed as initial hash value.
 * On subsequent calls, pass intermediate hash value for incremental hashing)*/
static uint32_t dekhash (const char *str, const uint32_t len, uint32_t hash)
{
	const unsigned char * const s = (const unsigned char *)str;
	for (uint32_t i = 0; i < len; ++i)
		hash = (hash << 5) ^ (hash >> 27) ^ s[i];
	return hash;
}

static uint32_t etag_create (struct stat *st, const int flags) {
	uint64_t x[4];
	uint32_t len = 0;

	if (flags & 1)
		x[len++] = st->st_ino;
	if (flags & 2)
		x[len++] = st->st_size;
	if (flags & 4) {
		x[len++] = st->st_mtime;
#ifndef st_mtime /* use high-precision timestamp if available */
#elif defined(__APPLE__) && defined(__MACH__)
		x[len++] = st->st_mtimespec.tv_nsec;
#else
		x[len++] = st->st_mtim.tv_nsec;
#endif
	}

	len *= sizeof *x;
	return dekhash((char *)x, len, len);
}

int main (int argc, char **argv) {
	if (argc <= 2) {
usage:
		errx(2, "usage: %s [aISM1-9]+ files...", argv[0]);
	}

	int flags = ~0;
	for (char *p = argv[1]; *p; p++) {
		switch (*p) {
		case 'a': flags = ~0; break;
		case 'I': flags &= ~1; break;
		case 'i': flags |= 1; break;
		case 'S': flags &= ~2; break;
		case 's': flags |= 2; break;
		case 'M': flags &= ~4; break;
		case 'm': flags |= 4; break;
		case '-': break;
		default: goto usage;
		}
	}

	for (char **p = argv+2; *p; p++) {
		struct stat st;
		stat(*p, &st);
		printf("%s%c%zu%c", *p, 0, (size_t)etag_create(&st, flags), 0);
	}
}
