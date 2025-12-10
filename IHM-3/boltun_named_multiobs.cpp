#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <mqueue.h>
#include <vector>
#include <cstring>
#include <cstdlib>
using namespace std;

volatile sig_atomic_t stop_flag = 0;
void sigint(int){ stop_flag = 1; }

const char* SHM_NAME = "/shm_boltuny23";
const char* PRINT_SEM = "/sem_bolt_print";
const char* REG_SEM   = "/sem_bolt_reg";

const int MAX_OBS = 20;
const int OBS_NAME = 64;
const int MAX_N = 20;
const int MSG_SIZE = 256;

struct SharedData {
    int N;
    int phone_busy[MAX_N];
    char observer_names[MAX_OBS][OBS_NAME];
    int observer_count;
};

int main(int argc, char* argv[]){
    if (argc < 2){
        cerr << "Использует: ./boltun_named_multiobs <id>\n";
        return 1;
    }

    signal(SIGINT, sigint);
    int id = atoi(argv[1]);

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);

    SharedData* sh = (SharedData*) mmap(nullptr, sizeof(SharedData),
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, 0);

    sem_t* print_sem = sem_open(PRINT_SEM, 0);
    sem_t* reg_sem   = sem_open(REG_SEM, 0);

    vector<sem_t*> phone_sem(sh->N);
    for (int i = 0; i < sh->N; i++){
        string s = "/sem_phone_" + to_string(i);
        phone_sem[i] = sem_open(s.c_str(), 0);
    }

    srand(time(NULL) ^ getpid());

    while (!stop_flag){
        bool call = rand() % 2;

        if (!call){
            sem_wait(print_sem);
            cout << "Болтун " << id << " ждёт звонка\n";
            sem_post(print_sem);
            usleep((rand()%300 + 200)*1000);
            continue;
        }

        int target = rand() % sh->N;
        if (target == id) continue;

        if (sem_trywait(phone_sem[target]) == 0){
            sh->phone_busy[id] = 1;
            sh->phone_busy[target] = 1;

            string msg = "Болтун " + to_string(id) + " дозвонился до " + to_string(target);

            sem_wait(print_sem);
            cout << msg << endl;
            sem_post(print_sem);

            sem_wait(reg_sem);

            for (int i = 0; i < sh->observer_count; i++){
                if (sh->observer_names[i][0] != 0){
                    mqd_t mq = mq_open(sh->observer_names[i], O_WRONLY);
                    if (mq != (mqd_t)-1){
                        mq_send(mq, msg.c_str(), msg.size()+1, 0);
                        mq_close(mq);
                    }
                }
            }

            sem_post(reg_sem);

            usleep((rand()%300 + 300)*1000);

            sh->phone_busy[id] = 0;
            sh->phone_busy[target] = 0;
            sem_post(phone_sem[target]);
        }
    }

    return 0;
}