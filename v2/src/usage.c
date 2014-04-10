#include <stdio.h>

void usage(const char * const app)
{
	printf(
"Usage: %s [options] <nickname>@<server>[:port]                             \n"
"                                                                           \n"
"Where [options] can be one of:                                             \n"
"        -h             -- Show this output.                                \n"
"        -d             -- Directory to mount (defaults to /tmp/ircfs)      \n"
"        -p             -- Prompt for IRC server password.                  \n"
"        -!             -- Disable join/part IRC messages.                  \n"
"                          Useful for proxy setups.                         \n"
"                                                                           \n"
"", app);
}
