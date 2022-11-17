#pragma once
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include "domain.h"
#include "ranges.h"
#include <unordered_map>
#include <string_view>
#include <optional>

namespace transport_directory {

	namespace transport_router {

		using Weight = double;
		using Graph = graph::DirectedWeightedGraph<Weight>;
		using VertexId = graph::VertexId;
		using EdgeId = graph::EdgeId;
		using Edge = graph::Edge<Weight>;

		class TransportGraph {
		public:
			struct RoutingSettings {
				size_t bus_wait_time;// minutes
				double bus_velocity;// km/h
			};

			class EdgeTransportGraph {
			public:

				enum class TypeTransportObject {
					BUS,
					STOP
				};

				EdgeTransportGraph(const transport_catalogue::TransportCatalogue& guide,
					const domain::BusRoute& bus, double bus_velocity,
					ranges::Range<std::vector<const domain::Stop*>::const_iterator> route);

				EdgeTransportGraph(const domain::Stop& stop, size_t bus_wait_time);
				TypeTransportObject GetType() const;
				Weight GetWait() const;
				std::string_view GetName() const;
				size_t GetSpunCount() const;

			private:
				TypeTransportObject type_;
				std::string_view name_;
				size_t spun_count_;//number of stops the bus made
				Weight wait_{};//minutes
			};

			TransportGraph(const transport_catalogue::TransportCatalogue& guide, RoutingSettings routing_settings);

			std::optional<VertexId> GetVertex(std::string_view name_stop) const;

			std::optional<EdgeTransportGraph> GetEdge(EdgeId id) const;

			const Graph& GetGraph() const;

		private:

			static std::unordered_map<const domain::Stop*, std::pair<VertexId, VertexId>> Indexing(const transport_catalogue::TransportCatalogue& guide);

			void AddBusRouteToGraph(const domain::BusRoute& bus_route, RoutingSettings routing_settings);

			void LoadGraphFromTransportCatalogue(RoutingSettings routing_settings);

			const transport_catalogue::TransportCatalogue& guide_;
			std::unordered_map<const domain::Stop*, std::pair<VertexId, VertexId>> stop_ides_;//start and end of waiting at the bus stop
			std::unordered_map<EdgeId, EdgeTransportGraph> id_edge_;
			Graph graph_;
		};
		
		class TransportRouter {
		public:

			struct RouteInfo {
				Weight wait{};
				std::vector<TransportGraph::EdgeTransportGraph> edges;
			};

			TransportRouter(const TransportGraph& graph);

			TransportRouter(TransportGraph&& graph);

			std::optional<RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

		private:
			TransportGraph graph_;
			graph::Router<Weight> router_;
		};
	}// namespace transport_router

}// namespace transport_directory