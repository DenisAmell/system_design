#pragma once

#include <string>

namespace taxi::driver {

struct Driver {
    std::string id;
    std::string user_id;
    std::string login;
    std::string car_model;
    std::string car_number;
    std::string car_class;
    std::string status{"OFFLINE"};
};

}  // namespace taxi::driver
