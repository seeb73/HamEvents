# 📡 hamevent - aktualizacja 0.3.1
Serwer danych UDP.

Aktualizacja projektu `HamEvents`:
- Log główny (`/var/log/hamevents/hamevents.log`)
- Log błędnych danych (`/var/log/hamevents/hamevents.err`)
- Log spływających danych (`/var/log/hamevents/hamevents.raw`)
- Konfiguracja przez plik `/etc/hamevents/hamevents.conf`

## Nowe parametry w hamevents.conf
- `logfile`: Ścieżka do pliku głównego logu
- `wronglog`: 'true/false` - czy logować błędne dane
- `wronglogfile`: Ścieżka do pliku błędnych danych

## Instrukcja użycia
1. Utwórz kopię pliku hamevents.conf.example do /etc/hamevents/hamevents.conf.
2. Wpisz właściwe dane do 'hamevensts.conf'.

## Budowanie
```bash
make clean
make
./hamevents 
```

---
