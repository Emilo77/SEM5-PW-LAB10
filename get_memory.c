#include <fcntl.h> // For O_* constants.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> // For mode constants.
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "err.h"

void list_shared_memory_objects(void)
{
    printf("Obecnie w /dev/shm znajdują się:\n");
    int ret = system("ls /dev/shm"); // Wykonaj komendę w powłoce /bin/sh.
    ASSERT_SYS_OK(ret);
    if (ret != 0)
        fatal("ls failed with return code: %d", ret);
    printf("\n\n");
}

int main(void)
{
    const int nap_time_seconds = 2;
    const char* shm_name = "/mimuw_pw_lab_test_memory";

    list_shared_memory_objects();

    // Tworzymy i otwieramy fragment pamięci dzielonej.
    int fd_memory = shm_open(shm_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ASSERT_SYS_OK(fd_memory);
    printf("Stworzyłem fragment pamięci, jeśli nie istniał.\n");
    list_shared_memory_objects();

    sleep(nap_time_seconds);

    // Zamykamy deskryptor, ale plik specjalny dalej istnieje.
    ASSERT_SYS_OK(close(fd_memory));
    printf("Zamknąłem deskryptor z dostępem do pamięci.\n");
    list_shared_memory_objects();

    sleep(nap_time_seconds);

    // Usuwamy plik specjalny.
    ASSERT_SYS_OK(shm_unlink(shm_name));
    printf("Usunąłem fragment pamięci.\n");
    list_shared_memory_objects();

    return 0;
}
