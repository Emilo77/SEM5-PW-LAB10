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
    // (This makes sure the child opens the memory only after it is resized).
    int fd_memory = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ASSERT_SYS_OK(fd_memory);
    ASSERT_SYS_OK(ftruncate(fd_memory, shm_size)); // Extend, filling with null bytes.
    ASSERT_SYS_OK(close(fd_memory));

    sleep(nap_time_seconds);

    pid_t pid = fork();
    ASSERT_SYS_OK(pid);
    if (!pid) {
        // Child.
        fd_memory = shm_open(shm_name, O_RDWR, S_IRUSR | S_IWUSR);
        ASSERT_SYS_OK(fd_memory);
        char* mapped_mem = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_memory, 0);
        if (mapped_mem == MAP_FAILED)
            syserr("mmap");
        ASSERT_SYS_OK(close(fd_memory));
        // We can't call shm_unlink(shm_name) here, because parent might try to re-open it later.

        sleep(nap_time_seconds);
        print_table(mapped_mem, shm_size);
        sleep(2 * nap_time_seconds);
        print_table(mapped_mem, shm_size);
        ASSERT_SYS_OK(munmap(mapped_mem, shm_size)); // Not needed, will disappear with process.
        return 0;
    }

    // Parent.

    // Make an additional dummy memory mapping, just to show that the shared memory won't always
    // be mapped to the same address.
    (void)mmap(NULL, 1, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    fd_memory = shm_open(shm_name, O_RDWR, S_IRUSR | S_IWUSR);
    ASSERT_SYS_OK(fd_memory);
    char* mapped_mem = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_memory, 0);
    if (mapped_mem == MAP_FAILED)
        syserr("mmap");
    close(fd_memory);

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

    // Unlink the special file now that we're sure nobody will try to open it.
    // What happens if we don't do that?
    ASSERT_SYS_OK(shm_unlink(shm_name));

    return 0;
}
