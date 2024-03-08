# Cykl:

Symulacja cyklicznego transferu danych pomiędzy procesami przy pomocy łączy nienazwanych (pipe). Program przyjmuje jeden parametr $N \geq 2$, oznaczający liczbę procesów biorących udział w transferze danych (liczbę wierzchołków cyklu). Domyślna wartość parametru $N$ to 3 (procesy tworzą wówczas trójkąt).

## Zasady:

Program używa łączy pipe do jednostronnej komunikacji pomiędzy $N$ procesami. Procesy tworzą cykl, w którym każdy wierzchołek (proces) ma dostęp do jednego końca łącza do odczytu i jednego końca łącza do zapisu. Kierunek łączy musi być dobrany tak, aby możliwe było przesyłanie komunikatów “w kółko” pomiędzy procesami.

Dodatkowo:
- Proces rodzic rozpoczyna transmisję wysyłając liczbę 3 (jako tekst o zmiennej długości) w obieg.
- W późniejszym etapie działania programu każdy proces pracuje identycznie, czyli odbiera liczbę, wypisuje ją na konsoli wraz ze swoim PID-em, czeka 100ms, modyfikuje liczbę poprzez dodanie losowej liczby całkowitej z przedziału $[-10, 15]$ i wysyła ją dalej.
- Transmisja kończy się w momencie, gdy odebrana przez proces liczba jest większa niż 999. Wówczas proces, który odebrał taką liczbę ma się zakończyć, a inne procesy poprzez detekcję zerwanego łącza także mają się zakończyć.
- Należy zadbać, aby wszystkie łącza zostały poprawnie zamknięte. Informację o zamykanym w danej chwili łączu należy wyświetlić w konsoli.
- Użytkownik w losowym momencie może zarządać przerwania transmisji poprzez wysłanie sygnału SIGINT. Zasady przerywania transmisji są w tym przypadku takie same jak w przypadku otrzymania liczby większe niż 999.
