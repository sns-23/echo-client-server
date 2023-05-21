#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "util.h"
#include "list.h"

#define MAX_CLIENT_CNT 10
#define MAX_MSG_SIZ 0x100
#define EXIT_MSG "exit\n"

struct client {
    struct list_head list;
    pthread_t th;
    struct in_addr ip;
    in_port_t port;
    int sk;
};

struct server {
    int sk;
    int port;
};

bool opt_e;
bool opt_b;
LIST_HEAD(client_list_head);
pthread_mutex_t client_list_lock;

void usage(void)
{
    puts("Usage: echo-server <port> [-e[-b]]");
}

void parse_opt(int argc, char *argv[])
{
    int opt;

    while (true) {
        opt = getopt(argc, argv, "eb");
        if (opt == 'e')
            opt_e = true;
        else if (opt == 'b')
            opt_b = true;
        else if (opt == EOF)
            break;
        else
            /* ignore */;      
    }
}

int broadcast_msg(struct client *client, char *msg, ssize_t n)
{
    struct client *cur;
    
    pthread_mutex_lock(&client_list_lock);
    list_for_each_entry(cur, &client_list_head, list) {
        send(cur->sk, msg, n, 0);
    }
    pthread_mutex_unlock(&client_list_lock);

    return 0;
}

int echo_msg(struct client *client, char *msg, ssize_t n)
{
    return send(client->sk, msg, n, 0);
}

void *handle_client_msg(void *arg)
{
    struct client *client;
    char msg[MAX_MSG_SIZ + 1];
    ssize_t n;

    client = (struct client *)arg;

    pr_info("client(%s:%d) is connected\n", inet_ntoa(client->ip), client->port);

    while (true) {
        n = recv(client->sk, msg, MAX_MSG_SIZ, 0);
        if (n <= 0)
            break;

        msg[n] = '\0';

        pr_info("client(%s:%d) message: %s\n", inet_ntoa(client->ip), client->port, msg);
        
        /* TODO: handle error that occur during sending process */
        if (opt_b) {
            broadcast_msg(client, msg, n);
        }

        if (!opt_b && opt_e) {
            echo_msg(client, msg, n);
        }

        if (!strcmp(msg, EXIT_MSG))
            break;
    }

    /* TODO: make delete_client() and replace following codes */
    pthread_mutex_lock(&client_list_lock);
    list_del(&client->list);
    pthread_mutex_unlock(&client_list_lock);

    close(client->sk);
    free(client);

    pr_info("client(%s:%d) is disconnected\n", inet_ntoa(client->ip), client->port);

    return NULL;
}

int server_main_loop(struct server *server)
{
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_len;
    struct client *new_client;

    pthread_mutex_init(&client_list_lock, NULL);
    
    while (true) {
        pthread_mutex_lock(&client_list_lock);
        if (list_count_nodes(&client_list_head) >= MAX_CLIENT_CNT) {
            pthread_mutex_unlock(&client_list_lock);
            continue;
        }
        pthread_mutex_unlock(&client_list_lock);

        /* TODO: make init_client() and replace following codes */
        new_client = malloc(sizeof(*new_client));
        if (new_client == NULL) {
            pr_warn("There is no memory\n");
            continue;
        }        

        new_client->sk = accept(server->sk, (struct sockaddr *)&clnt_addr, &clnt_addr_len);
        if (new_client->sk < 0) {
            pr_warn("Error while accepting client requests\n");
            free(new_client);
            continue;
        }

        new_client->ip = clnt_addr.sin_addr;
        new_client->port = clnt_addr.sin_port;

        if (pthread_create(&new_client->th, NULL, handle_client_msg, new_client) < 0) {
            pr_warn("Fail to create thread\n");
            close(new_client->sk);
            free(new_client);
            continue;
        }
        pthread_mutex_lock(&client_list_lock);
        list_add_tail(&new_client->list, &client_list_head);
        pthread_mutex_unlock(&client_list_lock);
    }

    pthread_mutex_destroy(&client_list_lock);

    return 0;
}

void delete_server(struct server *server)
{
    if (server->sk >= 0)
        close(server->sk);
    free(server);
}

int create_server_sk(int port)
{
    int sk;
    struct sockaddr_in srv_addr;

    const int flag = true;

    sk = socket(AF_INET, SOCK_STREAM, 0);
    if (sk == -1) {
        pr_err("socket\n");
        goto out_error;
    }
        
    if (setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
        pr_err("setsockopt\n");
        goto out_error;
    }

    memset(&srv_addr, 0, sizeof(struct sockaddr_in));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons(port);

    if (bind(sk, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1) {
        pr_err("bind\n");
        goto out_error;
    }
        
    if (listen(sk, MAX_CLIENT_CNT) == -1) {
        pr_err("listen\n");
        goto out_error;
    }   

    return sk;

out_error:
    if (sk >= 0)
        close(sk);
    return -1;
}

struct server *init_server(int argc, char *argv[])
{
    struct server *server;

    if (argc < 2){
        usage();
        return NULL;
    }

    server = malloc(sizeof(*server));
    if (server == NULL)
        return NULL;

    server->port = atoi(argv[1]);
    if (server->port == 0)
        goto out_error;

    server->sk = create_server_sk(server->port);
    if (server->sk < 0) 
        goto out_error;

    parse_opt(argc, argv);

    return server;

out_error:
    delete_server(server);
    return NULL;
}

int main(int argc, char *argv[])
{
    struct server *server;
    int error;

    server = init_server(argc, argv);
    if (server == NULL) {
        pr_err("Cannot initialize server\n");
        return 0;
    }

    error = server_main_loop(server);
    if (error < 0)
        pr_err("server exited with error code: %d\n", error);

    delete_server(server);
        
    return 0;
}