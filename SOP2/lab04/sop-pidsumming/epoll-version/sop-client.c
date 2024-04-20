#include "sop-socket.h"

void usage(char *pname);

int main(int argc, char **argv)
{
    if (argc != 3)
        usage(argv[0]);

    char* host = argv[1];
    char* port = argv[2];

    pid_t pid = getpid();
    int16_t answer;
    char buffer[PID_LENGTH];
    if (snprintf(buffer, PID_LENGTH, "%d", pid) < 0)
        ERR("snprintf");
    printf("[%d]: PID = %d\n", getpid(), pid);

    int client_socket = sop_connect_sockstream(host, port);

    if (sop_bulk_write(client_socket, buffer, PID_LENGTH) < 0)
        ERR("write:");
    if (sop_bulk_read(client_socket, (char *)&answer, sizeof(int16_t)) < (int)sizeof(int16_t))
        ERR("read:");
    
    printf("[%d]: SUM = %d\n", getpid(), ntohs(answer));

    if (TEMP_FAILURE_RETRY(close(client_socket)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}

void usage(char *pname)
{
    printf("USAGE: %s host port\n", pname);
    exit(EXIT_FAILURE);
}