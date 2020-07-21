#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

struct Queue {
	FILE *file;
	int client_fd;
	pthread_mutex_t lock;
	struct Queue *next;
};

struct Queue *add(struct Queue *queue, FILE *file, int client_fd);
struct Queue *del(struct Queue *queue, int nth);

#endif // _QUEUE_H_
