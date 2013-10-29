/**
 * server.h
 * @creation 21.7.2013
 * @author Allek
**/

#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * Data structure for message received from a client.
 * @since 0.0.1
**/
typedef struct client_msg_s {
	int socket_fd;
	struct sockaddr_in *client_addr;
	const char *msg;
	int len;
} client_msg;

/**
 * Initialize communications with a requesting client.
 * @since 0.0.1
**/
void process_con(int socket_fd, struct sockaddr_in *client_addr); // process connection

/**
 * Process a client message.
 * @since 0.0.1
**/
void process_msg(client_msg * msg_s); // process a message

/**
 * Log a message to the server console, as well as (eventually)
 * the server log file.
 * @since 0.0.1
**/
void slog(char *msg);

/**
 * Zero-out a provided buffer.
 * @since 0.0.1
**/
void flush(char *buf);

/**
 * Handle a caught signal.
 * @since 0.0.1
**/
void sighandle(int signal);

/**
 * What should've been called by sighandle earlier, but oh well.
 * @since 0.0.2
**/
void cleanleave();

/**
 * Spam the user until they successfully log in.
 * @since 0.0.2
**/
void prompt_user_login(int socket_fd, struct sockaddr_in *client_addr);

/**
 * Send the n00b1-3r, user help upon receiving /help.
 * @since 0.0.2
**/
void send_help(int socket_fd);