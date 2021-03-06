/**
 * chat.c
 * @creation 21.7.2013
 * @author Allek
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "chat.h"

void error(char *err_msg) {
	fprintf(stderr, "An error occurred while %s\n", err_msg);
	exit(1);
}

int send_msg(int sockfd, char *buffer) {
	/*char debug[20];
	sprintf(debug, "Sending %li byte message.", strlen(buffer));
	fprintf(stderr, "%s\n", debug);*/

	int sent, to_send;
	to_send = strlen(buffer);
	while (to_send > 0) {
		sent = send(sockfd, buffer, to_send, 0);
		if (sent == -1)
			return -1;
		to_send -= sent;
		buffer += sent;
	}
	return sent;
}

int get_line(int sockfd, char *dest_buffer) {

	//fprintf(stderr, "Gettin that line shiz.\n");

	char *current;
	current = dest_buffer;
	int result;
	while ((result = recv(sockfd, current, 1, 0)) == 1) {
		//fprintf(stderr, "Received character from socket: '%c'\n", *current);
		if ((*current) == '\n') {
			*current = '\0';
			return strlen(dest_buffer);
		} else if ((*current) == '\0') {
			return strlen(dest_buffer);
		}
		current++;
	}
	return result;
}

int yes_no() {
	char yes_no[4];
	fgets(yes_no, 4, stdin);
	switch (yes_no[0]) {
		case 'y':
		case 'Y': return 1;
		case 'n':
		case 'N': return 0;
	}
}

int login(const char *username, const char *password, struct user *start) {
	struct user *u =  get_user(username, start);

	if (u == NULL) // user doesn't exist
		return LOGIN_DNE;

	if (strcmp(password, u->password)) // passwords don't match
		return LOGIN_PASS_BAD;

	return LOGIN_GOOD;
}

int user_exists(const char *username, struct user *list_start) {
	return !(get_user(username, list_start) == NULL);
}

char *login_err_txt(int err_code) {
	switch (err_code) {
		case LOGIN_DNE:
			return "The provided username does not exist.";
		case LOGIN_PASS_BAD:
			return "The provided password was incorrect.";
		case LOGIN_GOOD:
			return "The login was a success.";
		default:
			return "Uhhh..... yeah.";
	}
}

// TODO add rooms...
struct user *chatdat_parse_users(FILE *chatdat) {
	printf("%s\n", "Parsing users from chat.dat...");
	if (chatdat == NULL)
		error("parsing chat.dat file");

	size_t ubuffer_byte_size = sizeof(struct user *) * USER_BUFFER_SIZE;

	struct user **users = (struct user **) malloc(ubuffer_byte_size); // array of users 0-19

	int ubuffers = 1; // # of 'full buffer sizes' (keep track of allocation)
	int cur_user_i = 0;
	int num_this_buf = 0;

	const char *line;
	while ((line = read_line(chatdat)) != NULL) {
		int line_type = chatdat_line_type(line);
		//printf("Line '%s' parsed to be %s.\n", line, chatdat_type_string(line_type));
		if (line_type == CHATDAT_USER) {
			struct user *u;
			if ((u = chatdat_parse_uline(line)) == NULL) {
				//printf("Line judged invalid.\n");
				continue;
			} else {
				if (num_this_buf == 20) {
					//printf("Buffer limit reached, extending array.\n");
					users = realloc(users, (ubuffer_byte_size * (++ubuffers)));
					num_this_buf = 0;
				}

				//printf("Line judged to be valid User line, adding struct to array.\n");
				users[cur_user_i] = u;
				if (cur_user_i > 0) {
					users[cur_user_i - 1]->next = users[cur_user_i];
				}
				cur_user_i++; num_this_buf++;
			}
		}
	}

	printf("Total of %i users parsed.\n", (cur_user_i));

	struct user *start = users[0];
	free(users);
	return start;
}

char *chatdat_type_string(int type) {
	switch (type) {
		case CHATDAT_BLANK: return "Blank";
		case CHATDAT_COMMENT: return "Comment";
		case CHATDAT_USER: return "User";
		case CHATDAT_ROOM: return "Chatroom";
		default: return "Unknown";
	}
}

int chatdat_line_type(const char *line) {
	char *token = chatdat_get_token(line);
	switch (*token) {
		case '\r':
		case '\n':
		case '\0':	return CHATDAT_BLANK;
		case '#':	return CHATDAT_COMMENT;
		case '$':	return CHATDAT_USER;
		case '@':	return CHATDAT_ROOM;
		default:	return CHATDAT_UNKNOWN;
	}
}

char *chatdat_get_token(const char *line) {
	int chari = 0;
	while (line[chari] == ' ')
		chari++;
	return (char *) (line + chari);
}

char *chatdat_clean_line(const char *line) {
	char *token = chatdat_get_token(line);
	return (char *) (token + 1);
}

struct user *chatdat_parse_uline(const char *line) {
	char *clean_line = chatdat_clean_line(line);

	//printf("Attempting to parse line '%s' into a user struct.\n", line);
	struct user *u = malloc(sizeof(struct user));

	char *tok;
	tok = strtok(clean_line, "|");
	//printf("Username token: '%s'\n", tok);
	u->username = tok;
	tok = strtok(NULL, "|");
	//printf("Password token: '%s'\n", tok);
	u->password = tok;

	if (((tok = strtok(NULL, "|")) != NULL)
		|| !chatdat_valid_string(u->username) || !chatdat_valid_string(u->password)) {
		//printf("Parsed token invalid, freeing struct and returning NULL.\n");
		free(u);
		return NULL;
	}

	return u;
}

int chatdat_valid_string(const char *string) {
	if (string == NULL) return 0;
	if (strchr(string, '#') != NULL) return 0;
	if (strchr(string, '$') != NULL) return 0;

	int i;
	for (i = 0; i < strlen(string); i++) {
		if (!isalnum(string[i]))
			return 0;
	}

	return 1;
}

struct user *get_user(const char *username, struct user *start) {
	if (start == NULL) { // No list to begin with
		printf("User list empty.\n");
		return NULL;
	}

	while (start != NULL) {	// Woohoo, serial search!
		fprintf(stderr, "Is %s = %s? ", username, start->username);
		if (!strcmp(username, start->username)) {
			printf("Yes!\n");
			return start;
		}
		printf("Nope.\n");
		start = start->next;
	}
	printf("No matches found.\n");
	return NULL;
}

void print_user_info(struct user *u) {
	printf("Hello, my name is %s.\n", u->username);
}

void chain_user_info(struct user *start) {
	while (start != 0) {
		print_user_info(start);
		start = start->next;
	}
}

void free_user_chain(struct user *start) {
	struct user *next;
	while (start != 0) {
		/*char debug_str[60];
		sprintf(debug_str, "Freeing user %s with password %s, %li bytes.",
			start->username, start->password, sizeof(struct user));
		printf("%s\n", debug_str);*/

		next = start->next;
		free(start);
		start = next;
	}
}


char *read_line(FILE *file) {

	char *line;
	int buffer_sizes,
		total_read,
		this_lb;

	if (file == NULL) // File pointer null
		error("reading line from null file pointer");

	buffer_sizes = 1;
	if ((line = (char *) malloc(LINE_BUFFER_SIZE * buffer_sizes * sizeof(char))) == NULL)
		error("allocating memory for line");

	total_read = 0;
	this_lb = 0;
	int c;
	while ((c = fgetc(file)) != EOF) { // Not at end of file
		if (this_lb == 127) {
			if ((line = realloc(line, ((++buffer_sizes) * LINE_BUFFER_SIZE * sizeof(char)))) == NULL)
				error("reallocating memory for line");
			this_lb = 0;
		}


		if ((char) c == '\n') {
			line[total_read] = '\0';
			return line;
		}

		line[total_read] = (char) c;

		total_read++;
	}

	if (total_read == 0)
		return NULL;

	line[total_read] = '\0';
	return line;
}

#define CHATDAT_GEN_ADD_USER 0
#define CHATDAT_GEN_CHANGE_PWORD 1
#define CHATDAT_GEN_CHANGE_UNAME 2
#define CHATDAT_GEN_REMOVE_USER 3

void chatdat_gen() {
	printf("Chat.dat Generator\n");

	FILE *chatdat = fopen("chat.dat", "rw");
	if (chatdat == NULL)
		error("trying to open chat.dat");
	struct user *all_users = chatdat_parse_users(chatdat);

	int another = 1;
	while (another) {
		prompt:
		printf("\n");
		printf(" 0 - Add a user.\n");
		printf(" 1 - Change a user's password.\n");
		printf(" 2 - Change a user's name.\n");
		printf(" 3 - Remove a user.\n");
		printf("-1 - Exit\n");
		printf("What would you like to do? ");

		int decision = 0;
		char decision_s[4];
		fgets(decision_s, 4, stdin);
		decision = atoi(decision_s);
		switch (decision) {
			case -1:
				printf("Goodbye.\n");
				return;
				break;
			case CHATDAT_GEN_ADD_USER:
				chatdat_gen_add_user(all_users);
				break;
			case CHATDAT_GEN_CHANGE_PWORD:
				break;
			case CHATDAT_GEN_CHANGE_UNAME:
				break;
			case CHATDAT_GEN_REMOVE_USER:
				break;
			default:
				printf("Invalid option.\n");
				goto prompt;
				break;
		}
		printf("\n");
		
		printf("Would you like to do something else? (y/n) ");
		another = yes_no();
	}
	printf("Goodbye.\n");
}

void chatdat_gen_add_user(struct user *chatdat_users) {
	int good_name = 1;
	char uname[27];
	prompt_uname:
	printf("Enter name of new user (; to exit): ");
	fgets(uname, 27, stdin);

	// contains ';' = exit
	if (strstr(uname, ";"))
		return;

	if (!chatdat_valid_username(uname)) {
		// no spaces
		printf("Invalid username. Please omit whitespace and special characters (spaces, tabs, ;, etc.).\n");
		good_name = 0;
	} else if (get_user((const char *) uname, chatdat_users)) {
		// can't exist already
		printf("The designated user already exists.\n");
		good_name = 0;
	}

	// name didn't meet requirements
	if (!good_name) {
		printf("Would you like to try a different name? (y/n) ");
		if (yes_no())
			goto prompt_uname;
		else
			return;
	}
}

int chatdat_valid_username(const char *uname) {
	if (strstr(uname, " ")) // TODO: insert regex here to check for special chars
		return 0;

	return 1;
}