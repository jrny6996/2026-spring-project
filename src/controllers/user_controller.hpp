#pragma once
#include <crow.h>
#include <crow/http_request.h>
#include <fmt/format.h>

#include <optional>
#include <string>
#include "../middlewares/middleware.hpp"
#include "../models/user_model.hpp"
#include "../services/db_connect.hpp"

struct StatusString {
  int http_code;
  std::string res_string;
};

std::string user_onboarding(const crow::query_string params) {
  std::string status_str = "n/a";

  char* email = params.get("email");
  std::string pw = static_cast<std::string>(params.get("password"));

  if (!email || !pw.data()) {
    return "Missing required fields";
  }

  if (check_exists(email).status) {
    status_str = "User already exists. Please login: <a href='/login'>here</a>";
  } else {

    std::string hashed_pw = hash_with_secret(pw);
    if (insert_user(email, hashed_pw))
      status_str = "Success! Created new user in DB <hr/>Login: <a href='/login?new-account=true'>here</a>";
    else
      status_str = "failed to insert user";
  }
  return status_str;
}

void add_user_route(crow::Crow<BaseMiddleware>& app) {

  CROW_ROUTE(app, "/api/users")
      .methods(crow::HTTPMethod::POST, crow::HTTPMethod::PATCH)([](const crow::request& req) {
        crow::HTTPMethod method = req.method;
        std::string msg = "n/a";
        if (crow::method_name(method) == "POST") {
          auto params = req.get_body_params();
          msg = user_onboarding(params);
        }
        std::string body = fmt::format("<h1>User performed {}</h1><h3>{}</h3> ",
                                       crow::method_name(method), msg);

        CROW_LOG_INFO << "Content-Type: "
                      << req.get_header_value("Content-Type");
        CROW_LOG_INFO << "Raw body: " << req.body;
   
        return body;
      });
  CROW_ROUTE(app, "/login")([]() {
    auto page = crow::mustache::load("login.html");
    return page.render();
  });
  return;
}