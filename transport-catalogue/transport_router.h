#pragma once
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include "domain.h"
#include "ranges.h"

#include <unordered_map>
#include <string_view>
#include <variant>
#include <memory>

namespace transport_directory {

	namespace transport_router {

		using Weight = double;
		using Graph = graph::DirectedWeightedGraph<Weight>;
		using VertexId = graph::VertexId;
		using EdgeId = graph::EdgeId;
		using Edge = graph::Edge<Weight>;

		class TransportRouter {
		public:

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

			struct DownloadedData {
				std::unique_ptr<Graph> graph;
				graph::Router<Weight>::RoutesInternalData data_of_router;
				std::unordered_map<const domain::Stop*, EdgeId> stop_id;
				std::unordered_map<EdgeId, TransportRouter::DataEdge> edge_id_data;
			};

			TransportRouter(const transport_catalogue::TransportCatalogue& guide, RoutingSettings routing_settings);

			TransportRouter(const transport_catalogue::TransportCatalogue& guide, RoutingSettings routing_settings, const DownloadedData& data_for_router);

			std::optional<RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

			DownloadedData GetDataForTransRouter() const;

			const std::unordered_map<const domain::Stop*, EdgeId>& GetStopId() const;
			const std::unordered_map<EdgeId, DataEdge>& GetEdgeIdData() const;
			const Graph& GetGraph() const;
			const graph::Router<Weight>& GetRouter() const;

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