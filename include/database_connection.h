/**
 * @file database_connection.h
 * @brief Lekki wrapper C API dla MariaDB (Wątkowo-bezpieczny).
 * @version 1.2.0
 */

#ifndef DATABASE_CONNECTION_H
#define DATABASE_CONNECTION_H

#include <mariadb/mysql.h>

class DatabaseConnection {
private:
    MYSQL* conn;

public:
    DatabaseConnection();
    ~DatabaseConnection();

    // Łączy z bazą na podstawie przekazanych parametrów
    bool connect(const char* host, const char* user, const char* password, 
                 const char* database, unsigned int port = 3306);
    
    // Wykonuje zapytanie SQL
    MYSQL_RES* query(const char* sql);
    
    // Zamyka połączenie
    void close();

    // Zwraca surowy wskaźnik MYSQL (potrzebny np. do mysql_real_escape_string)
    MYSQL* get_native_handle() { return conn; }
};

#endif // DATABASE_CONNECTION_H

