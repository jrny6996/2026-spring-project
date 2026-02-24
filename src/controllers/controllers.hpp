#pragma once

#include "./dashboard_controller.hpp"
#include "./user_controller.hpp"


void add_controllers(crow::Crow<BaseMiddleware>& app){
    add_dashboard_routes(app);
    add_user_route(app);
    return;
}


