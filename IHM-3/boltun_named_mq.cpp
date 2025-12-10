#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <mqueue.h>
#include <vector>
#include <cstdlib>
#include <cstring>
using namespace std;

volatile sig_atomic_t stop_flag = 0;
void sigint(int) { stop_flag = 1; }

const char* SHM_NAME  = "/shm_boltuny23";
const char* PRINT_SEM = "/sem_bolt_print";
const char* MQ        = "/mq_obs";
const int MAX_N       = 20;

struct SharedData {
    int N;
    int phone_busy[MAX_N];
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Использует: ./boltun_named_mq <id>\n";
        return 1;
    }

    signal(SIGINT, sigint);
    int id = atoi(argv[1]);

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return 1; }

    SharedData* sh = (SharedData*)mmap(nullptr, sizeof(SharedData),
                                       PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED) { perror("mmap"); return 1; }

    sem_t* print_sem = sem_open(PRINT_SEM, 0);
    if (print_sem == SEM_FAILED) { perror("sem_open print"); return 1; }

    mqd_t mq = mq_open(MQ, O_WRONLY | O_NONBLOCK);
    if (mq == (mqd_t)-1 && errno != ENOENT) {
        perror("mq_open MQ");
    }

    vector<sem_t*> phone_sem(sh->N);
    for (int i = 0; i < sh->N; i++) {
        string name = "/sem_phone_" + to_string(i);
        phone_sem[i] = sem_open(name.c_str(), 0);
        if (phone_sem[i] == SEM_FAILED) {
            perror(("sem_open phone " + to_string(i)).c_str());
            return 1;
        }
    }

    srand(time(NULL) ^ getpid());

    while (!stop_flag) {
        bool waiting = rand() % 2 == 0;

        if (waiting) {
            sem_wait(print_sem);
            cout << "Болтун " << id << " ждёт звонка\n";
            sem_post(print_sem);
            usleep((rand() % 300 + 200) * 1000);
            continue;
        }

        int target;
        do {
            target = rand() % sh->N;
        } while (target == id);

        if (sem_trywait(phone_sem[target]) == 0) {
            sh->phone_busy[id]     = 1;
            sh->phone_busy[target] = 1;

            string msg = "Болтун " + to_string(id) + " дозвонился до " + to_string(target);

            sem_wait(print_sem);
            cout << msg << endl;
            sem_post(print_sem);

            if (mq != (mqd_t)-1) {
                if (mq_send(mq, msg.c_str(), msg.size() + 1, 0) == -1) {
                    if (errno == EAGAIN) {
                    } else {
                        perror("mq_send");
                    }
                }
            }

            usleep((rand() % 300 + 300) * 1000);

            sh->phone_busy[id]     = 0;
            sh->phone_busy[target] = 0;
            sem_post(phone_sem[target]);
        }
        usleep(10000);
    }

    // Чистка
    for (auto s : phone_sem) sem_close(s);
    sem_close(print_sem);
    if (mq != (mqd_t)-1) mq_close(mq);
    munmap(sh, sizeof(SharedData));
    close(fd);
    return 0;
}