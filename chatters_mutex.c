#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <string.h>

/* =======================
   Задача о болтунах (вариант 23)
   Версия с:
   - мьютексами
   - условными переменными
   - атомарными переменными
   - вводом из командной строки ИЛИ конфиг-файла
   ======================= */

int N;
atomic_int *busy;                 // 0 — свободен, 1 — занят
pthread_mutex_t mutex;
pthread_cond_t *conds;
volatile sig_atomic_t running = 1;
FILE *logfile;

/* ---------- Обработка SIGINT ---------- */
void handle_sigint(int sig) {
    running = 0;
}

/* ---------- Логирование ---------- */
void log_event(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);
    fflush(stdout);

    va_list args2;
    va_start(args2, fmt);
    vfprintf(logfile, fmt, args2);
    fflush(logfile);

    va_end(args2);
    va_end(args);
}

/* ---------- Чтение конфигурации ---------- */
void read_config(const char *filename, int *N, char *logname) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Не удалось открыть конфигурационный файл");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        if (sscanf(line, "N=%d", N) == 1)
            continue;

        if (sscanf(line, "LOGFILE=%255s", logname) == 1)
            continue;
    }

    fclose(f);
}

/* ---------- Поток болтуна ---------- */
void* chatter_thread(void *arg) {
    int id = *(int*)arg;

    while (running) {
        int action = rand() % 2; // 0 — ждать, 1 — звонить

        if (action == 0) {
            pthread_mutex_lock(&mutex);
            log_event("[Болтун %d] ждет звонка\n", id);
            while (!busy[id] && running)
                pthread_cond_wait(&conds[id], &mutex);

            if (busy[id])
                log_event("[Болтун %d] разговаривает\n", id);

            pthread_mutex_unlock(&mutex);
        }
        else {
            while (running) {
                int target = rand() % N;
                if (target == id)
                    continue;

                pthread_mutex_lock(&mutex);
                if (!busy[target] && !busy[id]) {
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

/* ---------- main ---------- */
int main(int argc, char *argv[]) {
    char logname[256];

    if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        // режим конфигурационного файла
        read_config(argv[2], &N, logname);
    }
    else if (argc == 3) {
        // обычный режим
        N = atoi(argv[1]);
        strncpy(logname, argv[2], sizeof(logname));
    }
    else {
        printf("Использование:\n");
        printf("  %s <N> <logfile>\n", argv[0]);
        printf("  %s -c <configfile>\n", argv[0]);
        return 1;
    }

    logfile = fopen(logname, "w");
    if (!logfile) {
        perror("Не удалось открыть лог-файл");
        return 1;
    }

    signal(SIGINT, handle_sigint);
    srand(time(NULL));

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
    pthread_mutex_destroy(&mutex);

    return 0;
}
