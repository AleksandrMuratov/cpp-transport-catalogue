#pragma once

namespace geo {

    struct Coordinates {
        double lat = 0.0; // Широта
        double lng = 0.0; // Долгота
        bool operator==(const Coordinates& other) const {
            return lat == other.lat && lng == other.lng;
        }
        bool operator!=(const Coordinates& other) const {
            return !(*this == other);
        }
    };

    double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo