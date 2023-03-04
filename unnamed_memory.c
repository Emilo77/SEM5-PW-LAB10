#include <fcntl.h> // For O_* constants/
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

    const int shm_size = 6;

    printf("The page size is: %lu\n", sysconf(_SC_PAGE_SIZE));

    // Create a new anonymous memory fragment and map it into the process' address space.
    // This will be preserved after fork.
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_SHARED | MAP_ANONYMOUS;
    // flags = MAP_PRIVATE | MAP_ANONYMOUS;
    int fd_memory = -1; // There is no descriptor, no special file, processes can only inherit this memory mapping.
    char* mapped_mem = mmap(NULL, shm_size, prot, flags, fd_memory, 0);
    if (mapped_mem == MAP_FAILED)
        syserr("mmap failed");

    // Now we test the memory exactly as before.

    print_table(mapped_mem, shm_size);

    sleep(nap_time_seconds);

    pid_t pid = fork();
    ASSERT_SYS_OK(pid);
    if (!pid) {
        // Child.
        sleep(nap_time_seconds);
        print_table(mapped_mem, shm_size);
        sleep(2 * nap_time_seconds);
        print_table(mapped_mem, shm_size);
        ASSERT_SYS_OK(munmap(mapped_mem, shm_size)); // Not needed, will disappear with process.
        return 0;
    }

    // Parent.
    printf("Pid %d: child's pid: %d\n", getpid(), pid);
    print_table(mapped_mem, shm_size);
    sleep(2 * nap_time_seconds);

    // Copy a c-string into the shared memory.
    const char* message = "Ala ma kota";
    strncpy(mapped_mem, message, shm_size);
    mapped_mem[shm_size - 1] = '\0'; // Ensure the memory contains a null-terminated string.
    printf("Pid %d: I modified the memory contents.\n", getpid());
    print_table(mapped_mem, shm_size);

    ASSERT_SYS_OK(wait(NULL));
    ASSERT_SYS_OK(munmap(mapped_mem, shm_size)); // Not needed, will disappear with process.

    return 0;
}
