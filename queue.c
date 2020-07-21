#include "queue.h"

struct Queue *add(struct Queue *queue, FILE *file, int client_fd) {
	if (!queue) {
		struct Queue *q = malloc(sizeof(struct Queue));
		q->file = file;
		assert(!pthread_mutex_init(&q->lock, NULL));
		return q;
	}

	pthread_mutex_lock(&queue->lock);

	struct Queue *prev = queue;
	struct Queue *current;

	pthread_mutex_lock(&prev->lock);

	while ((current = prev->next)) {
		pthread_mutex_lock(&current->lock);
		pthread_mutex_unlock(&prev->lock);
		prev = current;
	}

	prev->next = malloc(sizeof(struct Queue));
	prev->next->file = file;
	prev->next->client_fd = client_fd;
	assert(!pthread_mutex_init(&prev->next->lock, NULL));

	pthread_mutex_unlock(&prev->lock);
	pthread_mutex_unlock(&queue->lock);

	return queue;
}

struct Queue *del(struct Queue *queue, int nth) {
	assert(queue);
	pthread_mutex_lock(&queue->lock);

	if (!nth) {
		struct Queue *q = queue->next;
		free(queue);
		return q;
	}

	struct Queue *prev = queue;
	struct Queue *current;

	pthread_mutex_lock(&prev->lock);

	while (nth--) {
		current = prev->next;
		pthread_mutex_lock(&current->lock);
		if (nth) {
			pthread_mutex_unlock(&prev->lock);
			prev = current;
		}
	}

	if (!current) {
		prev->next = NULL;
	} else {
		prev->next = current->next;
	}

	free(current);
	pthread_mutex_unlock(&prev->lock);
	pthread_mutex_unlock(&queue->lock);
	return queue;
}
