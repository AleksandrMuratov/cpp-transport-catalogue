#pragma once
#include <string>
#include <vector>
#include "geo.h"

namespace transport_directory {

	namespace domain {

		struct Stop {
			Stop() = default;
			Stop(std::string name_stop, geo::Coordinates coor);
			std::string name;
			geo::Coordinates coordinates;
		};

		struct BusRoute {
			BusRoute() = default;
			BusRoute(std::string name_bus, std::vector<const Stop*> bus_route);
			std::string name;
			std::vector<const Stop*> route;
			bool is_roundtrip = false;
		};

	}// namespace domain
}//namespace transport_directory

