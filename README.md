# ESP32_BLE_TASK

Cel: Przygotować aplikacje na ESP32 korzystając z ESP-IDF, która będzie umożliwiała:

  - Rozgłaszanie ESP32 jako dostępne urządzenie BLE,
  - Nawiązanie połączenia do ESP32 przez zewnętrzne urządzenie BLE,
  - Uruchomienie usług BLE umożliwiających komunikację typu serial,
  - Zdefiniowanie tablicy z wartościami liczbowymi (tablica 2000 elementów, z których każdy może mieć 2 byte’ową wartość),
  - Odbieranie komunikatu wskazującego na indeks tablicy i odsyłanie do zewnętrznego urządzenia BLE komunikatu z wartością tablicy znajdującą się pod wskazanym indeksem,
  - Podczas odsyłania tekstowego komunikatu należy zastosować mechanizm notyfikacji, aby urządzenie docelowe zostało poinformowane o nowym komunikacie do odczytu,
  - Utworzenie nowego wątku, który będzie dynamicznie (co 1 sekundę) zmieniał wartość przypisaną do jednego, losowo wybranego indeksu w zdefiniowanej tablicy i wyświetlał na wyjściu Serial informację o nowo         ustawionej wartości.

Platforma testowa: 
- ESP32-WROOM-32E
- Espressif-IDE Version: 3.0.0
- Windows 10
- nRF Connect - aplikacja
- Android 14
