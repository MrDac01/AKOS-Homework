#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <cstdlib>
#include <vector>
using namespace std;

volatile sig_atomic_t stop_flag = 0;
void sigint(int){ stop_flag = 1; }

const char* SHM_NAME = "/shm_boltuny23";
const char* PRINT_SEM = "/sem_bolt_print";

const int MAX_N = 20;

struct SharedData {
    int N;
    int phone_busy[MAX_N];
};

int main(int argc, char* argv[]){
    if (argc < 2){
        cerr << "Использует: ./boltun_named <id>\n";
        return 1;
    }

    signal(SIGINT, sigint);
    int id = atoi(argv[1]);

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    SharedData* sh = (SharedData*) mmap(nullptr, sizeof(SharedData),
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, 0);

    sem_t* print_sem = sem_open(PRINT_SEM, 0);

    vector<sem_t*> phone_sem(sh->N);
    for (int i = 0; i < sh->N; i++) {
        string name = "/sem_phone_" + to_string(i);
        phone_sem[i] = sem_open(name.c_str(), 0);
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
            sh->phone_busy[target] = 1;
            sh->phone_busy[id] = 1;

            sem_wait(print_sem);
            cout << "Болтун " << id << " дозвонился до " << target << endl;
            sem_post(print_sem);

            usleep((rand()%300 + 300)*1000);

            sh->phone_busy[target] = 0;
            sh->phone_busy[id] = 0;

            sem_post(phone_sem[target]);
        }
    }

    return 0;
}
