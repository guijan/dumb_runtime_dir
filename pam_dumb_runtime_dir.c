/**
 * pam_dumb_runtime_dir.c
 *
 * Creates an XDG_RUNTIME_DIR directory on login per the freedesktop.org
 * base directory spec. Flaunts the spec and never removes it, even after
 * last logout. This keeps things simple and predictable.
 *
 * The user is responsible for ensuring that the RUNTIME_DIR_PARENT directory,
 * (/run/user by default) exists and is only writable by root.
 *
 * Copyright 2021 Isaac Freund
 * Copyright 2021 Guilherme Janczak
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <security/pam_misc.h>
#include <security/pam_modules.h>

int pam_sm_open_session(pam_handle_t *pamh, int flags,
		int argc, const char **argv) {
	(void)flags;
	(void)argc;
	(void)argv;

	int rval = PAM_SESSION_ERR;

	const char *user;
	if (pam_get_user(pamh, &user, NULL) != PAM_SUCCESS) {
		return rval;
	}

	struct passwd *pw = getpwnam(user);
	if (pw == NULL) {
		return rval;
	}

	char *path;
	/*
	 * Adjacent string literals are combined into one as per the C standard.
	 * Appending the null string to the macro RUNTIME_DIR_PARENT will cause
	 * a syntax error if whoever compiles this program defines it to
	 * something other than a string.
	 */
	if (asprintf(&path, "%s/%jd", RUNTIME_DIR_PARENT "",
	    (intmax_t)pw->pw_uid) == -1) {
		return rval;
	}
	if (mkdir(path, 0700) < 0) {
		/* It's ok if the directory already exists, in that case we just
		 * ensure the mode is correct before we chown(). */
		if (errno != EEXIST) {
			goto end;
		}
		if (chmod(path, 0700) < 0) {
			goto end;
		}
	}

	if (chown(path, pw->pw_uid, pw->pw_gid) < 0) {
		goto end;
	}

	if (pam_misc_setenv(pamh, "XDG_RUNTIME_DIR", path, 1) != PAM_SUCCESS) {
		goto end;
	}

	rval = PAM_SUCCESS;
end:
	free(path);
	return rval;
}
