#pragma once
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include <iostream>
#include <functional>
#include <deque>
#include <set>
#include <cstdint>
#include <memory>

#include "geo.h"

namespace transport_directory {
	namespace transport_catalogue {

		class Statistics {
		public:
			virtual void Print(std::ostream& os) const = 0;
			virtual ~Statistics() = default;
		};

		class StatBusRoute : public Statistics {
		public:
			StatBusRoute(std::string name, size_t count_stops = 0, size_t count_unique_stops = 0, int route_length = 0, double curvate = 0.0);
			void Print(std::ostream& os) const override;
			~StatBusRoute() = default;
		private:
			std::string name_;
			size_t count_stops_ = 0;
			size_t count_unique_stops_ = 0;
			int route_length_ = 0;
			double curvature_ = 0.0;
		};

		class StatForStop : public Statistics {
		public:
			StatForStop(std::string_view name_stop, const std::set<std::string_view>* buses);
			void Print(std::ostream& os) const override;
			~StatForStop() = default;
		private:
			std::string name_stop_;
			const std::set<std::string_view>* buses_;
		};

		class TransportCatalogue {
		public:

			struct Stop {
				Stop() = default;
				Stop(std::string name_stop, Coordinates coor);
				std::string name;
				Coordinates coordinates;
			};

			struct BusRoute {
				BusRoute() = default;
				BusRoute(std::string name_bus, std::vector<Stop*> bus_route);
				std::string name;
				std::vector<Stop*> route;
			};

			void AddStop(std::string name_stop, Coordinates coordinates);

			void AddBusRoute(std::string name_bus, std::vector<std::string> stops);

			void AddDistance(std::string_view from, std::string_view to, int distance);

			int GetDistance(Stop* from, Stop* to) const;

			template<typename T = StatBusRoute>
			std::shared_ptr<Statistics> RequestStatBusRoute(std::string_view name_bus) const;

			template<typename T = StatForStop>
			std::shared_ptr<Statistics> RequestStatForStop(std::string_view name_stop) const;

			const BusRoute& SearchRoute(std::string_view name_bus) const;

			const Stop& SearchStop(std::string_view name_stop) const;

		private:

			struct DistancesHasher {
				std::hash<uintptr_t> hasher;
				size_t operator()(const std::pair<Stop*, Stop*>& p) const;
			};

			std::deque<Stop> stops_;
			std::deque<BusRoute> bus_routes_;
			std::unordered_map<std::string_view, Stop*> index_stops_;
			std::unordered_map<std::string_view, BusRoute*> index_buses_;
			std::unordered_map<std::pair<Stop*, Stop*>, int, DistancesHasher> distances_;
			std::unordered_map<std::string_view, std::set<std::string_view>> stop_buses_;
		};

		template<typename T>
		std::shared_ptr<Statistics> TransportCatalogue::RequestStatBusRoute(std::string_view name_bus) const {
			auto it = index_buses_.find(name_bus);
			if (it == index_buses_.end()) {
				return std::make_shared<T>(std::string(name_bus));
			}
			const BusRoute& bus_route = *it->second;
			size_t count_stops = bus_route.route.size();
			std::unordered_set<Stop*> unique_stops;
			unique_stops.insert(bus_route.route[0]);
			double distance = 0.0;
			int route_length = 0;
			for (size_t i = 1; i < bus_route.route.size(); ++i) {
				unique_stops.insert(bus_route.route[i]);
				distance += ComputeDistance(bus_route.route[i - 1]->coordinates, bus_route.route[i]->coordinates);
				route_length += GetDistance(bus_route.route[i - 1], bus_route.route[i]);
			}
			size_t count_unique_stops = unique_stops.size();
			double curvature = static_cast<double>(route_length) / distance;
			return std::make_shared<T>(std::string(name_bus), count_stops, count_unique_stops, route_length, curvature);
		}

		template<typename T>
		std::shared_ptr<Statistics> TransportCatalogue::RequestStatForStop(std::string_view name_stop) const {
			auto it = stop_buses_.find(name_stop);
			if (it == stop_buses_.end()) {
				return std::make_shared<T>(name_stop, nullptr);
			}
			return std::make_shared<T>(name_stop, &(it->second));
		}

	}//end namespace transport_catalogue
}// end namespace transport_directory

std::ostream& operator<<(std::ostream& os, const transport_directory::transport_catalogue::Statistics& stat);