/*
 * gah, ugly hack because last minute thought
 */

#include "ircfs.h"

static int join_disabled = 0;

void disable_join()
{
	join_disabled = 1;
}

void enable_join()
{
	join_disabled = 0;
}

int join_channel()
{
	return join_disabled == 0;
}
