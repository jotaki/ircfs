#ifndef QUEUE_H
# define QUEUE_H

# define MAX_PATH_NAME	1024	/* adjust this if you see fit. */

struct queue_object_s
{
	char * target,
	     * buffer;

	struct queue_object_s * next,
			      * end;
};

/*
 * returns the full path to filename (probably should be elsewhere, misc.h?)
 */
char *full_path(const char * const filename);

/*
 * append data to queue
 */
void queue_add(const char * const channel, const char * const data);

/*
 * send queue
 */
void send_queue(int sd);

/*
 * set filename
 */
void set_full_path(const char * const path);

#endif	/* !QUEUE_H */
