#include <iostream>
#include <mqueue.h>
#include <signal.h>
#include <unistd.h>
using namespace std;

volatile sig_atomic_t stop_flag = 0;
void sigint(int) { stop_flag = 1; }

const char* MQ       = "/mq_obs";
const int   MSG_SIZE = 256;

int main() {
    signal(SIGINT, sigint);

    mq_unlink(MQ);

    struct mq_attr attr;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = 100;     
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_curmsgs = 0;

    mqd_t mq = mq_open(MQ, O_CREAT | O_RDONLY | O_NONBLOCK, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open в observer_one");
        return 1;
    }

    cout << "Ожидаю сообщения в очереди " << MQ << "...\n\n";

    char buf[MSG_SIZE + 1];

    while (!stop_flag) {
        ssize_t n = mq_receive(mq, buf, MSG_SIZE, nullptr);
        if (n >= 0) {
            buf[n] = '\0';
            cout << "[OBS] " << buf << endl;
        } else {
            if (errno == EAGAIN) {
                usleep(50000); 
            } else if (errno != EINTR) {
                perror("mq_receive");
            }
        }
    }

    mq_close(mq);
    mq_unlink(MQ);
    cout << "\nНаблюдатель завершил работу.\n";
    return 0;
}