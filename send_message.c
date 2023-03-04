#include <fcntl.h> // For O_* constants.
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
    const int shm_size = 6;

    printf("The page size is: %lu\n", sysconf(_SC_PAGE_SIZE));

    // Create a shared memory fragment of size shm_size.
    int fd_memory = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ASSERT_SYS_OK(fd_memory);
    ASSERT_SYS_OK(ftruncate(fd_memory, shm_size)); // Extend, filling with null bytes.

    // Map the memory into the process' address space.
    // This will be preserved after fork.
    // We could also shm_open and mmap in the child, see send_message_no_inheritance.c.
    // (Note however that then the shared memory won't necessarily be mapped to the same address!).
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_SHARED;
    // flags = MAP_PRIVATE;
    char* mapped_mem = mmap(NULL, shm_size, prot, flags, fd_memory, 0);

    // We don't need the descriptor or special file anymore.
    ASSERT_SYS_OK(close(fd_memory));
    ASSERT_SYS_OK(shm_unlink(shm_name)); // Deleted the special file; what if we comment this out?

    if (mapped_mem == MAP_FAILED)
        syserr("mmap failed.");

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
