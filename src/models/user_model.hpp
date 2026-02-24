#pragma once
#include <sodium.h>
#include <optional>
#include <pqxx/pqxx>
#include <stdexcept>
#include <string>
#include "../services/db_connect.hpp"

class User {
 private:
  std::string password;

 public:
  std::string email;

  User() { return; }
  bool check_exists() { return false; };
};
struct EmailCheck {
  bool status;
  std::optional<pqxx::result> result;
};

EmailCheck check_exists(const std::string& email) {
  DbConnection conn;
  pqxx::connection cx = conn.get_connection();
  pqxx::read_transaction tx{cx};

  pqxx::result r =
      tx.exec_params("SELECT * FROM users WHERE email = $1 LIMIT 1", email);

  if (r.empty()) {
    return {false, std::nullopt};
  }

  return {true, r};
}

std::string hash_with_secret(std::string to_hash) {
  std::string secret_key{getenv("PASSWORD_SALT")};

  if (secret_key.size() < crypto_generichash_KEYBYTES_MIN ||
      secret_key.size() > crypto_generichash_KEYBYTES_MAX) {
    throw std::runtime_error("Invalid key length");
  }

  unsigned char hash[crypto_generichash_BYTES];

  crypto_generichash(
      hash, sizeof hash, reinterpret_cast<const unsigned char*>(to_hash.data()),
      to_hash.size(), reinterpret_cast<const unsigned char*>(secret_key.data()),
      secret_key.size());

  return std::string(reinterpret_cast<char*>(hash), sizeof hash);
}


bool insert_user(std::string email, std::string hashed_pw) {
  try {
    DbConnection conn;
    auto cx = conn.get_connection();
    pqxx::work tx{cx};
    pqxx::result r =
        tx.exec_params("INSERT INTO users (email, password) VALUES ($1, $2)",
                       email, pqxx::binarystring(hashed_pw));
    tx.commit();
    return true;
  } catch (std::exception& e) {
    CROW_LOG_ERROR << e.what();
    return false;
  }
}
