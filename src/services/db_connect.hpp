#pragma once

#include <fmt/core.h>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string>

class DbConnection {
 private:
  std::string _connection_str;
  //   pqxx::connection cx;
  //   pqxx::work tx;
 public:
  DbConnection() {
    // REFERENCE CONN STR: postgresql://john:secret@localhost:5432/mydatabase[?sslmode=...]

    this->_connection_str =
        fmt::format("postgresql://{}:{}@{}:{}/{}", getenv("DB_USERNAME"),
                    getenv("DB_PASSWORD"), getenv("DB_HOSTNAME"),
                    getenv("DB_PORT"), getenv("DB_NAME"));

    std::cout << this->_connection_str;
  };
  //   ~DbConnection() { this->cx.close(); };
  void exec(std::string query) {
    pqxx::connection cx{this->_connection_str};
    pqxx::work tx{cx};
    try {
      pqxx::result r{tx.exec(query)};
      tx.commit();

    } catch (pqxx::sql_error const& e) {
      fmt::print("Error {}", e.what());
    }
  }
  
  pqxx::connection get_connection(){
    pqxx::connection cx{this->_connection_str};
    return cx;
  }
};