#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>

#include "ircfs.h"

extern int errno;
static int chkdir(const char * const mount_dir);

int watch_dir(const char * const mount_dir, int * watcher, int * fd)
{
	if(chkdir(mount_dir))
		return 1;

	*watcher = inotify_init1(IN_CLOEXEC);
	if(0 > *watcher) {
		remove(mount_dir);
		return 1;
	}

	*fd = inotify_add_watch(*watcher, mount_dir, IN_CLOSE_WRITE);
	if(0 > *fd) {
		remove(mount_dir);
		close(*watcher);
		return 1;
	}

	return 0;
}

void stop_watch_dir(const char * const mount_dir, int * watcher, int * fd)
{
	close(*fd);
	close(*watcher);
	remove(mount_dir);

	*fd = *watcher = -1;
}

/*
 * this checks to verify the directory does not exist
 * is writable, and then creates it. (slightly misleading function name.)
 */

static int chkdir(const char * const mount_dir)
{
	struct stat dirinfo;
	int err;

	err = stat(mount_dir, &dirinfo);
	if(err == 0) {
		printf("Directory exists: ``%s'', this should not be.\n",
				mount_dir);
		return 1;
	}

	if(errno != ENOENT) {
		printf("Issue with using ``%s'': %s\n", mount_dir,
				strerror(errno));
		return 1;
	}

	err = mkdir(mount_dir, 0755);
	if(0 > err) {
		printf("Error creating directory: ``%s'': %s\n", mount_dir,
				strerror(errno));
		return 1;
	}

	return 0;
}
