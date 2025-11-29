#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sys/wait.h>

using namespace std;

volatile sig_atomic_t stop_flag = 0;
void handle_sigint(int){ stop_flag = 1; }

const int MAX_N = 20;

struct SharedData {
    int N;
    int phone_busy[MAX_N];
    sem_t print_sem;
    sem_t phone_sem[MAX_N];
};

int main(int argc, char* argv[]){
    if (argc < 2) {
        cerr << "Usage: ./boltuny_anon <N>\n";
        return 1;
    }

    signal(SIGINT, handle_sigint);
    int N = atoi(argv[1]);
    if (N <= 0 || N > MAX_N) {
        cerr << "N должен быть 1.." << MAX_N << "\n";
        return 1;
    }

    // Shared memory (anonymous)
    SharedData* sh = (SharedData*) mmap(
        nullptr, sizeof(SharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS, -1, 0
    );

    if (sh == MAP_FAILED) { perror("mmap"); return 1; }

    sh->N = N;
    for (int i = 0; i < N; i++) sh->phone_busy[i] = 0;

    sem_init(&sh->print_sem, 1, 1);
    for (int i = 0; i < N; i++)
        sem_init(&sh->phone_sem[i], 1, 1);

    vector<pid_t> pids;

    for (int id = 0; id < N; id++) {
        pid_t pid = fork();

        if (pid == 0) {
            srand(time(NULL) ^ getpid());

            while (!stop_flag) {
                bool call = rand() % 2;

                if (!call) {
                    sem_wait(&sh->print_sem);
                    cout << "Болтун " << id << " ждёт звонка\n";
                    sem_post(&sh->print_sem);
                    usleep((rand()%300 + 200) * 1000);
                    continue;
                }

                int target = rand() % N;
                if (target == id) continue;

                if (sem_trywait(&sh->phone_sem[target]) == 0) {
                    sh->phone_busy[target] = 1;
                    sh->phone_busy[id] = 1;

                    sem_wait(&sh->print_sem);
                    cout << "Болтун " << id
                         << " дозвонился до "
                         << target << endl;
                    sem_post(&sh->print_sem);

                    usleep((rand()%300 + 300) * 1000);

                    sh->phone_busy[id] = 0;
                    sh->phone_busy[target] = 0;
                    sem_post(&sh->phone_sem[target]);
                }
            }

            sem_wait(&sh->print_sem);
            cout << "Болтун " << id << " завершает работу\n";
            sem_post(&sh->print_sem);
            _exit(0);
        }

        pids.push_back(pid);
    }

    while (!stop_flag) pause();

    for (pid_t p : pids) kill(p, SIGINT);
    for (int i = 0; i < N; i++) wait(NULL);

    munmap(sh, sizeof(SharedData));

    return 0;
}

