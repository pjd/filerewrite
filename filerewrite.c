/*-
 * BSD 2-Clause License
 * 
 * Copyright (c) 2021, Pawel Jakub Dawidek <pawel@dawidek.net>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
#define	st_atim	st_atimespec
#define	st_mtim	st_mtimespec
#endif

#define	BUFSIZE	(8 * 1024 * 1024)

#define	WARNX(fmt, ...)		fprintf(stderr, fmt "\n", __VA_ARGS__)
#define	WARN(fmt, ...)		fprintf(stderr, fmt ": %s.\n", __VA_ARGS__, \
				    strerror(errno))
#define	VERBOSE(fmt, ...)	do {					\
					if (verbose) {			\
						printf(fmt "\n", __VA_ARGS__); \
					}				\
				} while (false)

static bool verbose = false;

static bool
rewrite_file(const char *path)
{
	static unsigned char buf[BUFSIZE];
	struct timespec times[2];
	struct stat sb;
	off_t offset;
	ssize_t rdone, wdone;
	bool ret;
	int fd;

	ret = false;

	fd = open(path, O_RDWR | O_NOFOLLOW);
	if (fd < 0) {
		WARN("Unable to open %s", path);
		return (false);
	}

	if (fstat(fd, &sb) < 0) {
		WARN("Unable to stat %s", path);
		goto out;
	}
	if (!S_ISREG(sb.st_mode)) {
		WARNX("%s is not a regular file, skipping.", path);
		goto out;
	}

	for (offset = 0;;) {
		rdone = pread(fd, buf, sizeof(buf), offset);
		if (rdone == 0) {
			break;
		} else if (rdone < 0) {
			WARN("Read from %s at offset %jd failed", path,
			    (intmax_t)offset);
			goto out;
		}
		VERBOSE("Read %zd from %s at offset %jd.", rdone, path,
		    (intmax_t)offset);

		wdone = pwrite(fd, buf, (size_t)rdone, offset);
		if (wdone < 0) {
			WARN("Write %s at offset %jd failed", path,
			    (intmax_t)offset);
			goto out;
		} else if (wdone == 0) {
			WARNX("Wrote nothing to %s at offset %jd.", path,
			    (intmax_t)offset);
			goto out;
		}
		VERBOSE("Wrote %zd to %s at offset %jd.", wdone, path,
		    (intmax_t)offset);
		if (wdone < rdone) {
			WARNX("Short write to %s at offset %jd (wrote %zd instead of %zd).",
			    path, (intmax_t)offset, wdone, rdone);
		}

		offset += wdone;
	}

	times[0] = sb.st_atim;
	times[1] = sb.st_mtim;
	if (futimens(fd, times) < 0) {
		WARN("Unable to restore access and modification times on %s",
		    path);
		goto out;
	}
	VERBOSE("Restored access and modification times on %s.", path);

	ret = true;
out:
	close(fd);

	return (ret);
}

int
main(int argc, char *argv[])
{
	unsigned int ii;
	int ret;

	argc--;
	argv++;;
	if (argc == 0 || strcmp(argv[0], "-h") == 0) {
		fprintf(stderr, "usage: filerewrite [-v] file ...\n");
		exit(1);
	} else if (strcmp(argv[0], "-v") == 0) {
		verbose = true;
		argc--;
		argv++;
	}

	ret = 0;
	for (ii = 0; ii < argc; ii++) {
		VERBOSE("Rewriting %s...", argv[ii]);
		if (!rewrite_file(argv[ii])) {
			ret = 1;
		}
	}

	exit(ret);
}
