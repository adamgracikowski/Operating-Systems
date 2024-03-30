#define _GNU_SOURCE
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define UNUSED(x) ((void)(x))
#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MIN_N 2
#define MAX_N 5
#define MIN_M 5
#define MAX_M 10
#define RESIGN_PROBABILITY 5
#define PLAYING 0
#define RESIGNED 1
#define INVALID_CARD -1
#define PERM 0666
#define SHM_NAME "/sop-cards"
#define SHM_SIZE(n, type) (sizeof(shm_data_t) + (n) * sizeof(type))

typedef struct player_data_t
{
    int card;
    int score;
    int active;
} player_data_t;

typedef struct shm_data_t
{
    pthread_barrier_t begin_barrier;
    pthread_barrier_t game_barrier;
    int n;
    player_data_t player_data[];
} shm_data_t;

void usage(char *name);
void parse_argv(int argc, char **argv, int *n, int *m);
int should_resign(int probability);
int draw_card(int *cards, int *last);
void create_cards(int *cards, int m);
void sop_barrierattr_shared(pthread_barrierattr_t *attr);
void sop_barrier_reload(pthread_barrier_t *barrier, pthread_barrierattr_t *attr, int n);
void player_work(shm_data_t *shm_ptr, int player_idx, int m);
void game_server_work(shm_data_t *shm_ptr, int m, pthread_barrierattr_t *attr);
void summarise(shm_data_t *shm_ptr, int n);
void find_winners(shm_data_t *shm_ptr, int n, int *winning_card, int *winning_count);

int main(int argc, char **argv)
{

    int n, m;
    parse_argv(argc, argv, &n, &m);
    fprintf(stderr, "n = %d, m = %d\n", n, m);

    srand((unsigned)time(NULL) * getpid());

    shm_unlink(SHM_NAME);

    int shm_fd;
    if ((shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, PERM)) < 0)
        ERR("shm_open");
    if (ftruncate(shm_fd, SHM_SIZE(n, player_data_t)) < 0)
        ERR("ftruncate");

    shm_data_t *shm_ptr;
    if ((shm_ptr = mmap(NULL, SHM_SIZE(n, player_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        ERR("mmap");

    memset(shm_ptr, 0, SHM_SIZE(n, player_data_t));
    pthread_barrierattr_t attr;
    sop_barrierattr_shared(&attr);
    if (pthread_barrier_init(&shm_ptr->game_barrier, &attr, n + 1))
        ERR("pthread_barrier_init");
    if (pthread_barrier_init(&shm_ptr->begin_barrier, &attr, n + 1))
        ERR("pthread_barrier_init");
    shm_ptr->n = n;

    for (int i = 0; i < n; i++)
    {
        switch (fork())
        {
        case -1:
            ERR("fork");
        case 0: // player
            player_work(shm_ptr, i, m);
            exit(EXIT_SUCCESS);
        }
    }

    game_server_work(shm_ptr, m, &attr);

    while (wait(NULL) > 0)
        ;

    if (pthread_barrier_destroy(&shm_ptr->game_barrier))
        ERR("pthread_barrier_destroy");
    if (pthread_barrier_destroy(&shm_ptr->begin_barrier))
        ERR("pthread_barrier_destroy");
    if (munmap(shm_ptr, SHM_SIZE(n, player_data_t)) < 0)
        ERR("munmap");
    if (shm_unlink(SHM_NAME) < 0)
        ERR("shm_unlink");

    return EXIT_SUCCESS;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s %d <= N <= %d, %d <= M <= %d\n", name, MIN_N, MAX_N, MIN_M, MAX_M);
    exit(EXIT_FAILURE);
}

void parse_argv(int argc, char **argv, int *n, int *m)
{
    if (argc != 3)
        usage(argv[0]);
    *n = atoi(argv[1]);
    if (*n < MIN_N || *n > MAX_N)
        usage(argv[0]);
    *m = atoi(argv[2]);
    if (*m < MIN_M || *m > MAX_M)
        usage(argv[0]);
}

int should_resign(int probability)
{
    return rand() % probability == 0;
}

int draw_card(int *cards, int *last)
{
    if (*last < 0)
        ERR("draw_card");

    int card_idx = rand() % (*last + 1);
    int drawn = cards[card_idx];
    for (int i = card_idx; i < *last; i++)
    {
        cards[i] = cards[i + 1];
    }
    cards[(*last)--] = drawn;
    return drawn;
}

void create_cards(int *cards, int m)
{
    for (int i = 0; i < m; i++)
        cards[i] = i + 1;
}

void sop_barrierattr_shared(pthread_barrierattr_t *attr)
{
    if (pthread_barrierattr_init(attr))
        ERR("pthread_barrierattr_init");
    if (pthread_barrierattr_setpshared(attr, PTHREAD_PROCESS_SHARED))
        ERR("pthread_barrierattr_setpshared");
}

void sop_barrier_reload(pthread_barrier_t *barrier, pthread_barrierattr_t *attr, int n)
{
    if (pthread_barrier_destroy(barrier))
        ERR("pthread_barrier_destroy");
    if (pthread_barrier_init(barrier, attr, n))
        ERR("pthread_barrier_init");
}

void player_work(shm_data_t *shm_ptr, int player_idx, int m)
{
    pid_t pid = getpid();
    srand((unsigned)time(NULL) * pid);
    int drawn, last = m - 1, last_score = 0;

    fprintf(stdout, "Player [%d]: Joined the game!\n", player_idx);

    int cards[m];
    create_cards(cards, m);

    for (int i = 1; i <= m; i++)
    {
        drawn = draw_card(cards, &last);
        shm_ptr->player_data[player_idx].card = drawn;
        fprintf(stdout, "Player [%d]: Drawing card %d in round %d!\n", player_idx, drawn, i);

        pthread_barrier_wait(&shm_ptr->begin_barrier);
        // server finds the winner and sends points
        pthread_barrier_wait(&shm_ptr->game_barrier);

        fprintf(stdout, "Player [%d]: Scoring %d in round %d!\n", player_idx, shm_ptr->player_data[player_idx].score - last_score, i);
        last_score = shm_ptr->player_data[player_idx].score;
        shm_ptr->player_data[player_idx].active = should_resign(RESIGN_PROBABILITY);

        pthread_barrier_wait(&shm_ptr->game_barrier);
        // server counts active players
        pthread_barrier_wait(&shm_ptr->game_barrier);

        if (shm_ptr->player_data[player_idx].active == RESIGNED)
        {
            fprintf(stdout, "Player [%d]: Leaving the game in round %d!\n", player_idx, i);
            break;
        }
    }
    if (munmap(shm_ptr, SHM_SIZE(shm_ptr->n, player_data_t)) < 0)
        ERR("munmap");
}

void summarise(shm_data_t *shm_ptr, int n)
{
    fprintf(stdout, "Server: It's time to summarise the game!\n");
    for (int i = 0; i < n; i++)
    {
        fprintf(stdout, "\tPlayer %d: %d points\n", i, shm_ptr->player_data[i].score);
    }
}

void find_winners(shm_data_t *shm_ptr, int n, int *winning_card, int *winning_count)
{
    for (int j = 0; j < n; j++)
    {
        if (shm_ptr->player_data[j].active == RESIGNED)
            continue;

        fprintf(stdout, "Server: Player %d has drawn %d!\n", j, shm_ptr->player_data[j].card);

        if (shm_ptr->player_data[j].card > *winning_card)
        {
            *winning_card = shm_ptr->player_data[j].card;
            *winning_count = 1;
        }
        else if (shm_ptr->player_data[j].card == *winning_card)
        {
            (*winning_count)++;
        }
    }
}

void game_server_work(shm_data_t *shm_ptr, int m, pthread_barrierattr_t *attr)
{
    int n = shm_ptr->n;
    int winning_card, winning_count, winning_score, active;

    fprintf(stdout, "Server: The game has started!\n");
    for (int i = 1; i <= m; i++)
    {
        active = 0;
        winning_card = -1;
        winning_count = 0;
        fprintf(stdout, "Server: Round %d!\n", i);

        pthread_barrier_wait(&shm_ptr->begin_barrier);

        find_winners(shm_ptr, n, &winning_card, &winning_count);
        fprintf(stdout, "Server: %d is the winning_card!\n", winning_card);
        fprintf(stdout, "Server: There are %d winners this round!\n", winning_count);
        winning_score = n / winning_count;

        for (int j = 0; j < n; j++)
            shm_ptr->player_data[j].score +=
                (shm_ptr->player_data[j].card == winning_card) ? winning_score : 0;

        pthread_barrier_wait(&shm_ptr->game_barrier);
        // here players resign or not
        pthread_barrier_wait(&shm_ptr->game_barrier);

        for (int j = 0; j < n; j++)
            if (shm_ptr->player_data[j].active == PLAYING)
                active++;

        if (active == 0)
        {
            pthread_barrier_wait(&shm_ptr->game_barrier);
            break;
        }

        sop_barrier_reload(&shm_ptr->begin_barrier, attr, active + 1);
        pthread_barrier_wait(&shm_ptr->game_barrier);
        sop_barrier_reload(&shm_ptr->game_barrier, attr, active + 1);

        fprintf(stdout, "Server: There are %d players left.\n", active);
    }
    summarise(shm_ptr, n);
}