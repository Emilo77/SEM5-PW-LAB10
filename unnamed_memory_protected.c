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
    const int nap_time_seconds = 2;

    const int buffer_size = 6;

    printf("The page size is: %lu\n", sysconf(_SC_PAGE_SIZE));

    // We will create enough memory to fit our buffer and a semaphore (this will fit on one page anyway).
    const size_t shmem_size = sizeof(sem_t) + buffer_size;

    // Create a new anonymous memory fragment and map it into the process' address space.
    void* mapped_mem_all = mmap(NULL, shmem_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mapped_mem_all == MAP_FAILED)
        syserr("mmap");

    // Split the memory into the semaphore and the shared buffer.
    // The buffer starts right after the semaphore.
    sem_t* sem = (sem_t*)mapped_mem_all;
    char* shared_buffer = (char*)(mapped_mem_all + sizeof(sem_t));

    print_table(shared_buffer, buffer_size);

    ASSERT_SYS_OK(sem_init(sem, 1, 0));

    pid_t pid = fork();
    ASSERT_SYS_OK(pid);
    if (!pid) {
        // Child.
        ASSERT_SYS_OK(sem_wait(sem));
        print_table(shared_buffer, buffer_size);
        return 0;
    }

    // Parent.
    printf("Pid %d: child's pid: %d\n", getpid(), pid);

    // Test whether the child waits for us to post on the semaphore.
    sleep(nap_time_seconds);

    // Copy a c-string into the shared memory.
    const char* message = "Ala ma kota";
    strncpy(shared_buffer, message, buffer_size);
    shared_buffer[buffer_size - 1] = '\0'; // Ensure the memory contains a null-terminated string.
    printf("Pid %d: I modified the memory contents.\n", getpid());
    print_table(shared_buffer, buffer_size);

    ASSERT_SYS_OK(sem_post(sem));

    ASSERT_SYS_OK(wait(NULL));
    ASSERT_SYS_OK(sem_destroy(sem));

    return 0;
}
