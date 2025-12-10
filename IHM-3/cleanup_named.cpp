#include <iostream>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
using namespace std;

const char* SHM_NAME = "/shm_boltuny23";
const char* PRINT_SEM = "/sem_bolt_print";

int main(){
    sem_unlink(PRINT_SEM);

    for (int i = 0; i < 20; i++){
        string s = "/sem_phone_" + to_string(i);
        sem_unlink(s.c_str());
    }

    shm_unlink(SHM_NAME);
    cout << "Все именованные объекты удалены\n";
    return 0;
}

