#pragma once
#include <crow.h>
#include "../middlewares/middleware.hpp"

void add_dashboard_routes(crow::Crow<BaseMiddleware>& app) {

  CROW_ROUTE(app, "/dashboard")([]() {

      auto page = crow::mustache::load("dashboard.html");
      return page.render();
    
  });
}