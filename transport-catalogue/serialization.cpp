#include "serialization.h"

#include <algorithm>
#include <variant>
#include <string>
#include <fstream>
#include <memory>

namespace transport_directory {
	namespace serialization_tr_catalogue {

		namespace detail {
			transport_catalogue_serialize::Stop CreateProtoStop(const domain::Stop& stop) {
				transport_catalogue_serialize::Stop proto_stop;
				proto_stop.set_name(stop.name);
				transport_catalogue_serialize::Coordinates proto_coors;
				proto_coors.set_lat(stop.coordinates.lat);
				proto_coors.set_lng(stop.coordinates.lng);
				*proto_stop.mutable_coordinates() = std::move(proto_coors);
				return proto_stop;
			}

			transport_catalogue_serialize::BusRoute CreateProtoBusRoute(const domain::BusRoute& bus_route,
				const std::unordered_map<std::string_view, size_t>& stops_indexes) {

				transport_catalogue_serialize::BusRoute proto_bus_route;
				proto_bus_route.set_name(bus_route.name);
				proto_bus_route.set_roundtrip(bus_route.is_roundtrip);
				for (const domain::Stop* stop : bus_route.route) {
					proto_bus_route.add_index_stops(stops_indexes.at(stop->name));
				}
				return proto_bus_route;
			}

			transport_catalogue_serialize::Distance CreateProtoDistance(const std::pair<const domain::Stop*, const domain::Stop*>& stops,
				int distance, const std::unordered_map<std::string_view, size_t>& stops_indexes) {

				transport_catalogue_serialize::Distance proto_distance;
				proto_distance.set_index_stop_from(stops_indexes.at(stops.first->name));
				proto_distance.set_index_stop_to(stops_indexes.at(stops.second->name));
				proto_distance.set_distance(distance);
				return proto_distance;
			}

			transport_catalogue_serialize::TransportCatalogue CreateProtoTransportCatalogue(const transport_catalogue::TransportCatalogue& guide) {
				const std::deque<domain::BusRoute>& bus_routes = guide.GetBusRoutes();
				const std::deque<domain::Stop>& stops = guide.GetStops();
				const std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, transport_catalogue::TransportCatalogue::DistancesHasher>& 
					distances = guide.GetDistances();

				transport_catalogue_serialize::TransportCatalogue proto_guide;
				const auto& stops_indexes = guide.GetIndexedOfStops();
				int index = 0;
				for (const auto& stop : stops) {
					transport_catalogue_serialize::Stop proto_stop(CreateProtoStop(stop));
					*proto_guide.add_stops() = std::move(proto_stop);
				}
				for (const auto& bus_route : bus_routes) {
					transport_catalogue_serialize::BusRoute proto_bus_route(CreateProtoBusRoute(bus_route, stops_indexes));
					*proto_guide.add_bus_routes() = std::move(proto_bus_route);
				}
				for (const auto& [stop, distance] : distances) {
					transport_catalogue_serialize::Distance proto_distance(CreateProtoDistance(stop, distance, stops_indexes));
					*proto_guide.add_distances() = std::move(proto_distance);
				}
				return proto_guide;
			}

			std::vector<BusRoute> GetBusRoutes(const transport_catalogue_serialize::TransportCatalogue& proto_guide) {
				std::vector<BusRoute> bus_routes;
				bus_routes.reserve(proto_guide.bus_routes_size());
				for (int i = 0; i < proto_guide.bus_routes_size(); ++i) {
					const transport_catalogue_serialize::BusRoute& proto_bus_route = proto_guide.bus_routes(i);
					BusRoute& bus_route = bus_routes.emplace_back();
					bus_route.name = proto_bus_route.name();
					bus_route.is_roundtrip = proto_bus_route.roundtrip();
					bus_route.stops.reserve(proto_bus_route.index_stops_size());
					for (int j = 0; j < proto_bus_route.index_stops_size(); ++j) {
						int index_of_stop = proto_bus_route.index_stops(j);
						const transport_catalogue_serialize::Stop& proto_stop = proto_guide.stops(index_of_stop);
						bus_route.stops.push_back(proto_stop.name());
					}
				}
				return bus_routes;
			}

			std::vector<domain::Stop> GetStops(const transport_catalogue_serialize::TransportCatalogue& proto_guide) {
				std::vector<domain::Stop> stops;
				stops.reserve(proto_guide.stops_size());
				for (int i = 0; i < proto_guide.stops_size(); ++i) {
					const transport_catalogue_serialize::Stop& proto_stop = proto_guide.stops(i);
					domain::Stop& stop = stops.emplace_back();
					stop.name = proto_stop.name();
					stop.coordinates.lat = proto_stop.coordinates().lat();
					stop.coordinates.lng = proto_stop.coordinates().lng();
				}
				return stops;
			}

			std::vector<DistanceBetweenStops> GetDistances(const transport_catalogue_serialize::TransportCatalogue& proto_guide) {
				std::vector<DistanceBetweenStops> distances;
				distances.reserve(proto_guide.distances_size());
				for (int i = 0; i < proto_guide.distances_size(); ++i) {
					const transport_catalogue_serialize::Distance& proto_distance = proto_guide.distances(i);
					DistanceBetweenStops& distance = distances.emplace_back();
					int index_of_stop_from = proto_distance.index_stop_from();
					int index_of_stop_to = proto_distance.index_stop_to();
					const transport_catalogue_serialize::Stop& proto_stop_from = proto_guide.stops(index_of_stop_from);
					const transport_catalogue_serialize::Stop& proto_stop_to = proto_guide.stops(index_of_stop_to);
					distance.from = proto_stop_from.name();
					distance.to = proto_stop_to.name();
					distance.distance = proto_distance.distance();
				}
				return distances;
			}

			transport_catalogue::TransportCatalogue GetTransportCatalogue(const transport_catalogue_serialize::TransportCatalogue& proto_guide) {
				transport_catalogue::TransportCatalogue guide;
				std::vector<BusRoute> bus_routes(GetBusRoutes(proto_guide));
				std::vector<domain::Stop> stops(GetStops(proto_guide));
				std::vector<DistanceBetweenStops> distances(GetDistances(proto_guide));

				std::for_each(stops.begin(), stops.end(), [&guide](domain::Stop& stop) {
					guide.AddStop(std::move(stop.name), stop.coordinates);
					});
				
				std::for_each(distances.begin(), distances.end(), [&guide](const DistanceBetweenStops& distance) {
					const domain::Stop* from = guide.SearchStop(distance.from);
					const domain::Stop* to = guide.SearchStop(distance.to);
					guide.SetDistance(from, to, distance.distance);
					});

				std::for_each(bus_routes.begin(), bus_routes.end(), [&guide](BusRoute& bus_route) {
					guide.AddBusRoute(std::move(bus_route.name), std::move(bus_route.stops), bus_route.is_roundtrip);
					});

				return guide;
			}

			transport_catalogue_serialize::Color CreateProtoColor(const svg::Color& color) {
				transport_catalogue_serialize::Color proto_color;
				if (std::holds_alternative<svg::Rgb>(color)) {
					transport_catalogue_serialize::Rgb proto_rgb;
					const svg::Rgb& rgb = std::get<svg::Rgb>(color);
					proto_rgb.set_red(rgb.red);
					proto_rgb.set_green(rgb.green);
					proto_rgb.set_blue(rgb.blue);
					*proto_color.mutable_rgb() = std::move(proto_rgb);
				}
				else if (std::holds_alternative<svg::Rgba>(color)) {
					transport_catalogue_serialize::Rgba proto_rgba;
					const svg::Rgba& rgba = std::get<svg::Rgba>(color);
					proto_rgba.set_red(rgba.red);
					proto_rgba.set_green(rgba.green);
					proto_rgba.set_blue(rgba.blue);
					proto_rgba.set_opacity(rgba.opacity);
					*proto_color.mutable_rgba() = std::move(proto_rgba);
				}
				else if (std::holds_alternative<std::string>(color)) {
					proto_color.set_color(std::get<std::string>(color));
				}
				return proto_color;
			}

			transport_catalogue_serialize::Offset CreateProtoOffset(const renderer::RenderSettings::OffSet& offset) {
				transport_catalogue_serialize::Offset proto_offset;
				proto_offset.set_x(offset.x);
				proto_offset.set_y(offset.y);
				return proto_offset;
			}

			transport_catalogue_serialize::RenderSettings CreateProtoRenderSettings(const renderer::RenderSettings& render_settings) {
				transport_catalogue_serialize::RenderSettings proto_render_settings;
				proto_render_settings.set_width(render_settings.width_);
				proto_render_settings.set_height(render_settings.height_);
				proto_render_settings.set_padding(render_settings.padding_);
				proto_render_settings.set_line_width(render_settings.line_width_);
				proto_render_settings.set_stop_radius(render_settings.stop_radius_);
				proto_render_settings.set_bus_label_font_size(render_settings.bus_label_font_size_);

				transport_catalogue_serialize::Offset proto_bus_offset(CreateProtoOffset(render_settings.bus_label_offset_));
				*proto_render_settings.mutable_bus_label_offset() = std::move(proto_bus_offset);

				proto_render_settings.set_stop_label_font_size(render_settings.stop_label_font_size_);

				transport_catalogue_serialize::Offset proto_stop_offset(CreateProtoOffset(render_settings.stop_label_offset_));
				*proto_render_settings.mutable_stop_label_offset() = std::move(proto_stop_offset);

				transport_catalogue_serialize::Color proto_underlayer_color(CreateProtoColor(render_settings.underlayer_color_));
				*proto_render_settings.mutable_underlayer_color() = std::move(proto_underlayer_color);

				proto_render_settings.set_underlayer_width(render_settings.underlayer_width_);

				for (const svg::Color& color : render_settings.color_palette_) {
					transport_catalogue_serialize::Color proto_color(CreateProtoColor(color));
					*proto_render_settings.add_color_palette() = std::move(proto_color);
				}

				return proto_render_settings;
			}

			svg::Rgb GetRgb(const transport_catalogue_serialize::Rgb& proto_rgb) {
				svg::Rgb rgb;
				rgb.red = proto_rgb.red();
				rgb.green = proto_rgb.green();
				rgb.blue = proto_rgb.blue();
				return rgb;
			}

			svg::Rgba GetRgba(const transport_catalogue_serialize::Rgba& proto_rgba) {
				svg::Rgba rgba;
				rgba.red = proto_rgba.red();
				rgba.green = proto_rgba.green();
				rgba.blue = proto_rgba.blue();
				rgba.opacity = proto_rgba.opacity();
				return rgba;
			}

			svg::Color GetColor(const transport_catalogue_serialize::Color& proto_color) {
				if (proto_color.has_rgb()) {
					return GetRgb(proto_color.rgb());
				}
				else if (proto_color.has_rgba()) {
					return GetRgba(proto_color.rgba());
				}
				else if (!proto_color.color().empty()) {
					return proto_color.color();
				}
				return {};
			}

			renderer::RenderSettings::OffSet GetOffset(const transport_catalogue_serialize::Offset& proto_offset) {
				renderer::RenderSettings::OffSet offset;
				offset.x = proto_offset.x();
				offset.y = proto_offset.y();
				return offset;
			}

			renderer::RenderSettings GetRenderSettings(const transport_catalogue_serialize::RenderSettings& proto_render_settings) {
				renderer::RenderSettings render_settings;
				render_settings.width_ = proto_render_settings.width();
				render_settings.height_ = proto_render_settings.height();
				render_settings.padding_ = proto_render_settings.padding();
				render_settings.line_width_ = proto_render_settings.line_width();
				render_settings.stop_radius_ = proto_render_settings.stop_radius();
				render_settings.bus_label_font_size_ = proto_render_settings.bus_label_font_size();
				render_settings.bus_label_offset_ = GetOffset(proto_render_settings.bus_label_offset());
				render_settings.stop_label_font_size_ = proto_render_settings.stop_label_font_size();
				render_settings.stop_label_offset_ = GetOffset(proto_render_settings.stop_label_offset());
				render_settings.underlayer_color_ = GetColor(proto_render_settings.underlayer_color());
				render_settings.underlayer_width_ = proto_render_settings.underlayer_width();

				render_settings.color_palette_.reserve(proto_render_settings.color_palette_size());
				for (int i = 0; i < proto_render_settings.color_palette_size(); ++i) {
					render_settings.color_palette_.emplace_back(GetColor(proto_render_settings.color_palette(i)));
				}

				return render_settings;
			}

			transport_catalogue_serialize::RoutingSettings CreateProtoRoutingSettings(
				const transport_router::TransportRouter::RoutingSettings& routing_settings) {

				transport_catalogue_serialize::RoutingSettings proto_routing_settings;
				proto_routing_settings.set_bus_wait_time(routing_settings.bus_wait_time);
				proto_routing_settings.set_bus_velocity(routing_settings.bus_velocity);
				return proto_routing_settings;
			}

			transport_router::TransportRouter::RoutingSettings GetRoutingSettings(
				const transport_catalogue_serialize::RoutingSettings proto_routing_settings) {

				transport_router::TransportRouter::RoutingSettings routing_settings;
				routing_settings.bus_wait_time = proto_routing_settings.bus_wait_time();
				routing_settings.bus_velocity = proto_routing_settings.bus_velocity();
				return routing_settings;
			}

			transport_catalogue_serialize::Edge CreateProtoEdge(const graph::Edge<transport_router::Weight>& edge) {
				transport_catalogue_serialize::Edge proto_edge;
				proto_edge.set_from(edge.from);
				proto_edge.set_to(edge.to);
				proto_edge.set_weight(edge.weight);
				return proto_edge;
			}

			graph::Edge<transport_router::Weight> GetEdge(const transport_catalogue_serialize::Edge& proto_edge) {
				graph::Edge<transport_router::Weight> edge;
				edge.from = proto_edge.from();
				edge.to = proto_edge.to();
				edge.weight = proto_edge.weight();
				return edge;
			}

			transport_catalogue_serialize::Graph CreateProtoGraph(const transport_router::Graph& graph) {
				transport_catalogue_serialize::Graph proto_graph;
				for (const auto& edge : graph.GetEdges()) {
					*proto_graph.add_edges() = CreateProtoEdge(edge);
				}
				proto_graph.set_vertex_count(graph.GetVertexCount());
				return proto_graph;
			}

			transport_router::Graph GetGraph(const transport_catalogue_serialize::Graph& proto_graph) {
				transport_router::Graph graph(proto_graph.vertex_count());
				for (int i = 0; i < proto_graph.edges_size(); ++i) {
					graph.AddEdge(GetEdge(proto_graph.edges(i)));
				}
				return graph;
			}

			transport_catalogue_serialize::RouteInternalData CreateProtoRouteInternalData(
				const graph::Router<transport_router::Weight>::RouteInternalData& route_internal_data) {

				transport_catalogue_serialize::RouteInternalData proto_route_internal_data;
				proto_route_internal_data.set_weight(route_internal_data.weight);
				if (route_internal_data.prev_edge.has_value()) {
					transport_catalogue_serialize::EdgeId proto_prev_edge;
					proto_prev_edge.set_id(*route_internal_data.prev_edge);
					*proto_route_internal_data.mutable_prev_edge() = std::move(proto_prev_edge);
				}
				proto_route_internal_data.set_is_init(true);
				return proto_route_internal_data;
			}

			graph::Router<transport_router::Weight>::RouteInternalData GetRouteInternalData(
				const transport_catalogue_serialize::RouteInternalData& proto_route_internal_data) {

				graph::Router<transport_router::Weight>::RouteInternalData route_internal_data;
				route_internal_data.weight = proto_route_internal_data.weight();
				if (proto_route_internal_data.has_prev_edge()) {
					route_internal_data.prev_edge = proto_route_internal_data.prev_edge().id();
				}
				return route_internal_data;
			}

			transport_catalogue_serialize::ArrayRouteInternalData CreateProtoArrayRouteInternalData(
				std::vector<std::optional<graph::Router<transport_router::Weight>::RouteInternalData>> array_route_internal_data) {

				transport_catalogue_serialize::ArrayRouteInternalData proto_array_route_internal_data;
				for (const auto& opt_route_internal_data : array_route_internal_data) {
					auto ptr_elem = proto_array_route_internal_data.add_array_route_internal_data();
					if (opt_route_internal_data) {
						*ptr_elem = CreateProtoRouteInternalData(*opt_route_internal_data);
					}
				}
				return proto_array_route_internal_data;
			}

			std::vector<std::optional<graph::Router<transport_router::Weight>::RouteInternalData>> GetArrayRouteInternalData(
				const transport_catalogue_serialize::ArrayRouteInternalData& proto_array_route_internal_data) {

				std::vector<std::optional<graph::Router<transport_router::Weight>::RouteInternalData>>
					array_route_internal_data(proto_array_route_internal_data.array_route_internal_data_size());

				for (int i = 0; i < proto_array_route_internal_data.array_route_internal_data_size(); ++i) {
					if (proto_array_route_internal_data.array_route_internal_data(i).is_init()) {
						array_route_internal_data[i] = GetRouteInternalData(proto_array_route_internal_data.array_route_internal_data(i));
					}
				}
				return array_route_internal_data;
			}

			transport_catalogue_serialize::StopId CreateProtoPairStopId(
				const std::pair<const domain::Stop*, transport_router::EdgeId>& stop_id, 
				const transport_catalogue::TransportCatalogue& guide) {

				const auto& stop_indexes = guide.GetIndexedOfStops();
				transport_catalogue_serialize::StopId proto_stop_id;
				proto_stop_id.set_stop(stop_indexes.at(stop_id.first->name));
				proto_stop_id.set_id(stop_id.second);
				return proto_stop_id;
			}

			std::pair<const domain::Stop*, transport_router::EdgeId> GetPairStopId(transport_catalogue_serialize::StopId proto_stop_id,
				const transport_catalogue::TransportCatalogue& guide) {
				const auto& stops = guide.GetStops();
				std::pair<const domain::Stop*, transport_router::EdgeId> stop_id{&stops[proto_stop_id.stop()], proto_stop_id.id() };
				return stop_id;
			}

			transport_catalogue_serialize::DataEdge CreateProtoDataEdge(const transport_router::TransportRouter::DataEdge& data_edge, 
				const transport_catalogue::TransportCatalogue& guide) {
				transport_catalogue_serialize::DataEdge proto_data_edge;
				if (std::holds_alternative<const domain::Stop*>(data_edge.obj)) {
					const auto& stop_indexes = guide.GetIndexedOfStops();
					transport_catalogue_serialize::StopName stop_name;
					stop_name.set_name(stop_indexes.at(std::get<const domain::Stop*>(data_edge.obj)->name));
					*proto_data_edge.mutable_stop() = std::move(stop_name);
				}
				else {
					const auto& bus_route_indexes = guide.GetIndexesOfBusRoutes();
					transport_catalogue_serialize::BusName bus_name;
					bus_name.set_name(bus_route_indexes.at(std::get<const domain::BusRoute*>(data_edge.obj)->name));
					*proto_data_edge.mutable_bus() = std::move(bus_name);
				}
				proto_data_edge.set_weight(data_edge.weight);
				proto_data_edge.set_spun_count(data_edge.spun_count);
				return proto_data_edge;
			}

			transport_router::TransportRouter::DataEdge GetDataEdge(const transport_catalogue_serialize::DataEdge& proto_data_edge, 
				const transport_catalogue::TransportCatalogue& guide) {
				transport_router::TransportRouter::DataEdge data_edge;
				if (proto_data_edge.has_stop()) {
					const auto& stops = guide.GetStops();
					data_edge.obj = &stops[proto_data_edge.stop().name()];
				}
				else if (proto_data_edge.has_bus()) {
					const auto& buses = guide.GetBusRoutes();
					data_edge.obj = &buses[proto_data_edge.bus().name()];
				}
				data_edge.weight = proto_data_edge.weight();
				data_edge.spun_count = proto_data_edge.spun_count();
				return data_edge;
			}

			transport_catalogue_serialize::EdgeIdData CreateProtoPairEdgeIdData(
				const std::pair<transport_router::EdgeId, transport_router::TransportRouter::DataEdge>& edge_id_data, 
				const transport_catalogue::TransportCatalogue& guide) {

				transport_catalogue_serialize::EdgeIdData proto_edge_id_data;
				proto_edge_id_data.set_edge_id(edge_id_data.first);
				*proto_edge_id_data.mutable_data_edge() = CreateProtoDataEdge(edge_id_data.second, guide);
				return proto_edge_id_data;
			}

			std::pair<transport_router::EdgeId, transport_router::TransportRouter::DataEdge> GetPairEdgeIdData(
				const transport_catalogue_serialize::EdgeIdData& proto_edge_id_data, const transport_catalogue::TransportCatalogue& guide) {

				return std::pair<transport_router::EdgeId, transport_router::TransportRouter::DataEdge>{ proto_edge_id_data.edge_id(), GetDataEdge(proto_edge_id_data.data_edge(), guide) };
			}

			transport_catalogue_serialize::DataForTransportRouter CreateProtoDataForTransportRouter(
				const json_reader::DownloadedDataForTransportRouter& data) {

				transport_catalogue_serialize::DataForTransportRouter proto_data;
				*proto_data.mutable_guide() = CreateProtoTransportCatalogue(data.guide);
				*proto_data.mutable_render_settings() = CreateProtoRenderSettings(data.render_settings);
				*proto_data.mutable_routing_settings() = CreateProtoRoutingSettings(data.routing_settings);

				if (data.data_for_router) {
					*proto_data.mutable_graph() = CreateProtoGraph(*data.data_for_router->graph);

					const graph::Router<transport_router::Weight>::RoutesInternalData& 
						routes_internal_data = data.data_for_router->data_of_router;
					for (const auto& elem : routes_internal_data) {
						*proto_data.add_data_of_router() = CreateProtoArrayRouteInternalData(elem);
					}

					const std::unordered_map<const domain::Stop*, transport_router::EdgeId>& 
						stop_id = data.data_for_router->stop_id;
					for (const auto& elem : stop_id) {
						*proto_data.add_stop_id() = CreateProtoPairStopId(elem, data.guide);
					}

					const std::unordered_map<transport_router::EdgeId, transport_router::TransportRouter::DataEdge>&
						edge_id_data = data.data_for_router->edge_id_data;
					for (const auto& elem : edge_id_data) {
						*proto_data.add_edge_id_data() = CreateProtoPairEdgeIdData(elem, data.guide);
					}
				}

				return proto_data;
			}

			json_reader::DownloadedDataForTransportRouter GetDataForTransportRouter(
				const transport_catalogue_serialize::DataForTransportRouter& proto_data) {

				json_reader::DownloadedDataForTransportRouter data;
				data.guide = GetTransportCatalogue(proto_data.guide());
				data.render_settings = GetRenderSettings(proto_data.render_settings());
				data.routing_settings = GetRoutingSettings(proto_data.routing_settings());
				
				transport_router::TransportRouter::DownloadedData downloaded_data_for_router;
				downloaded_data_for_router.graph = std::make_unique<transport_router::Graph>(GetGraph(proto_data.graph()));
				
				downloaded_data_for_router.data_of_router.reserve(proto_data.data_of_router_size());
				for (int i = 0; i < proto_data.data_of_router_size(); ++i) {
					downloaded_data_for_router.data_of_router.emplace_back(GetArrayRouteInternalData(proto_data.data_of_router(i)));
				}
				for (int i = 0; i < proto_data.stop_id_size(); ++i) {
					const auto [ptr_to_stop, edge_id] = GetPairStopId(proto_data.stop_id(i), data.guide);
					downloaded_data_for_router.stop_id.emplace(ptr_to_stop, edge_id);
				}

				for (int i = 0; i < proto_data.edge_id_data_size(); ++i) {
					const auto [edge_id, data_edge] = GetPairEdgeIdData(proto_data.edge_id_data(i), data.guide);
					downloaded_data_for_router.edge_id_data.emplace(edge_id, data_edge);
				}
				data.data_for_router = std::make_unique<transport_router::TransportRouter::DownloadedData>(std::move(downloaded_data_for_router));
				return data;
			}
		}// namespace detail

		bool SaveDataForTransportRouter(std::ostream& out, const json_reader::DownloadedDataForTransportRouter& data) {
			transport_catalogue_serialize::DataForTransportRouter proto_data = detail::CreateProtoDataForTransportRouter(data);
			return proto_data.SerializeToOstream(&out);
		}

		json_reader::DownloadedDataForTransportRouter LoadDataForTransportRouter(std::istream& ifs) {
			transport_catalogue_serialize::DataForTransportRouter proto_data;
			if (!proto_data.ParseFromIstream(&ifs)) {
				return {};
			}
			return detail::GetDataForTransportRouter(proto_data);
		}

	}// namespace serialization_tr_catalogue
}//namespace transport_directory