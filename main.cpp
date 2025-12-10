#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_SOURCES 100

// Буфер для хранения чисел
typedef struct {
    int *data;              // массив чисел
    int size;               // текущее количество элементов
    int capacity;           // максимальная вместимость
    pthread_mutex_t mutex;  // мьютекс для защиты буфера
    pthread_cond_t cond;    // условная переменная для уведомления
} Buffer;

Buffer buffer;

// Функция сумматора
void* sum_thread_func(void* arg) {
    int *nums = (int*)arg; // приведение void* -> int*
    int a = nums[0];
    int b = nums[1];
    free(nums);

    // имитация работы сумматора (3-6 секунд)
    int delay = rand() % 4 + 3;
    sleep(delay);

    int sum = a + b;

    pthread_mutex_lock(&buffer.mutex);
    buffer.data[buffer.size++] = sum;
    printf("[Sum thread] %d + %d = %d. Buffer size: %d\n", a, b, sum, buffer.size);
    pthread_cond_signal(&buffer.cond); // уведомляем монитор
    pthread_mutex_unlock(&buffer.mutex);

    return NULL;
}

// Поток-монитор
void* monitor(void* arg) {
    while (1) {
        pthread_mutex_lock(&buffer.mutex);
        while (buffer.size < 2) {
            pthread_cond_wait(&buffer.cond, &buffer.mutex);
        }

        // Берем два числа из буфера
        int a = buffer.data[--buffer.size];
        int b = buffer.data[--buffer.size];
        printf("[Monitor] picked %d and %d from buffer. Buffer size: %d\n", a, b, buffer.size);

        pthread_mutex_unlock(&buffer.mutex);

        // Создаем поток-сумматор
        int *nums = (int*)malloc(2 * sizeof(int));
        nums[0] = a;
        nums[1] = b;

        pthread_t sum_thread;
        pthread_create(&sum_thread, NULL, sum_thread_func, (void*)nums);
        pthread_detach(sum_thread);
    }
}

// Потоки-производители
void* producer(void* arg) {
    int id = *(int*)arg;
    int value = id + 1; // числа от 1 до 100

    // случайная задержка 1-7 секунд
    int delay = rand() % 7 + 1;
    sleep(delay);

    pthread_mutex_lock(&buffer.mutex);
    buffer.data[buffer.size++] = value;
    printf("[Producer %d] added %d to buffer. Buffer size: %d\n", id, value, buffer.size);
    pthread_cond_signal(&buffer.cond); // уведомляем монитор
    pthread_mutex_unlock(&buffer.mutex);

    return NULL;
}

int main() {
    srand(time(NULL));

    // Инициализация буфера
    buffer.data = (int*)malloc(NUM_SOURCES * sizeof(int) * 2);
    buffer.size = 0;
    buffer.capacity = NUM_SOURCES * 2;
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_cond_init(&buffer.cond, NULL);

    pthread_t producers[NUM_SOURCES];
    int ids[NUM_SOURCES];

    // Запуск производителей
    for (int i = 0; i < NUM_SOURCES; i++) {
        ids[i] = i;
        pthread_create(&producers[i], NULL, producer, &ids[i]);
    }

    // Запуск монитора
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor, NULL);

    // Ждем завершения всех производителей
    for (int i = 0; i < NUM_SOURCES; i++) {
        pthread_join(producers[i], NULL);
    }

    // Ждем окончательный результат (финальная сумма)
    while (1) {
        pthread_mutex_lock(&buffer.mutex);
        if (buffer.size == 1) {
            printf("[Main] Final result: %d\n", buffer.data[0]);
            pthread_mutex_unlock(&buffer.mutex);
            break;
        }
        pthread_mutex_unlock(&buffer.mutex);
        sleep(1);
    }

    // Очистка ресурсов
    free(buffer.data);
    pthread_mutex_destroy(&buffer.mutex);
    pthread_cond_destroy(&buffer.cond);

    return 0;
}
