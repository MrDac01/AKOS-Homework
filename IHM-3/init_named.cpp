#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <cstring>
using namespace std;

const char* SHM_NAME = "/shm_boltuny23";
const char* PRINT_SEM = "/sem_bolt_print";
const char* REG_SEM   = "/sem_bolt_reg";

const int MAX_N = 20;
const int MAX_OBS = 20;
const int OBS_NAME = 64;

struct SharedData {
    int N;
    int phone_busy[MAX_N];
    char observer_names[MAX_OBS][OBS_NAME];
    int observer_count;
};

int main(int argc, char* argv[]){
    if (argc < 2) {
        cerr << "Использует: ./init_named <N>\n";
        return 1;
    }

    int N = atoi(argv[1]);
    if (N <= 0 || N > MAX_N) {
        cerr << "N должен быть 1.." << MAX_N << endl;
        return 1;
    }

    shm_unlink(SHM_NAME);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        return 1;
    }

    SharedData* sh = (SharedData*) mmap(nullptr, sizeof(SharedData),
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    sh->N = N;
    for (int i = 0; i < MAX_N; i++)
        sh->phone_busy[i] = 0;

    sh->observer_count = 0;
    for (int i = 0; i < MAX_OBS; i++)
        sh->observer_names[i][0] = '\0';

    sem_unlink(PRINT_SEM);
    sem_t* ps = sem_open(PRINT_SEM, O_CREAT, 0666, 1);
    if (ps != SEM_FAILED) sem_close(ps);

    sem_unlink(REG_SEM);
    sem_t* rs = sem_open(REG_SEM, O_CREAT, 0666, 1);
    if (rs != SEM_FAILED) {
        sem_close(rs);                         
    }

    for (int i = 0; i < N; i++) {
        string s = "/sem_phone_" + to_string(i);
        sem_unlink(s.c_str());
        sem_t* ph = sem_open(s.c_str(), O_CREAT, 0666, 1);
        if (ph != SEM_FAILED) {
            sem_close(ph);
        }
    }

    munmap(sh, sizeof(SharedData));
    close(fd);

    cout << "init_named: создал SHM (size=" << sizeof(SharedData) << " bytes)\n";
    return 0;
}
