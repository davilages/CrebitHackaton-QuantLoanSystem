#include <iostream>
#include <pqxx/pqxx>

int main() {
    try {
        // Connect to the database
        // Default user is 'postgres', you may need to set a password or use peer authentication
        pqxx::connection C("dbname=postgres user=postgres password=sua_nova_senha host=127.0.0.1 port=5432");
        
        std::cout << "Connected to database " << C.dbname() << std::endl;

        // Start a transaction (or use a simple nontransaction)
        pqxx::nontransaction T(C);

        // Execute a query
        pqxx::result R = T.exec("SELECT datname FROM pg_database WHERE datistemplate = false;");

        // Iterate over the results
        for (pqxx::row row : R) {
            std::cout << "Database: " << row[0].as<std::string>() << std::endl;
        }

        // Commit the transaction (nontransaction doesn't need commit)
        // T.commit();

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
