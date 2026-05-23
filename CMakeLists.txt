cmake_minimum_required(VERSION 3.14)
project(taxi_services CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(userver REQUIRED COMPONENTS core sqlite REQUIRED)

add_library(taxi_common STATIC
    services/common/auth/auth_service.cpp
    services/common/auth/auth_checker.cpp
)
target_include_directories(taxi_common PUBLIC services/common)
target_link_libraries(taxi_common PUBLIC userver::core)

add_executable(user_service
    services/user-service/main.cpp
    services/user-service/handlers/handlers.cpp
    services/user-service/repository/sqlite_user_repository.cpp
    services/user-service/repository/user_repository.cpp
)
target_include_directories(user_service PRIVATE services/user-service)
target_link_libraries(user_service
    PRIVATE
        taxi_common
        userver::core
        SQLite::SQLite3)

add_executable(driver_service
    services/driver-service/main.cpp
    services/driver-service/handlers/handlers.cpp
    services/driver-service/repository/sqlite_driver_repository.cpp
    services/driver-service/repository/driver_repository.cpp
)
target_include_directories(driver_service PRIVATE services/driver-service)
target_link_libraries(driver_service
    PRIVATE
        taxi_common
        userver::core
        SQLite::SQLite3)

add_executable(ride_service
    services/ride-service/main.cpp
    services/ride-service/handlers/handlers.cpp
    services/ride-service/clients/driver_client.cpp
    services/ride-service/clients/user_client.cpp
    services/ride-service/repository/sqlite_ride_repository.cpp
    services/ride-service/repository/ride_repository.cpp
)
target_include_directories(ride_service PRIVATE services/ride-service)
target_link_libraries(ride_service
    PRIVATE
        taxi_common
        userver::core
        SQLite::SQLite3)
