/*
 * A rudimentary HTTP server with a unique feature: accessing /crash causes this
 * server to segfault.
 */
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * Cause the program to terminate via segfault.
 */
static void crash(void)
{
    volatile int *x = (volatile int *)0;
    *x = 0x12345678;
}

/*
 * Generates a simple HTTP response header and sends it along with the body.
 */
static void send_html_response(int fd, const char *body)
{
    char header[1024];
    snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=utf-8\r\n"
        "Content-Length: %zd\r\n"
        "\r\n",
        strlen(body));
    send(fd, header, strlen(header), 0);
    send(fd, body, strlen(body), 0);
}

/*
 * Handle a single connection.
 */
static void handle_connection(int fd)
{
    char buffer[1024] = {0};

    ssize_t r = read(fd, buffer, sizeof(buffer));
    if (r < 0) {
        close(fd);
        return;
    }

    const char *index_req = "GET / HTTP/1.1\r\n";
    const char *crash_req = "GET /crash HTTP/1.1\r\n";

    if (!memcmp(buffer, index_req, strlen(index_req))) {
        send_html_response(fd, "<html><body><h1>It Works!</h1></body></html>");
    } else if (!memcmp(buffer, crash_req, strlen(crash_req))) {
        send_html_response(fd, "<html><body><h1>Crashing!</h1></body></html>");
        crash();
    }

    close(fd);
}

/*
 * Starts the main listen/accept loop, processes connections sequentially. Pass
 * port in as first CLI argument.
 */
int main(int argc, char const *argv[])
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int port = 80;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        int conn_fd = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t*)&addrlen);
        if (conn_fd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        handle_connection(conn_fd);
    }

    return 0;
}
