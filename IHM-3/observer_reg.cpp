#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <mqueue.h>
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

using namespace std;

volatile sig_atomic_t stop_flag = 0;
void sigint_handler(int) { stop_flag = 1; }

const char* SHM_NAME = "/shm_boltuny23";
const char* REG_SEM  = "/sem_bolt_reg";

const int MAX_OBS  = 20;
const int OBS_NAME = 64;
const int MSG_SIZE = 256;

struct SharedData {
    int N;
    int phone_busy[20];
    char observer_names[MAX_OBS][OBS_NAME];
    int observer_count;
} *sh;

int main() {
    signal(SIGINT,  sigint_handler);
    signal(SIGTERM, sigint_handler);

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return 1; }

    sh = (decltype(sh))mmap(NULL, sizeof(*sh), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    sem_t* reg_sem = sem_open(REG_SEM, 0);
    if (reg_sem == SEM_FAILED) { perror("sem_open reg_sem"); return 1; }

    // Короткое имя очереди 
    char mqname[16];
    snprintf(mqname, sizeof(mqname), "/o%d", getpid());   // например: /o31415

    mq_unlink(mqname); // если осталось от прошлого запуска

    struct mq_attr attr = {};
    attr.mq_maxmsg  = 20;
    attr.mq_msgsize = MSG_SIZE;

    mqd_t mq = mq_open(mqname, O_CREAT | O_RDONLY | O_NONBLOCK, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        sem_close(reg_sem);
        munmap(sh, sizeof(*sh));
        close(fd);
        return 1;
    }

    // Регистрируем себя
    sem_wait(reg_sem);
    if (sh->observer_count < MAX_OBS) {
        strncpy(sh->observer_names[sh->observer_count], mqname, OBS_NAME-1);
        sh->observer_names[sh->observer_count][OBS_NAME-1] = '\0';
        sh->observer_count++;
        cout << "Наблюдатель pid=" << getpid() << " зарегистрирован как " << mqname << endl;
    } else {
        cout << "Слишком много наблюдателей!" << endl;
    }
    sem_post(reg_sem);

    // Приём сообщений
    char buf[MSG_SIZE + 1];
    while (!stop_flag) {
        ssize_t n = mq_receive(mq, buf, MSG_SIZE, NULL);
        if (n >= 0) {
            buf[n] = '\0';
            cout << "[OBS " << getpid() << "] " << buf << endl;
        } else if (errno != EAGAIN) {
            if (errno != EINTR) perror("mq_receive");
        } else {
            usleep(50000);
        }
    }

    // Чистка
    mq_close(mq);
    mq_unlink(mqname);
    sem_close(reg_sem);
    munmap(sh, sizeof(*sh));
    close(fd);

    cout << "Наблюдатель " << getpid() << " завершил работу" << endl;
    return 0;
}