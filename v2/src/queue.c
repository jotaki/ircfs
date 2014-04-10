#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "socket.h"

static char g_full_path[MAX_PATH_NAME + 1] = { 0 };
static struct queue_object_s * g_queue = NULL;

char *full_path(const char * const filename)
{
	char *path = NULL;

	path = calloc(strlen(filename) + strlen(g_full_path) + 2, sizeof(char));
	if(path)
		sprintf(path, "%s/%s", g_full_path, filename);

	return path;
}

void queue_add(const char * const channel, const char * const data)
{
	struct queue_object_s * q;

	q = calloc(1, sizeof(struct queue_object_s));
	if(q == NULL)
		return;

	q->target = calloc(strlen(channel) + 1, sizeof(char));
	if(q->target == NULL) {
		free(q);
		q = NULL;

		return;
	}

	if(data == NULL)
		q->buffer = data;
	else {
		q->buffer = calloc(strlen(data) + 1, sizeof(char));
		if(q->buffer == NULL) {
			free(q->target);
			free(q);
			q = NULL;

			return;
		}
	}

	strcpy(q->target, channel);
	strcpy(q->buffer, data);

	if(g_queue == NULL)
		g_queue = q;
	else {
		if(g_queue->end)
			g_queue->end->next = q;
		else
			g_queue->next = q;

		g_queue->end = q;
	}
}

void send_queue(int sd)
{
	int i;
	struct queue_object_s * q, * tmp;

	if(g_queue == NULL)
		return;

	for(i = 0, q = g_queue; g_queue != NULL && i < 3; ++i) {
		if(q->buffer != NULL) {
			skprintf(sd, "PRIVMSG %s :%s\r\n",
					q->target, q->buffer);
			free(q->buffer);
			q->buffer = NULL;
		} else
			skprintf(sd, "PART %s Leaving\r\n", q->target);

		tmp = q->next;

		free(q->target);
		q->target = NULL;

		if(tmp)
			tmp->end = q->end;

		free(q);
		q = g_queue = tmp;
	}
}

void set_full_path(const char * const path)
{
	strncpy(g_full_path, path, MAX_PATH_NAME);
}
