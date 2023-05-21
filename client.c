#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "util.h"

#define MAX_MSG_SIZ 0x100
#define EXIT_MSG "exit\n"

enum status {
    EXITED,
    RUNNING
};

int receiver_status;

void usage(void)
{
    puts("syntax : echo-client <ip> <port>");
    puts("sample : echo-client 192.168.10.2 1234");
}

int connect_to_server(char *ip, int port)
{
    struct sockaddr_in serv_addr;
    int sk;

    sk = socket(AF_INET, SOCK_STREAM, 0);
    if (sk < 0) {
        pr_err("socket\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    if (connect(sk, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        pr_err("connect (%s:%d)\n", ip, port);
        close(sk);
        return -1;
    }

    return sk;    
}

void *receiver(void *arg)
{
    char msg[MAX_MSG_SIZ + 1];
    ssize_t n;
    int sk = *(int *)arg;

    receiver_status = RUNNING;

    while (1) {
        n = recv(sk, msg, MAX_MSG_SIZ, 0);
        if (n <= 0) {
            pr_err("Error while receiving messages from server");
            break;
        }
        msg[n] = '\0';
        
        pr_info("server message: %s\n", msg);
    }

    receiver_status = EXITED;

    return NULL;
}

int client_main_loop(int sk)
{
    char msg[MAX_MSG_SIZ + 1];
    pthread_t th;

    if (pthread_create(&th, NULL, receiver, &sk) < 0) {
        pr_err("Cannot create thread\n");
        return -1;
    }

    while (receiver_status == EXITED)
        /* simple spinlock */;
    
    while (receiver_status == RUNNING) {
        fgets(msg, MAX_MSG_SIZ, stdin);
        
        if (send(sk, msg, strlen(msg), 0) < 0) {
            pr_err("Error while sending message\n");
            return -1;
        }

        if (!strcmp(msg, EXIT_MSG))
            break;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    char *ip;
    int port;
    int sk;
    int error;
    
    if (argc != 3) {
        usage();
        return 0;
    }

    ip = argv[1];
    port = atoi(argv[2]);
    sk = connect_to_server(ip, port);
    if (sk < 0) {
        pr_err("Cannot connect to server\n");
        return -1;
    }

    error = client_main_loop(sk);
    close(sk);

    if (error < 0) {
        pr_err("client exited with error code: %d\n", error);
        return -1;
    }

    return 0;
}