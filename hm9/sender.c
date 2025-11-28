#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

volatile sig_atomic_t receiver_pid = 0;

volatile uint32_t number = 0;
volatile sig_atomic_t bit_index = 0;
volatile sig_atomic_t waiting_ack = 0;
volatile sig_atomic_t sending = 1;

void send_next_bit() {
    if (bit_index >= 32) {
        kill(receiver_pid, SIGINT);
        sending = 0;
        return;
    }

    int bit;
    bit = (number >> bit_index) & 1;

    if (bit == 0) {
        kill(receiver_pid, SIGUSR1);
    } else {
        kill(receiver_pid, SIGUSR2);
    }

    waiting_ack = 1;
}

void handle_ack(int sigh) {
    waiting_ack = 0;
    bit_index++;
    send_next_bit();
}

int main() {
    printf("Sender PID: %d\n", getpid());
    printf("Enter receiver PID: ");
    scanf("%d", &receiver_pid);

    printf("Enter integer to send: ");
    int32_t input;
    scanf("%d", &input);

    number = (uint32_t)input;

    struct sigaction sa = {0};
    sa.sa_handler = handle_ack;
    sigaction(SIGUSR1, &sa, NULL);

    printf("Async sending...\n");

    send_next_bit();

    while (sending) {
        pause();
    }

    printf("Done (async).\n");
    return 0;
}
