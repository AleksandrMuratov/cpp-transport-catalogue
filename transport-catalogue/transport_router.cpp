#include "transport_router.h"

namespace transport_directory {

	namespace transport_router {

		//-------------------------class TransportGraph::EdgeTransportGraph -----------------------------------

		TransportGraph::EdgeTransportGraph::EdgeTransportGraph(const transport_catalogue::TransportCatalogue& guide,
			const domain::BusRoute& bus, double bus_velocity,
			ranges::Range<std::vector<const domain::Stop*>::const_iterator> route)
			: type_(TypeTransportObject::BUS),
			name_(bus.name),
			spun_count_(route.size() - 1)
		{
			int64_t length = 0;//meters
			for (auto it = std::next(route.begin()); it != route.end(); ++it) {
				length += guide.GetDistance(*std::prev(it), *it);
			}
			wait_ = static_cast<double>(length * 60) / (bus_velocity * 1000);
		}

		TransportGraph::EdgeTransportGraph::EdgeTransportGraph(const domain::Stop& stop, size_t bus_wait_time)
			:type_(TypeTransportObject::STOP),
			name_(stop.name),
			spun_count_(0),
			wait_(bus_wait_time)
		{}

		TransportGraph::EdgeTransportGraph::TypeTransportObject TransportGraph::EdgeTransportGraph::GetType() const {
			return type_;
		}
		Weight TransportGraph::EdgeTransportGraph::GetWait() const {
			return wait_;
		}

		std::string_view TransportGraph::EdgeTransportGraph::GetName() const {
			return name_;
		}

		size_t TransportGraph::EdgeTransportGraph::GetSpunCount() const {
			return spun_count_;
		}

		//-----------------------------class TransportGraph -------------------------------------------------

		TransportGraph::TransportGraph(const transport_catalogue::TransportCatalogue& guide, RoutingSettings routing_settings)
			:guide_(guide),
			stop_ides_(Indexing(guide_)),
			graph_(stop_ides_.size() * 2)
		{
			LoadGraphFromTransportCatalogue(routing_settings);
		}

		std::optional<VertexId> TransportGraph::GetVertex(std::string_view name_stop) const {
			//if there are no routes through the stop, then there is no stop in the graph
			auto stat = guide_.RequestStatForStop(name_stop);
			if (!stat.buses_ || stat.buses_->empty()) {
				return std::nullopt;
			}
			const domain::Stop* stop = guide_.SearchStop(name_stop);
			auto it = stop_ides_.find(stop);
			if (it == stop_ides_.end()) {
				return std::nullopt;
			}
			return it->second.first;
		}

		std::optional<TransportGraph::EdgeTransportGraph> TransportGraph::GetEdge(EdgeId id) const {
			auto it = id_edge_.find(id);
			if (it == id_edge_.end()) {
				return std::nullopt;
			}
			return it->second;
		}

		const Graph& TransportGraph::GetGraph() const {
			return graph_;
		}

		std::unordered_map<const domain::Stop*, std::pair<VertexId, VertexId>> TransportGraph::Indexing(const transport_catalogue::TransportCatalogue& guide) {
			//it is assumed that there are no duplicate stops in the TransportCatalogue
			std::unordered_map<const domain::Stop*, std::pair<VertexId, VertexId>> stop_id;
			const auto& stops = guide.GetStops();
			size_t count = 0;
			for (const auto& stop : stops) {
				auto stat = guide.RequestStatForStop(stop.name);
				if (!stat.buses_ || stat.buses_->empty()) {
					continue;
				}
				VertexId begin = count++;
				VertexId end = count++;
				stop_id[&stop] = { begin, end };
			}
			return stop_id;
		}

		void TransportGraph::AddBusRouteToGraph(const domain::BusRoute& bus_route, RoutingSettings routing_settings) {
			for (auto it = bus_route.route.begin();
				it != bus_route.route.end() && it != std::prev(bus_route.route.end());
				++it) {
				const domain::Stop* start_stop = *it;
				EdgeTransportGraph edge_stop(*start_stop, routing_settings.bus_wait_time);
				auto [begin, end] = stop_ides_[start_stop];
				EdgeId id_edge_stop = graph_.AddEdge(Edge{ begin, end, edge_stop.GetWait() });
				id_edge_.emplace(id_edge_stop, edge_stop);
				for (auto it2 = std::next(it); it2 != bus_route.route.end(); ++it2) {
					EdgeTransportGraph edge_bus(guide_, bus_route, routing_settings.bus_velocity, ranges::Range(it, std::next(it2)));
					auto start = stop_ides_[*it].second; //starting from the end of waiting
					auto finish = stop_ides_[*it2].first;//the end of the movement is at the beginning of the waiting
					EdgeId id_edge_bus = graph_.AddEdge(Edge{ start, finish, edge_bus.GetWait() });
					id_edge_.emplace(id_edge_bus, edge_bus);
				}
			}
		}

		void TransportGraph::LoadGraphFromTransportCatalogue(RoutingSettings routing_settings) {
			const auto& bus_routes = guide_.GetBusRoutes();
			for (const auto& bus_route : bus_routes) {
				AddBusRouteToGraph(bus_route, routing_settings);
			}
		}

		//------------------------------------ class TransportRouter ----------------------------------------------------

		TransportRouter::TransportRouter(const TransportGraph& graph)
			: graph_(graph),
			router_(graph_.GetGraph())
		{}

		TransportRouter::TransportRouter(TransportGraph&& graph)
			:graph_(std::move(graph)),
			router_(graph_.GetGraph())
		{}

		std::optional<TransportRouter::RouteInfo> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const {
			auto start = graph_.GetVertex(from);
			auto finish = graph_.GetVertex(to);
			if (!start || !finish) {
				return std::nullopt;
			}
			auto route_info = router_.BuildRoute(*start, *finish);
			if (!route_info) {
				return std::nullopt;
			}
			RouteInfo result;
			result.wait = route_info->weight;
			result.edges.reserve(route_info->edges.size());
			for (const EdgeId id : route_info->edges) {
				result.edges.push_back(std::move(*graph_.GetEdge(id)));
			}
			return result;
		}

	}// namespace transport_router

}// namespace transport_directory