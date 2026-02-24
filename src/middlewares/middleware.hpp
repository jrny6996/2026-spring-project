#pragma once

#include <crow.h>
#include <jwt-cpp/jwt.h>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> protected_routes = {"/play"};

struct BaseMiddleware {

  struct context {};
  void before_handle(crow::request& req, crow::response& res, context& ctx) {
    bool is_protected{false};
    for (std::string& route : protected_routes) {
      CROW_LOG_INFO << req.raw_url;
      if( route == req.raw_url.substr(0, route.length())){
        
        is_protected = true;
        break;
    }
    }
    if (is_protected) {
     
      std::string const token =
          "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXUyJ9."
          "eyJpc3MiOiJhdXRoMCIsInNhbXBsZSI6InRlc3QifQ.lQm3N2bVlqt2-1L-"
          "FsOjtR6uE-L4E9zJutMWKIe1v1M";
      auto decoded_token = jwt::decode(token);

      auto verifier = jwt::verify()
                          .with_issuer("auth0")
                          .with_claim("sample", jwt::claim(std::string("test")))
                          .allow_algorithm(jwt::algorithm::hs256{"secret"});

      try {
        verifier.verify(decoded_token);

      } catch (std::exception& e) {
        auto err = e.what();
        std::string err_str = static_cast<std::string>(err);
        std::cout << err_str << "\n";
        res.redirect("/error");
        res.end();
      }
    }
  }

  void after_handle(crow::request& req, crow::response& res, context& ctx) {}
};
