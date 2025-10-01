#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

pthread_mutex_t m_count = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_resource = PTHREAD_MUTEX_INITIALIZER;

int read_count = 0;
void spinlock_s(int i) {
	struct timespec curr;
	long long curr_ms, start_ms;
	timespec_get(&curr, TIME_UTC);
	start_ms = curr.tv_sec*1000 + curr.tv_nsec/1000;
	do {
		timespec_get(&curr, TIME_UTC);
		curr_ms = curr.tv_sec*1000 + curr.tv_nsec/1000;
	}while(curr_ms - start_ms < i*1000);
}

void *reader(void *arg) {
    int id = *((int *)arg);

    while (1) {
        pthread_mutex_lock(&m_count);
        read_count++;
        if (read_count == 1) {
            pthread_mutex_lock(&m_resource);
        }
        pthread_mutex_unlock(&m_count);

        // Чтение ресурса
        printf("Читатель %d читает ресурс\n", id);
		spinlock_s(1);

        // Критическая секция для обновления читателей
        pthread_mutex_lock(&m_count);
        read_count--;
        if (read_count == 0) {
            // Последний читатель отпускает ресурс
            pthread_mutex_unlock(&m_resource);
        }
        pthread_mutex_unlock(&m_count);
		sleep(2);
    }
    return NULL;
}

void *writer(void *arg) {
    int id = *((int *)arg);

    while (1) {
        // Писатель пытается захватить ресурс
        pthread_mutex_lock(&m_resource);

        // Запись в ресурс
        printf("Писатель %d пишет в ресурс\n", id);
        spinlock_s(2); // имитация записи

        pthread_mutex_unlock(&m_resource);
		sleep(2);
    }
    return NULL;
}

int main() {
    pthread_t r1, r2, w1;
    int id1 = 1, id2 = 2, id3 = 1;

    pthread_create(&r1, NULL, reader, &id1);
    pthread_create(&r2, NULL, reader, &id2);
    pthread_create(&w1, NULL, writer, &id3);

    pthread_join(r1, NULL);
    pthread_join(r2, NULL);
    pthread_join(w1, NULL);

    return 0;
}

