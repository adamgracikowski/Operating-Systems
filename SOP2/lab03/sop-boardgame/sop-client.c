#include "boardgame-utils.h"

void usage(char *name);
void parse_argv(int argc, char **argv, int *n, pid_t *server_pid);

int main(int argc, char *argv[])
{
    int n;
    pid_t server_pid;
    parse_argv(argc, argv, &n, &server_pid);

    pid_t pid = getpid();
    srand((unsigned)time(NULL) * pid);

    char shm_name[SHM_NAME_MAX];
    create_shm_name(shm_name, SHM_NAME_MAX, server_pid);

    int shm_fd;
    if ((shm_fd = shm_open(shm_name, O_RDWR, PERM)) == -1)
        ERR("shm_open");

    shm_data_t *shm_ptr;
    if ((shm_ptr = mmap(NULL, SHM_SIZE(n), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        ERR("mmap");

    sop_shm_subscribe(shm_ptr);

    int score = 0;
    while (1)
    {
        sop_mutex_sharedlock(&shm_ptr->board_mutex);
        if (rand() % 9 == 0)
        {
            printf("Client [%d]: Ops...\n", pid);
            sop_mutex_sharedunlock(&shm_ptr->board_mutex);
            break;
        }

        int x = rand() % n,
            y = rand() % n;
        printf("Client [%d]: Trying to search field (%d, %d)\n", pid, x, y);

        int field_value = shm_ptr->board[n * y + x];
        if (field_value == 0)
        {
            printf("Client [%d]: GAME OVER! My score is %d\n", pid, score);
            sop_mutex_sharedunlock(&shm_ptr->board_mutex);
            break;
        }
        else
        {
            printf("Client [%d]: Found %d points\n", pid, field_value);
            score += field_value;
            shm_ptr->board[n * y + x] = 0;
        }

        sop_mutex_sharedunlock(&shm_ptr->board_mutex);

        timespec_t ts = {.tv_sec = 1, .tv_nsec = 0};
        nanosleep(&ts, &ts);
    }

    sop_shm_unsubscribe(shm_ptr);

    if (munmap(shm_ptr, SHM_SIZE(n)) < 0)
        ERR("munmap");

    return EXIT_SUCCESS;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n server_pid\n", name);
    fprintf(stderr, "%d <= n <= %d - board size\n", (int)MIN_N, (int)MAX_N);
    exit(EXIT_FAILURE);
}

void parse_argv(int argc, char **argv, int *n, pid_t *server_pid)
{
    if (argc != 3)
        usage(argv[0]);
    *n = atoi(argv[1]);
    if (*n < MIN_N || *n > MAX_N)
        usage(argv[0]);
    *server_pid = atoi(argv[2]);
    if (*server_pid <= 0)
        usage(argv[0]);
}