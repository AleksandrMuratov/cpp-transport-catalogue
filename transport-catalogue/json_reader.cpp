#include "json_reader.h"
#include "json_builder.h"
#include "serialization.h"

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <variant>
#include <fstream>
#include <filesystem>

namespace transport_directory {
	namespace json_reader {

		void PrintAnswearsForRequests(const json::Document& doc, const DownloadedDataForTransportRouter& data, std::ostream& os) {
			using namespace std::literals;
			const json::Array& requests = doc.GetRoot().AsDict().at("stat_requests"s).AsArray();
			json::Array answears;
			std::string type = "type"s;
			answears.reserve(requests.size());
			std::unique_ptr<transport_router::TransportRouter> router_ptr;
			router_ptr = data.data_for_router ?
				std::make_unique<transport_router::TransportRouter>(data.guide, data.routing_settings, *data.data_for_router)
				: std::make_unique<transport_router::TransportRouter>(data.guide, data.routing_settings);
			for (const auto& node : requests) {
				const auto& request = node.AsDict();
				if (request.at(type).AsString() == "Bus"s) {
					answears.push_back(detail::RequestBusRoute(request, data.guide));
				}
				else if(request.at(type).AsString() == "Stop"s) {
					answears.push_back(detail::RequestBusesForStop(request, data.guide));
				}
				else if (request.at(type).AsString() == "Map"s) {
					answears.push_back(detail::RequestMap(data.render_settings, request, data.guide));
				}
				else if (request.at(type).AsString() == "Route"s) {
					answears.push_back(detail::RequestFindRoute(request, *router_ptr));
				}
			}
			json::Document doc_with_answears(std::move(answears));
			json::Print(doc_with_answears, os);
		}

		void LoadTransportGuide(const json::Document& doc, transport_catalogue::TransportCatalogue& guide) {
			using namespace std::literals;
			std::vector<detail::TransportObject> objects;
			for (const auto& node : doc.GetRoot().AsDict().at("base_requests"s).AsArray()) {
				objects.push_back(detail::ParseRequestToFillTransportGuide(node));
			}

			std::for_each(objects.begin(), objects.end(), [&guide](const detail::TransportObject& tr_obj) {
				if (tr_obj.type == detail::TypeTransportObject::STOP) {
					guide.AddStop(std::string(tr_obj.name), tr_obj.coordinates);
				}
				});
			std::for_each(objects.begin(), objects.end(), [&guide](const detail::TransportObject& tr_obj) {
				if (tr_obj.type == detail::TypeTransportObject::STOP) {
					const domain::Stop* from = guide.SearchStop(tr_obj.name);
					for (const auto& [stop_to, distance] : tr_obj.distances_to) {
						const domain::Stop* to = guide.SearchStop(stop_to);
						guide.SetDistance(from, to, distance);
					}
				}
				});
			std::for_each(objects.begin(), objects.end(), [&guide](detail::TransportObject& tr_obj) {
				if (tr_obj.type == detail::TypeTransportObject::BUS) {
					guide.AddBusRoute(std::string(tr_obj.name), std::move(tr_obj.stops), tr_obj.is_roundtrip);
				}
				});
		}

		svg::Document CreateSvgDocumentMap(const renderer::MapRenderer& renderer, const transport_catalogue::TransportCatalogue& guide) {
			using namespace std::literals;
			std::vector<const domain::BusRoute*> bus_routes;
			for (const auto& bus_route : guide.GetBusRoutes()) {
				bus_routes.push_back(&bus_route);
			}
			std::sort(bus_routes.begin(), bus_routes.end(), [](const domain::BusRoute* lhs, const domain::BusRoute* rhs) {
				return lhs->name < rhs->name;
				});
			const auto array_of_objects = renderer.CreateArrayObjects(bus_routes);
			svg::Document doc_for_draw;
			for (const auto& obj : array_of_objects) {
				obj->Draw(doc_for_draw);
			}
			return doc_for_draw;
		}

		void PrintMapToSvg(const renderer::RenderSettings& render_settings, const transport_catalogue::TransportCatalogue& guide, std::ostream& os) {
			CreateSvgDocumentMap(renderer::MapRenderer(render_settings), guide).Render(os);
		}

		bool SaveDataToFile(const json::Document& doc, const DownloadedDataForTransportRouter& data) {
			using namespace std::literals;
			std::filesystem::path file = doc.GetRoot().AsDict().at("serialization_settings"s).AsDict().at("file").AsString();
			std::ofstream out(file, std::ios::binary);
			if (!out) {
				return false;
			}
			if (serialization_tr_catalogue::SaveDataForTransportRouter(out, data)) {
				return true;
			}
			return false;
		}

		DownloadedDataForTransportRouter LoadDataFromFile(const json::Document& doc) {
			using namespace std::literals;
			std::filesystem::path file = doc.GetRoot().AsDict().at("serialization_settings"s).AsDict().at("file").AsString();
			std::ifstream ifs(file, std::ios::binary);
			DownloadedDataForTransportRouter data;
			if (ifs.good()) {
				data = serialization_tr_catalogue::LoadDataForTransportRouter(ifs);
			}
			return data;
		}

		DownloadedDataForTransportRouter LoadDataFromJson(const json::Document& doc) {
			DownloadedDataForTransportRouter data;
			LoadTransportGuide(doc, data.guide);
			data.render_settings = detail::LoadRenderSettings(doc);
			data.routing_settings = detail::LoadRoutingSettings(doc);
			return data;
		}

		namespace detail {

			transport_router::TransportRouter::RoutingSettings LoadRoutingSettings(const json::Document& doc) {
				using namespace std::literals;
				const json::Dict& settings = doc.GetRoot().AsDict().at("routing_settings"s).AsDict();
				return { static_cast<size_t>(settings.at("bus_wait_time"s).AsInt()), settings.at("bus_velocity"s).AsDouble() };
			}

			json::Node RequestFindRoute(const json::Dict& request, const transport_router::TransportRouter& router) {
				using namespace std::literals;
				auto route_info = router.BuildRoute(request.at("from"s).AsString(), request.at("to"s).AsString());
				int request_id = request.at("id"s).AsInt();
				if (!route_info) {
					return ErrorMessageNotFound(request_id);
				}
				return RouteInfoToJson(*route_info, request_id);
			}

			json::Node RequestBusRoute(const json::Dict& request, const transport_catalogue::TransportCatalogue& guide) {
				using namespace std::literals;
				const auto stat = guide.RequestStatBusRoute(request.at("name"s).AsString());
				return detail::StatToJson(stat, request.at("id"s).AsInt());
			}

			json::Node RequestBusesForStop(const json::Dict& request, const transport_catalogue::TransportCatalogue& guide) {
				using namespace std::literals;
				const auto stat = guide.RequestStatForStop(request.at("name"s).AsString());
				return detail::StatToJson(stat, request.at("id"s).AsInt());
			}

			json::Node RequestMap(const renderer::RenderSettings& render_settings, const json::Dict& request, const transport_catalogue::TransportCatalogue& guide) {
				using namespace std::literals;
				std::ostringstream os;
				json_reader::PrintMapToSvg(render_settings, guide, os);
				return detail::SvgToJson(os.str(), request.at("id"s).AsInt());
			}

			json::Node StatToJson(const transport_catalogue::StatBusRoute& stat, int request_id) {
				using namespace std::literals;
				if (stat.count_stops_) {
					return json::Builder().StartDict()
						.Key("request_id"s).Value(request_id)
						.Key("curvature"s).Value(stat.curvature_)
						.Key("route_length"s).Value(stat.curvature_)
						.Key("route_length"s).Value(stat.route_length_)
						.Key("stop_count"s).Value(static_cast<int>(stat.count_stops_))
						.Key("unique_stop_count"s).Value(static_cast<int>(stat.count_unique_stops_)).EndDict().Build();
				}
				else {
					return ErrorMessageNotFound(request_id);
				}
			}

			json::Node StatToJson(const transport_catalogue::StatForStop& stat, int request_id) {
				using namespace std::literals;
				if (stat.buses_) {
					json::Array arr;
					arr.reserve(stat.buses_->size());
					for (const auto& bus : *stat.buses_) {
						arr.push_back(json::Builder().Value(std::string(bus)).Build());
					}
					return json::Builder().StartDict()
						.Key("request_id"s).Value(request_id)
						.Key("buses"s).Value(std::move(arr))
						.EndDict().Build();
				}
				else {
					return ErrorMessageNotFound(request_id);
				}
			}

			json::Node SvgToJson(std::string svg_str, int request_id) {
				using namespace std::literals;
				return json::Builder().StartDict()
					.Key("request_id"s).Value(request_id)
					.Key("map"s).Value(std::move(svg_str))
					.EndDict().Build();
			}

			json::Node RouteInfoToJson(const transport_router::TransportRouter::RouteInfo& route_info, int request_id) {
				using namespace std::literals;
				json::Array items;
				for (const auto& data_edge : route_info.edges) {
					if (std::holds_alternative<const domain::Stop*>(data_edge.obj)) {
						const domain::Stop* stop = std::get<const domain::Stop*>(data_edge.obj);
						items.push_back(json::Builder().StartDict()
							.Key("type"s).Value("Wait"s)
							.Key("stop_name"s).Value(stop->name)
							.Key("time"s).Value(data_edge.weight)
							.EndDict().Build());
					}
					else {
						const domain::BusRoute* bus = std::get<const domain::BusRoute*>(data_edge.obj);
						items.push_back(json::Builder().StartDict()
							.Key("type"s).Value("Bus"s)
							.Key("bus"s).Value(bus->name)
							.Key("span_count"s).Value(data_edge.spun_count)
							.Key("time"s).Value(data_edge.weight)
							.EndDict().Build());
					}
				}
				return json::Builder().StartDict()
					.Key("request_id"s).Value(request_id)
					.Key("total_time"s).Value(route_info.weight)
					.Key("items"s).Value(std::move(items))
					.EndDict().Build();
			}

			json::Node ErrorMessageNotFound(int request_id) {
				using namespace std::literals;
				return json::Builder().StartDict()
					.Key("request_id"s).Value(request_id)
					.Key("error_message"s).Value("not found"s)
					.EndDict().Build();
			}

			void ParseDistancesToStops(const json::Node& node, TransportObject& obj) {
				for (const auto& [stop, distance] : node.AsDict()) {
					obj.distances_to.emplace_back(stop, distance.AsDouble());
				}
			}

			TransportObject ParseBusObject(const json::Node& node) {
				using namespace std::literals;
				TransportObject obj;
				obj.type = TypeTransportObject::BUS;
				const auto& bus = node.AsDict();
				obj.name = bus.at("name"s).AsString();
				for (const auto& stop : bus.at("stops"s).AsArray()) {
					obj.stops.push_back(stop.AsString());
				}
				if (!(bus.at("is_roundtrip"s).AsBool()) && obj.stops.size() > 1) {
					for (int i = obj.stops.size() - 2; i >= 0; --i) {
						obj.stops.push_back(obj.stops[i]);
					}
				}
				else {
					obj.is_roundtrip = true;
				}
				return obj;
			}

			TransportObject ParseStopObject(const json::Node& node) {
				using namespace std::literals;
				TransportObject obj;
				obj.type = TypeTransportObject::STOP;
				const auto& stop = node.AsDict();
				obj.name = stop.at("name"s).AsString();
				obj.coordinates.lat = stop.at("latitude"s).AsDouble();
				obj.coordinates.lng = stop.at("longitude"s).AsDouble();
				ParseDistancesToStops(stop.at("road_distances"s), obj);
				return obj;
			}

			TransportObject ParseRequestToFillTransportGuide(const json::Node& node) {
				using namespace std::literals;
				const std::string& type = node.AsDict().at("type"s).AsString();
				if (type == "Stop"s) {
					return ParseStopObject(node);
				}
				else if (type == "Bus"s) {
					return ParseBusObject(node);
				}
				else {
					throw std::invalid_argument("Wrong format of request");
				}
				return {};
			}


			svg::Rgba LoadRgba(const json::Node& node) {
				const json::Array arr = node.AsArray();
				return svg::Rgba(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt(), arr[3].AsDouble());
			}

			svg::Rgb LoadRgb(const json::Node& node) {
				const json::Array arr = node.AsArray();
				return svg::Rgb(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt());
			}

			svg::Color LoadColor(const json::Node& node) {
				if (node.IsArray()) {
					if (node.AsArray().size() == 4) {
						return detail::LoadRgba(node);
					}
					else {
						return detail::LoadRgb(node);
					}
				}
				else {
					return node.AsString();
				}
			}

			std::vector<svg::Color> LoadColorPalette(const json::Node& node) {
				std::vector<svg::Color> color_palette;
				const json::Array& arr = node.AsArray();
				color_palette.reserve(arr.size());
				for (const auto& node_color : arr) {
					color_palette.emplace_back(LoadColor(node_color));
				}
				return color_palette;
			}

			renderer::RenderSettings LoadRenderSettings(const json::Document& doc) {
				using namespace std::literals;
				renderer::RenderSettings settings;
				const json::Dict& render_settings = doc.GetRoot().AsDict().at("render_settings"s).AsDict();
				settings.width_ = render_settings.at("width"s).AsDouble();
				settings.height_ = render_settings.at("height"s).AsDouble();
				settings.padding_ = render_settings.at("padding"s).AsDouble();
				settings.line_width_ = render_settings.at("line_width"s).AsDouble();
				settings.stop_radius_ = render_settings.at("stop_radius"s).AsDouble();
				settings.bus_label_font_size_ = render_settings.at("bus_label_font_size"s).AsInt();
				const json::Array& bus_label_offset_array = render_settings.at("bus_label_offset"s).AsArray();
				settings.bus_label_offset_ = { bus_label_offset_array[0].AsDouble(), bus_label_offset_array[1].AsDouble() };
				settings.stop_label_font_size_ = render_settings.at("stop_label_font_size"s).AsInt();
				const json::Array& stop_label_offset_array = render_settings.at("stop_label_offset"s).AsArray();
				settings.stop_label_offset_ = { stop_label_offset_array[0].AsDouble(), stop_label_offset_array[1].AsDouble() };
				settings.underlayer_color_ = detail::LoadColor(render_settings.at("underlayer_color"s));
				settings.underlayer_width_ = render_settings.at("underlayer_width"s).AsDouble();
				settings.color_palette_ = std::move(detail::LoadColorPalette(render_settings.at("color_palette"s)));
				return settings;
			}
		}// namespace detail

	}// namespace json_reader

}// namespace transport_directory
