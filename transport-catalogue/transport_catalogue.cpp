#include "transport_catalogue.h"
#include <iomanip>

namespace transport_directory {
	namespace transport_catalogue {

		StatBusRoute::StatBusRoute(std::string name, size_t count_stops, size_t count_unique_stops, int route_length, double curvature)
			: name_(std::move(name)),
			count_stops_(count_stops),
			count_unique_stops_(count_unique_stops),
			route_length_(route_length),
			curvature_(curvature) {}

		void StatBusRoute::Print(std::ostream& os) const {
			os << "Bus " << name_ << ": ";
			if (count_stops_) {
				os << count_stops_ << " stops on route, "
					<< count_unique_stops_ << " unique stops, "
					<< route_length_ << " route length, "
					<< std::setprecision(6) << curvature_ << " curvature";
			}
			else {
				os << "not found";
			}
		}

		StatForStop::StatForStop(std::string_view name_stop, const std::set<std::string_view>* buses)
			: name_stop_(name_stop),
			buses_(buses)
		{}

		void StatForStop::Print(std::ostream& os) const {
			os << "Stop " << name_stop_ << ": ";
			if (buses_) {
				if (buses_->empty()) {
					os << "no buses";
				}
				else {
					os << "buses";
					for (const auto& bus : *buses_) {
						os << ' ' << bus;
					}
				}
			}
			else {
				os << "not found";
			}
		}

		void TransportCatalogue::AddStop(std::string name_stop, Coordinates coordinates) {
			auto& stop = stops_.emplace_back(std::move(name_stop), coordinates);
			index_stops_.emplace(stop.name, &stop);
			stop_buses_[stop.name];
		}

		void TransportCatalogue::AddBusRoute(std::string name_bus, std::vector<std::string> stops) {
			std::vector<Stop*> route(stops.size());
			auto& bus = bus_routes_.emplace_back(std::move(name_bus), std::move(route));
			for (size_t i = 0; i < bus.route.size(); ++i) {
				bus.route[i] = index_stops_[stops[i]];
				stop_buses_[stops[i]].insert(bus.name);
			}
			index_buses_.emplace(bus.name, &bus);
		}

		void TransportCatalogue::SetDistance(const Stop* from, const Stop* to, int distance) {
			distances_.emplace(std::pair<const Stop*, const Stop*>{ from, to }, distance);
		}

		int TransportCatalogue::GetDistance(const Stop* from, const Stop* to) const {
			auto it = distances_.find({ from, to });
			if (it == distances_.end()) {
				return distances_.at({ to, from });
			}
			return it->second;
		}

		TransportCatalogue::Stop::Stop(std::string name_stop, Coordinates coor) : name(std::move(name_stop)), coordinates(coor) {}

		TransportCatalogue::BusRoute::BusRoute(std::string name_bus, std::vector<Stop*> bus_route) : name(std::move(name_bus)), route(std::move(bus_route)) {}

		const TransportCatalogue::BusRoute* TransportCatalogue::SearchRoute(std::string_view name_bus) const {
			auto it = index_buses_.find(name_bus);
			if (it == index_buses_.end()) {
				return nullptr;
			}
			return it->second;
		}

		const TransportCatalogue::Stop* TransportCatalogue::SearchStop(std::string_view name_stop) const {
			auto it = index_stops_.find(name_stop);
			if (it == index_stops_.end()) {
				return nullptr;
			}
			return it->second;
		}

		size_t TransportCatalogue::DistancesHasher::operator()(const std::pair<const Stop*, const Stop*>& p) const {
			return hasher((uintptr_t)p.first) + 47 * hasher((uintptr_t)p.second);
		}

	}//end namespace transport_catalogue
}//end namespace transport_directory

std::ostream& operator<<(std::ostream& os, const transport_directory::transport_catalogue::Statistics& stat) {
	stat.Print(os);
	return os;
}