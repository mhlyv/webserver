NAME = webserver
CFLAGS = -Wall -Wextra -pedantic -O3
LDFLAGS = -Wall -Wextra -pedantic -O3
SRC = server.c queue.c
OBJ = ${SRC:.c=.o}

all: ${NAME}

.c.o:
	${CC} -c $< ${CFLAGS}

server.c: queue.h
queue.c: queue.h

${NAME}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS} 

clean:
	rm -rf ${OBJ} ${NAME}

execute: ${NAME}
	./${NAME}

install: ${NAME}
	cp ${NAME} /usr/local/bin/${NAME}

uninstall:
	rm -f /usr/local/bin/${NAME}
