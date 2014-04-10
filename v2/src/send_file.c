#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ircfs.h"
#include "queue.h"

void send_file(const char * const filename)
{
	FILE *fp = NULL;
	char *path = NULL,
	     *buf = NULL,
	     *ptr = NULL;

	path = full_path(filename);
	if(path == NULL)
		return;

	buf = calloc(sizeof(char), 392);
	if(buf == NULL) {
		free(path);
		path = NULL;
		return;
	}

	fp = fopen(path, "r");
	if(fp == NULL) {
		free(path);
		free(buf);

		path = buf = NULL;
		return;
	}

	while(fgets(buf, 384, fp)) {
		ptr = strchr(buf, '\n');
		if(ptr) {
			if(*(ptr-1) == '\r')
				*(ptr-1) = '\0';

			*ptr = 0;
			ptr = NULL;
		}
		queue_add(/* channel */ filename, buf);
	}
	queue_add(filename, NULL);
	fclose(fp);
	remove(path);

	free(path);
	free(buf);
	path = buf = NULL;
	fp = NULL;
}
