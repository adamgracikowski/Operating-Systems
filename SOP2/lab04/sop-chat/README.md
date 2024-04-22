# Chat Grupowy:

Celem zadania jest napisanie serwera czatu zgodnego z dostarczonym klientem. Wszystkie wiadomości wysyłane między klientem a serwerem mają zawsze długość $512$ bajtów. Pierwsze $64$ bajty przeznaczone są na nazwę klienta (użytkownika), reszta bajtów przeznaczona jest na tekst wiadomości, lub w przypadku autoryzacji na klucz. Program klienta dostarczony jest w postaci pliku wykonywalnego i jego kodu źródłowego w przypadku potrzeby zbudowania samemu. Dostarczony jest też plik wykonywalny.

## Etapy:
1. Zaimplementuj przyłączanie pojedynczego klienta do serwera TCP. Serwer przyjmuje jako arugumenty wejściowe numer portu i klucz autoryzacji. Przykładowe wykonanie serwera: `./sop-chat 9000 key`. Po uruchomieniu serwer nasłuchuje na przyłączających się klientów. Gdy ustanowione zostanie podłączenie z pierwszym klientem, serwer je zamyka, a następnie się kończy
2. Zaimplementuj mechanizm autoryzacji klienta. Po przyłączeniu się klienta serwer czyta wiaddomość autoryzacji, wypisuje na konsolę nazwę użytkownika i dostarczony klucz. Jeżeli klucz klienta jest zgodny z kluczem serwera, to odsyła otrzymaną wiadomość, w pętli czyta $5$ kolejnych wiadomości klienta i wypisuje je na konsolę. Po otrzymaniu $5$ wiadomości połączenie jest zamykane. Jeżeli
klucze nie są zgodne, to połączenie jest natychmiast zamykane.
3. Zaimplementuj obsługę wielu klientów. Wiadomości klientów czytane są od teraz bez ograniczeń ilościowych. Po przyłączeniu się nowego klienta zostaje on dodany do listy aktualnych klientów. Maksymalna liczba klientów to $4$. Jeśli przyłączy się maksymalna liczba klientów, należy odrzucać
nowe połączenia. Kiedy serwer otrzyma wiadomość od któregoś klienta, wyświetla je na konsoli i rozsyła do pozostałych klientów. Do implementacji tego etapu użyj funkcji `epoll` (lub alternatywnie `pselect`, lub `ppoll`).
4. Po otrzymaniu sygnału `SIGINT`, serwer zamyka wszystkie połączenia, zwalnia zasoby i kończy pracę. Dodaj poprawną obsługę rozłączania klientów w następujących przypadkach:
   - Czytanie z deskryptora danego klienta zwraca wiadomość długości $0$.
   - Pisanie do deskryptora danego klienta zgłasza błąd `EPIPE`.
   - Pisanie lub czytanie zgłasza błąd `ECONNRESET`.
   - Serwer otrzymał `C-c`.

## Autor zadania:
Autorem i pomysłodawcą zadania jest [Tomasz Herman](https://github.com/tomasz-herman).
