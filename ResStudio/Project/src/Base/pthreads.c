#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define mutex_init mutex_init_
#define mutex_destroy mutex_destroy_
#define mutex_lock mutex_lock_
#define mutex_unlock mutex_unlock_

void mutex_init(pthread_mutex_t **mutex) {
	*mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(*mutex, NULL);	
}

void mutex_destroy(pthread_mutex_t **mutex) {
	pthread_mutex_destroy(*mutex);
	free(*mutex);
}

void mutex_lock(pthread_mutex_t **mutex) {
	pthread_mutex_lock(*mutex);
}

void mutex_unlock(pthread_mutex_t **mutex) {
	pthread_mutex_unlock(*mutex);
}
