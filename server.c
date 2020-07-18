#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define QUEUE 10

#define CONTENT_TYPE(extension) \
	!strcmp("html", extension) ? "text/html; charset=utf-8"    : \
	!strcmp("css", extension)  ? "text/css; charset=utf-8"     : \
	!strcmp("txt", extension)  ? "text/plain; charset=utf-8"   : \
	!strcmp("js", extension)   ? "text/javascript"             : \
	!strcmp("ico", extension)  ? "image/x-icon"                : \
	!strcmp("gif", extension)  ? "image/gif"                   : \
	!strcmp("png", extension)  ? "image/png"                   : \
	!strcmp("jpg", extension)  ? "image/jpeg"                  : \
	!strcmp("jpeg", extension) ? "image/jpeg"                  : \
	!strcmp("mpeg", extension) ? "video/mpeg"                  : \
	!strcmp("mp4", extension)  ? "video/mp4"                   : \
	!strcmp("webm", extension) ? "video/webm"                  : \
	!strcmp("zip", extension)  ? "application/zip"             : \
	!strcmp("ogg", extension)  ? "application/ogg"             : \
	!strcmp("xml", extension)  ? "application/xml"             : \
	!strcmp("json", extension) ? "application/json"            : \
	!strcmp("pdf", extension)  ? "application/pdf"             : \
	"text/plain; charset=utf-8"

#define OK_RESPONSE \
	"HTTP/1.0 200 OK\n" \
	"Content-Type: %s\n" \
	"Content-Length: %d\n" \
	"Connection: Closed\n\n"

#define SEND(fd, buffer, message_size) \
{ \
	ssize_t sent = send(fd, buffer, message_size, 0); \
	assert(sent != -1); \
	if (sent < message_size) { \
		fprintf(stderr, "Warning: couldn't send whole message to %d\n", fd); \
	} \
}

static inline int init_server_socket();
static inline int count_char(char c, char *buffer);
static inline char *get_file_extension(char *ptr);
static inline int read_word(int fd, char *buffer);
static inline int send_file(int fd, char *buffer, int buffer_size);
static inline int resolve(int client_fd);
static inline int accept_and_serve(int server_fd);

int
init_server_socket()
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	int sock_opt = 1;
	struct sockaddr_in addr;

	// create socket
	assert(server_fd != 0);
	assert(setsockopt(
		server_fd,
		SOL_SOCKET,
		SO_REUSEADDR | SO_REUSEPORT,
		&sock_opt,
		sizeof(sock_opt)) == 0);

	// assign socket
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);

	// bind socket and listen
	assert(bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0);
	assert(listen(server_fd, QUEUE) == 0);

	return server_fd;
}

int
count_char(char c, char *buffer)
{
	// buffer must be '\0' terminated
	int count = 0;
	do if (*buffer == c) count++;
	while (*(++buffer));
	return count;
}

char *
get_file_extension(char *ptr)
{
	// return a pointer to where the file extension starts
	while (count_char('.', ptr)) {
		while (*(ptr++) != '.');
	}

	// return NULL if the filename ends in '.'
	return *ptr ? ptr : NULL;
}

int
read_word(int fd, char *buffer)
{
	// read a word from file descriptor into the buffer
	// return the length of the word
	char c[1] = {'\0'};
	if (!read(fd, &c, 1)) {
		return 0;
	}

	int i = 0;
	for (; c[0] != ' '; i++) {
		buffer[i] = c[0];
		if (!read(fd, &c, 1)) {
			return i;
		}
	}

	return i;
}

int
send_file(int fd, char *buffer, int buffer_size)
{
	uint32_t file_size = 0;
	FILE *f = fopen(buffer, "rb");

	if (!f) {
		fprintf(stderr, "Couldn't send %s: file doesn't exist\n", buffer);
		return 0;
	}

	// get file size and rewind file
	while (fgetc(f) != EOF) file_size++;
	rewind(f);

	// create and send response header
	char *content_type = CONTENT_TYPE(get_file_extension(buffer));
	memset(buffer, 0, buffer_size);
	sprintf(buffer, OK_RESPONSE, content_type, file_size);
	SEND(fd, buffer, (int) strlen(buffer));

	// send file incrementally
	int over = 0;
	while (!over) {
		int i = 0;
		for (; i < buffer_size; i++) {
			if (!file_size--) {
				over = 1;
				break;
			}
			buffer[i] = fgetc(f);
		}
		SEND(fd, buffer, i);
	}

	// send ending newline
	buffer[0] = '\n';
	send(fd, buffer, 1, 0);

	fclose(f);
	return 1;
}

int
resolve(int client_fd)
{
	int buffer_size = 256;
	char buffer[buffer_size];
	int reset_size;

	// read method
	memset(buffer, 0, buffer_size);
	reset_size = read_word(client_fd, buffer);

	printf("Recieving request from: %d\n", client_fd);
	printf("Request method: %s\n", buffer);

	if (!strcmp("GET", buffer)) {
		// skip leading slash from stream
		read(client_fd, buffer, 1);

		// read path
		memset(buffer, 0, reset_size);
		read_word(client_fd, buffer);
		printf("Request path: %s\n", buffer);

		// send index.html by default
		if (!buffer[0]) {
			strncpy(buffer, "index.html", 11);
		}

		if (!send_file(client_fd, buffer, buffer_size)) {
			return 0;
		}
	}

	// read the remains of the response
	for (;;) {
		if (read(client_fd, buffer, buffer_size) < buffer_size) {
			break;
		}
	}

	printf("\n");
	return 1;
}

int
accept_and_serve(int server_fd)
{
	struct sockaddr_in addr;
	unsigned int addr_size = sizeof(addr);
	int client_fd;

	// accept and serve client
	memset(&addr, 0, sizeof(addr));
	assert((client_fd = accept(
		server_fd, (struct sockaddr *)&addr, &addr_size)) >= 0);
	resolve(client_fd);

	close(client_fd);
	return 0;
}

int
main()
{
	chroot("./");
	int server_fd = init_server_socket();

	for (;;) {
		accept_and_serve(server_fd);
	}

	close(server_fd);
	return 0;
}
