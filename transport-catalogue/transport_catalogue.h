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
#include <iostream>

#include "domain.h"
#include "geo.h"

namespace transport_directory {
	namespace transport_catalogue {

		struct StatBusRoute {
			StatBusRoute() = default;

			StatBusRoute(std::string name, size_t count_stops = 0, size_t count_unique_stops = 0, int route_length = 0, double curvate = 0.0, const domain::BusRoute* bus_route_ = nullptr);
			
			StatBusRoute& SetName(std::string name);
			StatBusRoute& SetCountStops(size_t count_stops);
			StatBusRoute& SetCountUniqueStops(size_t count_unique_stops);
			StatBusRoute& SetRouteLength(int route_length);
			StatBusRoute& SetCurvate(double curvate);
			StatBusRoute& SetBusRoute(const domain::BusRoute* bus_route);

			std::string name_;
			size_t count_stops_ = 0;
			size_t count_unique_stops_ = 0;
			int route_length_ = 0;
			double curvature_ = 0.0;
			const domain::BusRoute* bus_route_ = nullptr;
		};

		struct StatForStop {

			StatForStop(std::string_view name_stop, const std::set<std::string_view>* buses);

			std::string name_stop_;
			const std::set<std::string_view>* buses_;
		};

		class TransportCatalogue {
		public:

			void AddStop(std::string name_stop, geo::Coordinates coordinates);

			void AddBusRoute(std::string name_bus, std::vector<std::string> stops, bool is_roundtrip = false);

			void SetDistance(const domain::Stop* from, const domain::Stop* to, int distance);

			//return value in meters
			int GetDistance(const domain::Stop* from, const domain::Stop* to) const;

			StatBusRoute RequestStatBusRoute(std::string_view name_bus) const;

			StatForStop RequestStatForStop(std::string_view name_stop) const;

			const domain::BusRoute* SearchRoute(std::string_view name_bus) const;

			const domain::Stop* SearchStop(std::string_view name_stop) const;

			const std::deque<domain::BusRoute>& GetBusRoutes() const;

			const std::deque<domain::Stop>& GetStops() const;

		private:

			struct DistancesHasher {
				std::hash<uintptr_t> hasher;
				size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& p) const;
			};

			std::deque<domain::Stop> stops_;
			std::deque<domain::BusRoute> bus_routes_;
			std::unordered_map<std::string_view, domain::Stop*> index_stops_;
			std::unordered_map<std::string_view, domain::BusRoute*> index_buses_;
			std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, DistancesHasher> distances_;
			std::unordered_map<std::string_view, std::set<std::string_view>> stop_buses_;
		};

		

	}//end namespace transport_catalogue
}// end namespace transport_directory
