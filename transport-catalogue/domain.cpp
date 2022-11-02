#include "domain.h"

namespace transport_directory {

	namespace domain {

		Stop::Stop(std::string name_stop, geo::Coordinates coor) : name(std::move(name_stop)), coordinates(coor) {}

		BusRoute::BusRoute(std::string name_bus, std::vector<Stop*> bus_route) : name(std::move(name_bus)), route(std::move(bus_route)) {}

	}// namespace domain
}//namespace transport_directory

