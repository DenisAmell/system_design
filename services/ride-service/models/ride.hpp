#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace taxi::ride {

struct GeoPoint {
    double lat{};
    double lon{};
};

enum class RideStatus { kCreated, kAccepted, kCompleted, kCancelled };

inline std::string ToString(RideStatus s) {
    switch (s) {
        case RideStatus::kCreated:   return "CREATED";
        case RideStatus::kAccepted:  return "ACCEPTED";
        case RideStatus::kCompleted: return "COMPLETED";
        case RideStatus::kCancelled: return "CANCELLED";
    }
    return "UNKNOWN";
}

inline RideStatus StatusFromString(std::string_view s) {
    if (s == "ACCEPTED")  return RideStatus::kAccepted;
    if (s == "COMPLETED") return RideStatus::kCompleted;
    if (s == "CANCELLED") return RideStatus::kCancelled;
    return RideStatus::kCreated;
}

struct Ride {
    std::string id;
    std::string passenger_login;
    std::optional<std::string> driver_login;
    GeoPoint from;
    GeoPoint to;
    std::string car_class;
    RideStatus status{RideStatus::kCreated};
    double price{};
    std::chrono::system_clock::time_point created_at{
        std::chrono::system_clock::now()};
};

}  // namespace taxi::ride
