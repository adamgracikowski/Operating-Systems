# Klient-Serwer:

Napisz dwa programy: klienta i serwera, komunikujące się przy użyciu kolejek POSIX.

## Zasady:

Serwer tworzy 3 kolejki nazwane `PID_s`, `PID_d` i `PID_m`, gdzie `PID` to identyfikator procesu i wypisuje ich nazwy.
Serwer czeka na następujące dane z dowolnej kolejki: `PID` klienta oraz dwie liczby całkowite. Serwer przetwarza zamówienia klienta w następujący sposób: oblicza sumę przesłanych liczb całkowitych dla kolejki `PID_s`, iloraz dla `PID_d` oraz resztę z dzielenia dla `PID_m`. Wynik jest zapisywany do kolejki klienta (patrz niżej opis klienta). 
Po otrzymaniu sygnału `SIGINT`, serwer usuwa swoje kolejki i kończy działanie.

Program klienta jest wywoływany z jednym parametrem - nazwą kolejki serwera (jedną z trzech). Klient tworzy swoją własną kolejkę nazwaną swoim PID-em. Następnie, aż do odczytania `EOF`, odczytuje linie zawierające dwie liczby ze standardowego wejścia. Po odczytaniu linii klient wysyła wiadomość składającą się ze swojego numeru `PID` oraz odczytanych dwóch liczb do kolejki serwera i oczekuje na odpowiedź w swojej kolejce. Po otrzymaniu odpowiedzi wypisuje ją. Jeśli nie otrzyma odpowiedzi w ciągu 1 sekundy, kończy swoje działanie. Przed zakończeniem swojego działania, klient usuwa utworzoną przez siebie kolejkę.

## Etapy:
1. Serwer tworzy swoje kolejki i wypisuje ich nazwy. Po upływie sekundy usuwa je i kończy działanie. Klient tworzy swoją kolejkę, czeka 1 sekundę, usuwa ją i kończy działanie.
2. Serwer odczytuje pierwszą wiadomość z kolejki `PID_s`, po czym odsyła odpowiedź do klienta. Na tym etapie program ignoruje wszystkie błędy. Klient odczytuje 2 liczby ze standardowego wejścia i wysyła wiadomość do serwera. Czeka na odpowiedź i wypisuje ją.
3. Serwer obsługuje wszystkie kolejki. Kończy działanie po otrzymaniu `SIGINT`. Klient wysyła swoje wiadomości do momentu odczytania EOF albo przekroczenia czasu oczekiwania na odpowiedź.
4. Kolejki są usuwane przy zamykaniu programów. Pełna obsługa błędów.

## Szczegóły implementacji:

Czekanie na wiadomość z dowolnej kolejki przez serwer zostało zrealizowane poprzez utworzenie dedykowanych procesów dla każdej z obsługiwanych przez serwer kolejek.
