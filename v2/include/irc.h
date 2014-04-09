/**
 * @file irc.h
 * @author Joseph Kinsella
 * @date September, 2013
 * @brief Header for simple IRC processing
 */

#ifndef IRC_H_
# define IRC_H_

# define IRC_BUFFER_SIZE	512

# define IRC_MSG_PRIVMSG	"PRIVMSG"
# define SIRC_MSG_PRIVMSG	7

# define IRC_MSG_NOTICE		"NOTICE"
# define SIRC_MSG_NOTICE	6

# define IRC_MSG_JOIN		"JOIN"
# define SIRC_MSG_JOIN		4

# define IRC_MSG_PART		"PART"
# define SIRC_MSG_PART		4

# define IRC_MSG_QUIT		"QUIT"
# define SIRC_MSG_QUIT		4

# define IRC_MSG_NICK		"NICK"
# define SIRC_MSG_NICK		4

# define IRC_MSG_PING		"PING"
# define SIRC_MSG_PING		4

/*
 * an IRC message object
 */
struct irc_s
{
	char buffer[IRC_BUFFER_SIZE+1];	/* container */

	char *source;	/* source */
	char *target;	/* target */
	char *command;	/* command */
	char *message;	/* command parameters (message) */

	struct {
		char *servername;
		char *nickname;
		char *username;
		char *hostname;
	} src;

	void * data;
};

/*
 * An IRC dispatch configuration object
 */
struct irc_dispatch_s
{
	void (*on_numeric)(struct irc_s *irc);
	void (*on_ping)(struct irc_s *irc);
	void (*on_privmsg)(struct irc_s *irc);
	void (*on_notice)(struct irc_s *irc);
	void (*on_join)(struct irc_s *irc);
	void (*on_part)(struct irc_s *irc);
	void (*on_quit)(struct irc_s *irc);
	void (*on_nick)(struct irc_s *irc);
};

/**
 * @name irc_dispatch
 * @brief Dispatches to functions based on input irc/dispatch object.
 * @param [in] irc IRC message object
 * @param [in] dispatch Dispatch message configuration
 */
void irc_dispatch(struct irc_s * const irc,
		struct irc_dispatch_s * const dispatch);

/**
 * @name irc_parse
 * @brief Breaks up an IRC message into appropriate chunks for easy use.
 * @param [in] buffer an IRC message conforming to RFC 1459 / 2812
 *
 * @retval NULL Something went wrong (Out of Memory, or improper message)
 * @retval an irc_s object for later use.
 */
struct irc_s *irc_parse(const char * const buffer);

#endif	/* !IRC_H */
