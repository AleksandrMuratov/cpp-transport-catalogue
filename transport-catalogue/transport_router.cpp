#include "transport_router.h"

namespace transport_directory {

	namespace transport_router {

		//-----------------------------class TransportRouter -------------------------------------------------

		TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& guide, RoutingSettings routing_settings)
			:guide_(guide),
			routing_settings_(routing_settings)
		{
			ConstructGraphAndFillGraphByStops();
			FillGraphByBusRoutes();
			router_ = std::make_unique<graph::Router<Weight>>(*graph_);
		}

		TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& guide, RoutingSettings routing_settings, const DownloadedData& data_for_router)
			: guide_(guide)
			, routing_settings_(std::move(routing_settings))
			, stop_id_(data_for_router.stop_id)
			, edge_id_data_(data_for_router.edge_id_data)
			, graph_(std::make_unique<Graph>(*data_for_router.graph))
			, router_(std::make_unique<graph::Router<Weight>>(*graph_, data_for_router.data_of_router))
		{}

		std::optional<TransportRouter::RouteInfo> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const {
			const domain::Stop* start = guide_.SearchStop(from);
			const domain::Stop* finish = guide_.SearchStop(to);
			if (!start || !finish) {
				return std::nullopt;
			}
			auto it_start = stop_id_.find(start);
			auto it_finish = stop_id_.find(finish);
			if (it_start == stop_id_.end() || it_finish == stop_id_.end()) {
				return std::nullopt;
			}
			VertexId from_id = graph_->GetEdge(it_start->second).from;
			VertexId to_id = graph_->GetEdge(it_finish->second).from;
			auto route_info = router_->BuildRoute(from_id, to_id);
			if (!route_info) {
				return std::nullopt;
			}
			RouteInfo result;
			result.weight = route_info->weight;
			for (const EdgeId id : route_info->edges) {
				result.edges.push_back(edge_id_data_.at(id));
			}
			return result;
		}

		TransportRouter::DownloadedData TransportRouter::GetDataForTransRouter() const {
			DownloadedData data;
			data.graph = std::make_unique<Graph>(*graph_);
			data.edge_id_data = edge_id_data_;
			data.stop_id = stop_id_;
			data.data_of_router = router_->GetData();
			return data;
		}

		const std::unordered_map<const domain::Stop*, EdgeId>& TransportRouter::GetStopId() const {
			return stop_id_;
		}

		const std::unordered_map<EdgeId, TransportRouter::DataEdge>& TransportRouter::GetEdgeIdData() const {
			return edge_id_data_;
		}

		const Graph& TransportRouter::GetGraph() const {
			return *graph_;
		}

		const graph::Router<Weight>& TransportRouter::GetRouter() const {
			return *router_;
		}

		void TransportRouter::ConstructGraphAndFillGraphByStops() {
			const auto& stops = guide_.GetStops();
			for (const auto& stop : stops) {
				auto stat = guide_.RequestStatForStop(stop.name);
				if (!stat.buses_ || stat.buses_->empty()) {
					continue;
				}
				stop_id_[&stop];
			}
			graph_ = std::make_unique<Graph>(stop_id_.size() * 2);
			size_t count = 0;
			for (auto& [stop, id] : stop_id_) {
				VertexId from = count++;
				VertexId to = count++;
				id = graph_->AddEdge(Edge{ from, to, static_cast<double>(routing_settings_.bus_wait_time) });
				edge_id_data_.emplace(id, DataEdge{ static_cast<double>(routing_settings_.bus_wait_time), stop, 0 });
			}
		}

		void TransportRouter::FillGraphByBusRoutes() {
			const auto& bus_routes = guide_.GetBusRoutes();
			for (const auto& bus_route : bus_routes) {
				AddBusRouteToGraph(bus_route);
			}
		}

		Weight TransportRouter::ComputeWeightForRoute(ranges::Range<std::vector<const domain::Stop*>::const_iterator> route) const {
			int64_t length = 0;//meters
			for (auto it = std::next(route.begin()); it != route.end(); ++it) {
				length += guide_.GetDistance(*std::prev(it), *it);
			}
			return static_cast<double>(length * 60) / (routing_settings_.bus_velocity * 1000);
		}

		void TransportRouter::AddBusRouteToGraph(const domain::BusRoute& bus_route) {
			for (auto it = bus_route.route.begin();
				it != bus_route.route.end() && it != std::prev(bus_route.route.end());
				++it) {
				for (auto it2 = std::next(it); it2 != bus_route.route.end(); ++it2) {
					VertexId from = graph_->GetEdge(stop_id_[*it]).to; // starting from the end of waiting
					VertexId to = graph_->GetEdge(stop_id_[*it2]).from; // the end of the movement is at the beginning of the waiting
					Weight weight = ComputeWeightForRoute(ranges::Range(it, std::next(it2)));
					EdgeId id = graph_->AddEdge(Edge{ from, to, weight });
					edge_id_data_.emplace(id, DataEdge{weight, &bus_route, static_cast<int>(std::distance(it, it2)) - 1});
				}
			}
		}

	}// namespace transport_router

}// namespace transport_directory