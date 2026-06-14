# 📡 hamevent - aktualizacja

Aktualizacja projektu `hamevent`:
- Log główny (`/var/log/hamevent.log`)
- Log błędnych danych (`/var/log/hamevent.err`)
- Konfiguracja przez plik `config.ini`

## Nowe parametry w config.ini
- `logfile`: Ścieżka do pliku głównego logu
- `wronglog`: `y` lub `n` - czy logować błędne dane
- `wronglogfile`: Ścieżka do pliku błędnych danych

## Instrukcja użycia
1. Podmień pliki `config_manager.h`, `config_manager.cpp`, `logger.h`, `logger.cpp`.
2. Dodaj `config.ini` do katalogu projektu.
3. Dodaj obsługę błędnych danych w `qso_processor.cpp`:
   - Jeśli parsowanie danych zwraca pusty wynik, zapisz do `wronglogfile`.

## Budowanie
```bash
make clean
make
```

---