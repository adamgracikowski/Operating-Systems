#include "table.h"

#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include "macros.h"

table_t *table_init(int *player_idx)
{
    sem_t *sem_ptr;
    if ((sem_ptr = sem_open(SHMEM_SEMAPHORE_NAME, O_CREAT, 0666, 1)) == SEM_FAILED)
        ERR("sem_open");

    sem_wait(sem_ptr);

    table_t *table_ptr;

    errno = 0;
    int shm_fd = shm_open(SHMEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd >= 0)
    {
        if ((table_ptr = mmap(NULL, sizeof(table_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        {
            sem_post(sem_ptr);
            ERR("mmap");
        }
        if (ftruncate(shm_fd, sizeof(table_t)))
        {
            shm_unlink(SHMEM_NAME);
            sem_post(sem_ptr);
            ERR("mmap");
        }

        close(shm_fd);

        errno = 0;
        pthread_barrierattr_t barrier_attr;
        pthread_barrierattr_init(&barrier_attr);
        pthread_barrierattr_setpshared(&barrier_attr, PTHREAD_PROCESS_SHARED);
        pthread_barrier_init(&table_ptr->players_barrier, &barrier_attr, PLAYERS_COUNT);

        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&table_ptr->players_counter_lock, &mutex_attr);
        pthread_mutex_init(&table_ptr->placed_lock, &mutex_attr);

        pthread_condattr_t cond_attr;
        pthread_condattr_init(&cond_attr);
        pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&table_ptr->placed_cond, &cond_attr);

        for (int i = 0; i < CARDS_COUNT; ++i)
            table_ptr->cards[i] = i;

        for (int i = 0; i < PLAYERS_COUNT; ++i)
            table_ptr->trick[i] = INVALID_CARD;

        table_ptr->players_counter = 1;
        *player_idx = 0;

        table_ptr->players_counter = 1;
        *player_idx = 0;
        for (int i = 0; i < 10; ++i)
            shuffle(table_ptr->cards, CARDS_COUNT);

        if (errno)
        {
            shm_unlink(SHMEM_NAME);
            sem_post(sem_ptr);
            ERR("shmem init");
        }

        for (int i = 0; i < PLAYERS_COUNT - 1; i++)
            sem_post(sem_ptr);
    }
    else if (errno == EEXIST)
    {
        if ((shm_fd = shm_open(SHMEM_NAME, O_RDWR, 0666)) < 0) 
            ERR("shm_open");
        if ((table_ptr = mmap(NULL, sizeof(table_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
            ERR("mmap");

        close(shm_fd);

        pthread_mutex_lock(&table_ptr->players_counter_lock);
        *player_idx = table_ptr->players_counter++;
        pthread_mutex_unlock(&table_ptr->players_counter_lock);
    }
    else
    {
        sem_post(sem_ptr);
        ERR("shm_open");
    }

    printf("[%d]: Waiting for players!\n", getpid());
    pthread_barrier_wait(&table_ptr->players_barrier);

    return table_ptr;
}

void table_close(table_t *shmem)
{
    munmap(shmem, sizeof(table_t));
}

void table_destroy(table_t *shmem)
{
    pthread_cond_destroy(&shmem->placed_cond);
    pthread_mutex_destroy(&shmem->players_counter_lock);
    pthread_mutex_destroy(&shmem->placed_lock);
    pthread_barrier_destroy(&shmem->players_barrier);

    table_close(shmem);

    if (shm_unlink(SHMEM_NAME))
        ERR("shm_unlink");
    if (sem_unlink(SHMEM_SEMAPHORE_NAME))
        ERR("sem_unlink");
}
