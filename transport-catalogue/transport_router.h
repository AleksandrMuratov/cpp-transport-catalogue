#pragma once
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include "domain.h"
#include "ranges.h"
#include <unordered_map>
#include <string_view>
#include <optional>
#include <variant>
#include <memory>

namespace transport_directory {

	namespace transport_router {

		

		class TransportRouter {
		public:
			using Weight = double;
			using Graph = graph::DirectedWeightedGraph<Weight>;
			using VertexId = graph::VertexId;
			using EdgeId = graph::EdgeId;
			using Edge = graph::Edge<Weight>;

			struct RoutingSettings {
				size_t bus_wait_time;// minutes
				double bus_velocity;// km/h
			};

			struct DataEdge {
				Weight weight;
				std::variant<const domain::Stop*, const domain::BusRoute*> obj;
				int spun_count = 0;
			};

			struct RouteInfo {
				Weight weight{};
				std::vector<DataEdge> edges;
			};

			TransportRouter(const transport_catalogue::TransportCatalogue& guide, RoutingSettings routing_settings);

			std::optional<RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

		private:

			Weight ComputeWeightForRoute(ranges::Range<std::vector<const domain::Stop*>::const_iterator> route) const;

			void ConstructGraphAndFillGraphByStops();

			void FillGraphByBusRoutes();

			void AddBusRouteToGraph(const domain::BusRoute& bus_route);

			const transport_catalogue::TransportCatalogue& guide_;
			RoutingSettings routing_settings_;
			std::unordered_map<const domain::Stop*, EdgeId> stop_id_;
			std::unordered_map<EdgeId, DataEdge> edge_id_data_;
			std::unique_ptr<Graph> graph_;
			std::unique_ptr<graph::Router<Weight>> router_;
		};

	}// namespace transport_router

}// namespace transport_directory