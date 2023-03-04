#include <fcntl.h> // For O_* constants.
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> // For mode constants.
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "err.h"

void print_table(const char* table, int len)
{
    printf("Pid %d: table is at address %p, contains:\n", getpid(), table);
    for (int i = 0; i < len; ++i)
        printf("|%c", table[i]);
    printf("|\n\n");
}

int main(void)
{
    const int nap_time_seconds = 1;
    const char* shm_name = "/mimuw_pw_lab_test_memory";
    const char* sem_name = "/mimuw_pw_lab_test_sem";
    const int shm_size = 6;

    printf("The page size is: %lu\n", sysconf(_SC_PAGE_SIZE));

    // Create and map shared memory.
    int fd_memory = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ASSERT_SYS_OK(fd_memory);
    ASSERT_SYS_OK(ftruncate(fd_memory, shm_size));
    char* mapped_mem = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_memory, 0);
    ASSERT_SYS_OK(close(fd_memory));
    ASSERT_SYS_OK(shm_unlink(shm_name));
    if (mapped_mem == MAP_FAILED)
        syserr("mmap");

    // We could create and initialize the semaphore here, and make sure the name isn't already
    // used (O_EXCL). But we don't have to, it doesn't matter who creates the semaphore first.
    // {
    //     sem_t* sem = sem_open(sem_name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR, 0);
    //     if (sem == SEM_FAILED)
    //         syserr("parent sem_open");
    //     ASSERT_SYS_OK(sem_close(sem));
    // }

    pid_t pid = fork();
    ASSERT_SYS_OK(pid);
    if (!pid) {
        // Child.
        // Create the semaphore with inital value 0, or use the existing one if already created.
        sem_t* sem = sem_open(sem_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
        // If we're sure the semaphore already exists, we could just call:
        // sem = sem_open(sem_name, O_RDWR);
        if (sem == SEM_FAILED)
            syserr("child sem_open");
        ASSERT_SYS_OK(sem_wait(sem));

        print_table(mapped_mem, shm_size);
        return 0;
    }

    // Parent.
    printf("Pid %d: child's pid: %d\n", getpid(), pid);

    // Test whether the child waits for us to post on the semaphore.
    sleep(nap_time_seconds);

    // Copy a c-string into the shared memory.
    const char* message = "Ala ma kota";
    strncpy(mapped_mem, message, shm_size);
    mapped_mem[shm_size - 1] = '\0'; // Ensure the memory contains a null-terminated string.
    printf("Pid %d: I modified the memory contents.\n", getpid());

    // Create the semaphore with inital value 0, or use the existing one if already created.
    sem_t* sem = sem_open(sem_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    // If we're sure the semaphore already exists, we could just call:
    // sem = sem_open(sem_name, O_RDWR);
    if (sem == SEM_FAILED)
        syserr("parent sem_open 2");
    ASSERT_SYS_OK(sem_post(sem));

    ASSERT_SYS_OK(wait(NULL));
    ASSERT_SYS_OK(sem_close(sem));
    ASSERT_SYS_OK(sem_unlink(sem_name));

    return 0;
}
