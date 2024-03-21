# Pracownicy:

Stwórz system dystrybucji zadań dla procesów pracowników z wykorzystaniem kolejek wiadomości POSIX. System będzie symulować pracę przez dodawanie dwóch losowych liczb zmiennoprzecinkowych z przedziału $(0.0,100.0)$ oraz uśpienie procesu na losowy czas ($500$ms - $2000$ms).
Główny proces (serwer) co losowy czas ($[T1,T2]$ ms, gdzie $100 \leq T1 < T2 \leq 5000$) dodaje nowe zadanie do kolejki. Każde zadanie to para losowych liczb zmiennoprzecinkowych z przedziału $(0.0,100.0)$.
Na początku tworzone są procesy dzieci ($N$ pracowników, gdzie $2 <= N <= 20$), które rejestrują się w kolejce zadań o nazwie `rask_queue_[server_pid]`.
Pracownicy, oczekując na zadanie, pobierają je z kolejki, gdy są dostępne i gdy nie są zajęci. Każdy pracownik zwraca wyniki przez swoją własną kolejkę o nazwie `result_queue_[server_pid]_[employee_pid]`.

Uwaga: Program powienien obsługiwać wiele instancji bez kolizji nazw kolejek!

## Etapy:
1. Główny proces tworzy $N$ pracowników, którzy po wykonaniu losowego uśpienia ($500% - $2000$ ms) kończą pracę. Na starcie, serwer wypisuje `Server is starting...` a po zakończeniu pracy wszystkich pracowników: `Server: All child processes have finished.`. Pracownicy przy starcie wypisują: `[employee_pid]: Employee ready!` oraz `[employee_pid]: Exits!` przy zakończeniu.
2. Serwer tworzy $5N$ zadań, każde w losowych odstępach czasu ($T1$ do $T2$ ms), i dodaje je do kolejki. Serwer informuje o dodaniu zadania: `Server: New task queued: [v1, v2]` lub o pełnej kolejce: `Server: Queue is full!`. Pracownicy przy otrzymaniu zadania informują: `[employee_pid]: Received task [v1, v2]`, śpią losowo ($500$-$2000$ ms), wypisują wynik: `[employee_pid]: Result result_value` i oczekują na kolejne zadanie. Kończą pracę po wykonaniu $5$ zadań.
3. Pracownicy wysyłają wyniki do serwera przez indywidualne kolejki i informują o tym na standardowym wyjściu: `[employee_pid]: Result sent result_value`. Serwer odbiera wyniki i wyświetla: `Server: Result from employee employee_pid: result_value`.
4. Proces główny kontynuuje tworzenie zadań, aż do otrzymania sygnału `SIGINT`, po czym informuje pracowników o zakończeniu pracy (poprzez kolejkę). Serwer czeka na zakończenie aktualnych zadań pracowników, a następnie kończy działanie. Pracownicy kończą pracę po otrzymaniu informacji o zakończeniu od serwera (doliczają do końca jedynie rozpoczęte zadania - pozostałe zadania z kolejki są ignorowane). Wszystkie zasoby są poprawnie zwalniane.
