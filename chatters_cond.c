#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdarg.h>

typedef enum {
    WAITING,
    TALKING
} state_t;

int N;
atomic_int *busy;
pthread_mutex_t mutex;
pthread_cond_t *conds;
volatile sig_atomic_t running = 1;
FILE *logfile;

void handle_sigint(int sig) {
    running = 0;
}

void log_event(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    fflush(stdout);
    vfprintf(logfile, fmt, args);
    va_end(args);
    fflush(logfile);
}

void* chatter_thread(void *arg) {
    int id = *(int*)arg;

    while (running) {
        int action = rand() % 2;

        if (action == 0) {
            pthread_mutex_lock(&mutex);
            log_event("[Болтун %d] ждет звонка\n", id);
            while (!busy[id] && running)
                pthread_cond_wait(&conds[id], &mutex);
            if (busy[id]) {
                log_event("[Болтун %d] разговаривает\n", id);
            }
            pthread_mutex_unlock(&mutex);
        } else {
            while (running) {
                int target = rand() % N;
                if (target == id) continue;

                pthread_mutex_lock(&mutex);
                if (!busy[target]) {
                    busy[target] = 1;
                    busy[id] = 1;
                    log_event("[Болтун %d] дозвонился до %d\n", id, target);
                    pthread_mutex_unlock(&mutex);

                    sleep(rand() % 3 + 1);

                    pthread_mutex_lock(&mutex);
                    busy[target] = 0;
                    busy[id] = 0;
                    pthread_cond_signal(&conds[target]);
                    log_event("[Болтун %d] закончил разговор с %d\n", id, target);
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                pthread_mutex_unlock(&mutex);
            }
        }
        sleep(1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Использование: %s <N> <logfile>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, handle_sigint);
    srand(time(NULL));

    N = atoi(argv[1]);
    logfile = fopen(argv[2], "w");

    busy = malloc(sizeof(atomic_int) * N);
    conds = malloc(sizeof(pthread_cond_t) * N);
    pthread_t threads[N];
    int ids[N];

    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < N; i++) {
        busy[i] = 0;
        pthread_cond_init(&conds[i], NULL);
        ids[i] = i;
        pthread_create(&threads[i], NULL, chatter_thread, &ids[i]);
    }

    for (int i = 0; i < N; i++)
        pthread_join(threads[i], NULL);

    fclose(logfile);
    free(busy);
    free(conds);
    return 0;
}
