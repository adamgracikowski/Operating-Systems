#define _GNU_SOURCE
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define UNUSED(x) ((void)(x))
#define NEXT_DOUBLE (((double)rand()) / RAND_MAX)
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MIN_N 1
#define MAX_N 29
#define MIN_ITERATIONS 1e2
#define MAX_ITERATIONS 1e6
#define ESTIMATION_ENTRY_LENGTH sizeof(float)
#define LOG_ENTRY_LENGTH 8
#define LOG_PATH "./montecarlo_log.txt"

void usage(char *name);
void parse_argv(int argc, char **argv, int *n, int *m);
void estimation_worker(int index, int m, float *data_mapping, char *log_mapping);
void create_estimation_workers(int n, int m, float *data_mapping, char *log_mapping);
void parent_work(int n, float *data_mapping);

int main(int argc, char **argv)
{
    int n, m;
    parse_argv(argc, argv, &n, &m);

    int log_fd;
    if ((log_fd = open(LOG_PATH, O_CREAT | O_RDWR | O_TRUNC, -1)) < 0)
        ERR("open");
    if (ftruncate(log_fd, n * LOG_ENTRY_LENGTH))
        ERR("ftruncate");

    size_t log_mapping_length = n * LOG_ENTRY_LENGTH;
    char *log_mapping;
    if ((log_mapping = (char *)mmap(NULL, log_mapping_length, PROT_WRITE | PROT_READ, MAP_SHARED, log_fd, 0)) == MAP_FAILED)
        ERR("mmap");
    if (close(log_fd))
        ERR("close");

    size_t data_mapping_length = n * ESTIMATION_ENTRY_LENGTH;
    float *data_mapping;
    if ((data_mapping = mmap(NULL, data_mapping_length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED)
        ERR("mmap");

    create_estimation_workers(n, m, data_mapping, log_mapping);
    parent_work(n, data_mapping);

    if (munmap(data_mapping, data_mapping_length) < 0)
        ERR("munmap");
    if (msync(log_mapping, log_mapping_length, MS_SYNC) < 0)
        ERR("msync");
    if (munmap(log_mapping, log_mapping_length) < 0)
        ERR("munmap");
    return EXIT_SUCCESS;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n m\n", name);
    fprintf(stderr, "%d <= n <= %d - number of child processes\n", (int)MIN_N, (int)MAX_N);
    fprintf(stderr, "%d <= m <= %d - number of monte carlo iterations\n", (int)MIN_ITERATIONS, (int)MAX_ITERATIONS);
    exit(EXIT_FAILURE);
}

void parse_argv(int argc, char **argv, int *n, int *m)
{
    if (argc != 3)
        usage(argv[0]);
    *n = atoi(argv[1]);
    *m = atoi(argv[2]);
    if (*n < MIN_N || *n > MAX_N || *m < MIN_ITERATIONS || *m > MAX_ITERATIONS)
        usage(argv[0]);
}

void estimation_worker(int index, int m, float *data_mapping, char *log_mapping)
{
    srand((unsigned)time(NULL) * getpid());
    int counter = 0;
    for (int i = 0; i < m; i++)
    {
        double x = NEXT_DOUBLE, y = NEXT_DOUBLE;
        if (x * x + y * y <= 1.0)
            counter++;
    }
    data_mapping[index] = (float)counter / m;

    char log_buffer[LOG_ENTRY_LENGTH + 1];
    if (snprintf(log_buffer, LOG_ENTRY_LENGTH + 1, "%7.5f\n", data_mapping[index] * 4.0F) < 0)
        ERR("snprintf");
    memcpy(log_mapping + index * LOG_ENTRY_LENGTH, log_buffer, LOG_ENTRY_LENGTH);
}

void create_estimation_workers(int n, int m, float *data_mapping, char *log_mapping)
{
    for (int i = 0; i < n; i++)
    {
        switch (fork())
        {
        case -1:
            ERR("fork");
        case 0:
            estimation_worker(i, m, data_mapping, log_mapping);
            exit(EXIT_SUCCESS);
        }
    }
}

void parent_work(int n, float *data_mapping)
{
    float pi = 0.0F;

    errno = 0;
    while (wait(NULL) > 0)
        ;
    for (int i = 0; i < n; i++)
        pi += data_mapping[i];
    pi /= n;
    pi *= 4;

    printf("PI is approximately %f\n", pi);
}
