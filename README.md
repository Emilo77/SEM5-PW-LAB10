# Laboratorium - współbieżność w systemie operacyjnym: pamięć współdzielona i semafory

### Dołączone pliki
Do laboratorium zostały dołączone pliki:
- [CMakeLists.txt](CMakeLists.txt)
- [err.c](err.c)
- [err.h](err.h)
- [get_memory.c](get_memory.c)
- [gifts.c](gifts.c)
- [send_message.c](send_message.c)
- [send_message_no_inheritance.c](send_message_no_inheritance.c)
- [send_message_protected.c](send_message_protected.c)
- [unnamed_memory.c](unnamed_memory.c)
- [unnamed_memory_protected.c](unnamed_memory_protected.c)

## Pamięć współdzielona

Pamięć współdzielona w systemie Linux jest identyfikowana poprzez nazwę. Nazwa jest to napis nie dłuższy niż `NAME_MAX` znaków, postaci `/somename` gdzie `somename` to napis nie zawierający znaku `/`. (Wartość `NAME_MAX` dla danego systemu plików można sprawdzić za pomocą polecenia `getconf NAME_MAX /`, zazwyczaj jest to 255.)

Podobieństwo nazwy `SHM` do ścieżki dostępu do pliku nie jest przypadkowe. Wiele narzędzi i mechanizmów w systemie Linux korzysta z dostosowanego do własnych potrzeb interfejsu związanego z obsługą plików. Fragmenty pamięci współdzielonej, jako obiekty nazwane, są przechowywane w wirtualnym systemie plików i traktowane przez system operacyjny w podobny sposób.

W związku z tym, to do użytkownika często należy zapewnienie unikalności nazwy w tzw. *obiektach nazwanych*.

###Uzyskanie dostępu/stworzenie nowego fragmentu pamięci współdzielonej

By uzyskać dostęp do fragmentu pamięci współdzielonej należy wywołać funkcję [shm_open](http://man7.org/linux/man-pages/man3/shm_open.3.html), będącą analogiem funkcji funkcji [open](http://man7.org/linux/man-pages/man2/open.2.html) służącej do otrzymania deskryptora pliku.

```c
int shm_open(const char* name, int oflag, mode_t mode);
```

gdzie

- `name` to nazwa fragmentu pamięci,
- `mode` to prawa dostępu do pamięci (wykorzystywane, jeśli wywołanie `shm_open` stworzy nowy fragment pamięci); przeczytaj [man 2 open](http://man7.org/linux/man-pages/man2/open.2.html), część `O_CREAT` by poznać możliwe wartości,
- a `oflag` to bitowa unia jednej z dwu flag
  - `O_RDONLY` - tylko do odczytu,
  - lub `O_RDWR` - odczyt i zapis;
    oraz pewnej liczby poniższych flag
  - `O_CREAT` - stwórz fragment `SHM`, jeśli nie istnieje;
  - `O_TRUNC` - jeśli fragment istnieje to zmień jego rozmiar do `0`;
  - `O_EXCL` - jeśli `O_CREAT` jest zdefiniowane i fragment istnieje zwróć błąd.

W przypadku niepowodzenia `shm_open` zwraca wartość `-1` i ustawia odpowiednią wartość zmiennej `errno`. W razie powodzenia, funkcja zwraca deskryptor pozwalający na dostęp do pamięci o nazwie name. Jeśli flaga `O_CREAT` jest zdefiniowana i fragment pamięci o nazwie `name` nie istnieje to zostanie on utworzony i w systemie plików zostanie utworzony plik specjalny reprezentujący ten fragment (zazwyczaj umieszczony w `/dev/shm/`). Otrzymany deskryptor jest klonowany podczas funkcji `fork` i automatycznie zamykany podczas operacji `exec`.

Jeśli wywołanie `shm_open` stworzyło nowy fragment pamięci, ma on prawa dostępu ustawione zgodnie z `mode` i początkową wielkość `0`. Stworzony fragment pamięci będzie istniał dopóki jego nazwa będzie istniała w systemie plików i jakiś proces będzie miał dowiązanie do tego fragmentu pamięci (otwarty deskryptor, aktywne mapowanie).

Aby zmienić wielkość `SHM` należy wywołać funkcję [ftruncate](http://man7.org/linux/man-pages/man2/ftruncate.2.html) podając deskryptor `fd` oraz nową wartość wielkości pamięci `length`:

```c
int ftruncate(int fd, off_t length);
```

W przypadku błędu funkcja zwraca `-1` i ustawia odpowiednią wartość `errno`. W przypadku sukcesu, wartość pamięci jest ustawiana na `length`. Jeśli nowa wielkość jest mniejsza niż poprzednia, nadmiarowe dane zostaną utracone.

Zobacz przykład [get_memory.c](get_memory.c).

## Przyłączenie pamięci współdzielonej

By skorzystać z fragmentu pamięci dzielonej należy go przyłączyć do przestrzeni adresowej procesu. Można to uczynić wywołując funkcję [mmap](http://man7.org/linux/man-pages/man2/mmap.2.html), która przyłącza fragment pliku opisanego deskryptorem `fd` pod adres `addr`. Przyłączony fragment zaczyna się w pozycji `offset` i ma długość `length`. By zrozumieć jak dokładnie działa funkcja `mmap` należy wiedzieć, że pamięć wykorzystywana przez procesy, a także przez systemy plików jest podzielona na strony, każda strona ma taki sam, ustalony rozmiar, który można otrzymać wywołując `sysconf(_SC_PAGE_SIZE)`. Funkcja `mmap` pozwala mapować część pliku do pamięci procesu, ta część pliku to pewna liczba kolejnych stron pamięci, znajdująca się w przedziale `[offset, offset+length]`. Oznacza to, że `mmap` nie dołącza pojedynczych bitów ale całe strony.

Funkcja `mmap` jest zadeklarowana jako
```c
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
```
gdzie

- `addr` to sugestia skierowana do systemu operacyjnego miejsca w pamięci procesu, w którym chcemy przyłączyć fragment pliku, `addr = NULL` oznacza brak sugestii;
- **Uwaga!**: adres ten może zostać zignorowany, i dla przenośności programu powinna zostać podana wartość `NULL`,
- `length` to długość przyłączanego fragmentu (w bajtach)
- `prot` to pole opisujące prawa dostępu do przyłączonych stron pliku, może mieć wartość `PROT_NONE` (brak dostępu do stron) lub być unią pewnej liczby poniższych flag
  - `PROT_EXEC` strony mogą być wykonywane,
  - `PROT_READ` strony mogą być czytane,
  - `PROT_WRITE` strony mogą być zapisywane.
- `flags` to dodatkowe flagi definiujące parametry przyłączenia;
- `fd` to deskryptor pliku, którego fragment chcemy dołączyć
- `offset` to miejsce w pliku, początek dołączanego fragmentu; ta wartość musi być wielokrotnością rozmiaru strony.

Pole `flags` to bitowa unia jedej z dwu flag

- `MAP_SHARED` - współdziel przyłączenie: wszelkie zmiany dokonane w tym obszarze są propagowane do pliku wskazanego przez `fd`, oraz do innych miejsc mapujących ten obszar
- `MAP_PRIVATE` - stwórz prywatną kopię: zmiany dokonane w tym obszarze nie są propagowane do pliku wskazanego przez `fd`, ani do innych miejsc mapujących ten obszar;

oraz pewnej ilości innych flag, o których można przeczytać w [dokumentacji](http://man7.org/linux/man-pages/man2/mmap.2.html), np. flaga `MAP_ANONYMOUS`, która powoduje zignorowanie pola `fd` i stworzenie mapowania bez użycia zewnętrznego pliku.

W przypadku błędu, funkcja `mmap` zwraca `MAP_FAILED` i ustawia odpowiednią wartość `errno`. W przypadku sukcesu, funkcja zwraca adres (wskaźnik typu `(void *)`), w którym faktycznie został przyłączony fragment pliku. Z tej pamięci możemy teraz korzystać tak jak z pamięci lokalnej procesu, np. jak z tablicy.

Jak wynika z opisu funkcja `mmap` pozwala na przyłączenie nie tylko fragmentów pamięci współdzielonej, ale także innych struktur, do których dostęp jest obsługiwany przez deskryptory i działanie funkcji `mmap` jest zdefiniowane. W przypadku `SHM` zwyczajowo wywołujemy mmap z parametrami

```c
mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
```

Po przyłączeniu `SHM` do procesu możemy korzystać z tej pamięci w ten sam sposób co z pamięci lokalnej procesu, a system zadba by wszelkie zmiany były propagowane do innych użytkowników tego fragmentu pamięci, zgodnie z ustawionymi flagami.

## Kończenie pracy z pamięcią współdzieloną

Kiedy proces już nie chce korzystać z pamięci współdzielonej musi odłączyć ją z przestrzeni adresowej i zamknąć związany z nią deskryptor. Deskryptor zamykamy znanym nam poleceniem `void close(int fd)`, natomiast odłączenie jest dokonywane za pomocą funkcji
```c
int munmap(void* addr, size_t length);
```
gdzie

- `addr` to adres w pamięci procesu, będący wielokrotnością rozmiaru strony, od którego zaczyna się pamięć, którą chcemy odłączyć,
- `length` to rozmiar pamięci, którą chcemy odłączyć.

W przypadku błędu funkcja zwraca `-1` i ustawia odpowiednią wartość `errno`. W przypadku sukcesu, funkcja zwraca `0`, i odłącza z przestrzeni adresy znajdujące się na stronach zawierających adresy z przedziału `[addr, addr + len]`.

**Uwaga!** Samo zamknięcie deskryptora nie powoduje odłączenie `SHM`. Z podłączonej pamięci można korzystać nawet po zamknięciu deskryptora. Co więcej, usunięcie wszystkich przyłączonych fragmentów nie musi zwolnić zasobów systemowych związanych z tym fragmentem pamięci, plik specjalny ciągle może istnieć.

By usunąć plik specjalny związany z fragmentem pamięci dzielonej o nazwie `name` należy wywołać funkcję 
```c
int shm_unlink(const char* name);
```

Powyższa funkcja powoduje usunięcie pliku specjalnego z systemu plików, ale nie samego fragmentu pamięci współdzielonej. Nawet po usunięciu pliku fragment pamięci będzie istniał w systemie, dopóki jakiś proces będzie z niego korzystał, czyli będzie miał deskryptor związany z tym fragmentem lub cześć adresów procesu będzie mapowana na tę pamięć dzieloną. Co więcej, deskryptory i przyłączone fragmenty pozostaną w pełni funkcjonalne.

Przykład [send_message.c](send_message.c) prezentuje prostą komunikację z wykorzystaniem pamięci dzielonej. Rodzic otwiera pamięć dzieloną, następnie tworzy dziecko i wpisuje do fragmentu jedno zdanie. Dziecko ma je odczytać. Synchronizacja jest wymuszona usypianiem procesów `sleep`, by działały w rozłącznych przedziałach czasu.

## Przykład: Anonimowa pamięć współdzielona

Linux umożliwia na korzystanie z współdzielenie pamięci między procesami, bez jawnego użycia struktur SHM. Wynika to z faktu, że podczas operacji `fork`, przyłączone przestrzenie adresów są kopiowane, a niezbędne struktury do obsługi mapowania znajdują się w systemie operacyjnym. Pokazuje to przykład [unnamed_memory.c](unnamed_memory.c).

Program pozwala także zaprezentować pewne własności implementacji funkcji `mmap`: zmiana zdefiniowanych stałych może zmienić zachowanie programu.

## Semafory

Pamięć współdzielona, jak sama nazwa wskazuje jest przeznaczona do pracy z wieloma procesami. Oznacza to, że dostęp od niej powinien być synchronizowany. Do synchronizacji można wykorzystać np. semafory. 

Semafory zgodne z `POSIX`, udostępniają dwie podstawowe operacje `sem_post`, odpowiadającą operacji `V`, oraz `sem_wait`, odpowiadającą operacji `P` i są udostępniane w dwu wersjach: *nazwanej* - z powiązanym plikiem specjalnym - i *nienazwanej* - umieszczone jedynie w pamięci programu. Obie wersje semaforów, charakteryzują się podobną funkcjonalnością synchronizacji. Różnią się jedynie sposobem pozyskania, inicjalizacji i zwalniania.

## Pozyskanie semaforów nazwanych

Tak jak fragmenty pamięci dzielonej, nazwane semafory mają nazwę postaci `/somename`, nie dłuższą niż `NAME_MAX-4`, i związany plik specjalny `/dev/shm/sem.somename`.

By uzyskać dostęp do semafora należy wywołać funkcję [sem_open](http://man7.org/linux/man-pages/man3/sem_open.3.html)
```c
sem_t* sem_open(const char* name, int oflag, mode_t mode, unsigned int value);
```
gdzie
- `name` to nazwa semafora,
- `oflag` to flagi, modyfikujące działanie funkcji,
- `mode` to uprawnienia, używane przy tworzeniu semafora,
- `value` to początkowa wartość semafora.

W przypadku błędu funkcja zwraca `SEM_FAILED` i ustawia odpowiednią wartość `errno`. W przypadku sukcesu, funkcja zwraca adres nowego semafora.

**Uwaga!**: Jeśli przed wykonaniem `sem_open` semafor istniał to `mode` i `value` są ignorowane, a referencja, którą otrzymujemy odnosi się do już istniejącego i (obecnie) współdzielonego semafora.

Po zakończeniu korzystania z semafora, należy zwolnić zasoby pobrane przy pozyskaniu semafora. By tego dokonać wywołujemy funkcję [sem_close](http://man7.org/linux/man-pages/man3/sem_close.3.html)
```c
int sem_close(sem_t* sem);
```
która pozwala, by zasoby systemowe użyte do obsługi semafora przez proces zostały zwolnione. Funkcja ta nie powoduje usunięcia powiązanego pliku specjalnego, aby tego dokonać należy wywołać funkcję [sem_unlink](http://man7.org/linux/man-pages/man3/sem_unlink.3.html)
```c
int sem_unlink(const char* name);
```
gdzie name to nazwa semafora. W przypadku błędu funkcja zwraca `-1` i ustawia odpowiednią wartość `errno`. W przypadku sukcesu, funkcja zwraca `0`.

## Pozyskanie semaforów nienazwanych

Nienazwane semafory nie mają nazwy, a tym samym powiązanego pliku specjalnego. Z tego powodu muszą być umieszczone w pamięci procesu. By skorzystać z nienazwanych semaforów należy zarezerwować fragment pamięci (np. poprzez zadeklarowanie zmiennej typu `sem_t`) w miejscu dostępnym dla wszystkich procesów (wątków), które mają z niego korzystać.

Przed użyciem semafora nienazwanego należy go zainicjalizować przy pomocy funkcji [sem_init](http://man7.org/linux/man-pages/man3/sem_init.3.html)
```c
int sem_init(sem_t* sem, int pshared, unsigned int value);
```
gdzie

- `sem` to adres semafora,
- `value` to początkowa wartość semafora,
- `pshared` to informacja czy semafor będzie współdzielony przez wątki czy przez procesy.

Jeśli `pshared = 0` to semafor będzie współdzielony przez wątki procesu i może zostać w lokalnej przestrzeni procesu, wpp. semafor będzie współdzielony przez procesy i powinien był być umieszczony pamięci dostępnej dla wszystkich procesów, np. w we fragmencie pamięci współdzielonej.
W przypadku błędu funkcja zwraca `-1` i ustawia odpowiednią wartość `errno`. W przypadku sukcesu, funkcja zwraca `0`.

Po zakończeniu pracy z semaforem należy go zwolnić poprzez wywołanie funkcji [sem_destroy](http://man7.org/linux/man-pages/man3/sem_destroy.3.html)
```c
int sem_destroy(sem_t* sem);
```
gdzie `sem` to adres semafora. W przypadku błędu funkcja zwraca `-1` i ustawia odpowiednią wartość `errno`. W przypadku sukcesu, funkcja zwraca `0`.

**Uwaga!**: Powyższa funkcja niszczy struktury systemowe związane z semaforem, zatem nie należy jej wywoływać, jeśli jakiś proces (wątek) korzysta z tego semafora.

## Operacje na semaforach

Gdy mamy dostęp do istniejącego i zainicjalizowanego semafora możemy wykonać standardowe operacje semaforowe.

Funkcja [sem_wait](http://man7.org/linux/man-pages/man3/sem_wait.3.html)
```c
int sem_wait(sem_t* sem);
```
obniża wartość semafora sem o wartość `1`, ale tak by nie spadła poniżej `0`. Jeśli można zmniejszyć wartość semafora, funkcja wraca natychmiast. Jeśli nie może obniżyć tej wartości, proces (wątek) zostaje wstrzymany do momentu, kiedy będzie mógł obniżyć wartość lub nastąpi przerwanie związane z obsługą sygnału. W przypadku błędu funkcja zwraca `-1` i ustawia odpowiednią wartość `errno`. W przypadku sukcesu, funkcja zwraca `0`.

Funkcja [sem_post](http://man7.org/linux/man-pages/man3/sem_post.3.html)
```c
int sem_post(sem_t* sem);
```
zwiększa wartość semafora `sem` w wartość `1`. Jeśli wartość staje się większa niż `0` i istnieje wstrzymany proces (wątek) to zostanie obudzony. W przypadku błędu funkcja zwraca `-1` i ustawia odpowiednią wartość `errno`. W przypadku sukcesu, funkcja zwraca `0`.

## Przykład: Semafory nazwane

Przykład [send_message_protected.c](send_message_protected.c) prezentuje prostą synchronizację dostępu do pamięci współdzielonej. Rodzic: uzyskuje dostęp do `SHM` i tworzy semafor, pełniący funkcję synchronizacyjną. Następnie, tworzy dziecko, zapisuje wiadomość do fragmentu pamięci i kończy działanie.

Dziecko: dziedziczy dostęp do fragmentu pamięci współdzielonej, uzyskuje dostęp do semafora, a następnie próbuje odczytać wiadomość od rodzica.

Przykład [unnamed_memory_protected.c](unnamed_memory_protected.c) jest implementacją z wykorzystaniem nienazwanego wariantu semaforów.

## Zadanie: prezenty

Okres przedświąteczny jest zawsze pracowity: szczególnie gdy termin dostarczenia zadania zbliża się, a Ty jesteś elfem w fabryce prezentów.

Pomóż elfom pracującym w fabryce Mikołaja przygotować prezenty na święta.

Prezenty w fabryce produkuje `N_ELFS` elfów, a do Mikołaja dostarcza je `N_REINDEERS` reniferów. Każdy elf produkuje `LOOPS` prezentów, funkcja `int produce()` tworzy jeden prezent. Po wytworzeniu prezentu, każdy elf odnosi go do magazynu i zaczyna produkcję kolejnego prezentu. Magazyn jest buforem cyklicznym o rozmiarze `BUFF_SIZE`. Prezenty z magazynu do Mikołaja dostarczają renifery. Każdy renifer odbiera jeden prezent z magazynu i dostarcza go prosto do Mikołaja procedurą `void deliver(int gift)`.

Uzupełnij (w miejscach oznaczonych `TODO`) kod w pliku [gifts.c](gifts.c) tak, by elfy i renifery mogły pracować bezpiecznie. Wykorzystaj pamięć współdzieloną i semafory do obsługi magazynu. Zadbaj o współbieżność i nie zapomnij zwolnić zasobów systemowych po zakończeniu zadania. 
