#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

volatile sig_atomic_t sender_pid = 0;
volatile sig_atomic_t bit_count = 0;
volatile uint32_t buffer = 0;
volatile sig_atomic_t receiving = 1;

void handle_bit0(int sig, siginfo_t *info, void *ucontext) {
    if (!sender_pid) {
        sender_pid = info->si_pid;
    }
    buffer |= (0u << bit_count);
    bit_count++;
    kill(sender_pid, SIGUSR1);
}

void handle_bit1(int sig, siginfo_t *info, void *ucontext) {
    if (!sender_pid) {
        sender_pid = info->si_pid;
    }
    buffer |= (1u << bit_count);
    bit_count++;
    kill(sender_pid, SIGUSR1);
}

void handle_end(int sig) {
    receiving = 0;
}

int main() {
    printf("Receiver PID: %d\n", getpid());

    struct sigaction sa0 = {0}, sa1 = {0}, sa_end={0};
    sa0.sa_sigaction = handle_bit0;
    sa1.sa_sigaction = handle_bit1;
    sa_end.sa_handler = handle_end;

    sa0.sa_flags = SA_SIGINFO;
    sa1.sa_flags = SA_SIGINFO;

    sigaction(SIGUSR1, &sa0, NULL);   // bit=0
    sigaction(SIGUSR2, &sa1, NULL);   // bit=1
    sigaction(SIGINT,  &sa_end, NULL);

    printf("Enter sender PID (optional, only to show): ");
    scanf("%d", &sender_pid);

    printf("Receiver ready (async)...\n");

    while (receiving)
        pause();

    int32_t result = (int32_t)buffer;
    printf("Received number: %d\n", result);
    return 0;
}
