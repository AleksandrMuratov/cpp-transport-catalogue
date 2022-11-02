#include "json_reader.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace transport_directory {
	namespace json_reader {

		void PrintAnswearsForRequests(const json::Document& doc, transport_catalogue::TransportCatalogue guide, std::ostream& os) {
			using namespace std::literals;
			const json::Array& requests = doc.GetRoot().AsMap().at("stat_requests"s).AsArray();
			json::Array answears;
			answears.reserve(requests.size());
			for (const auto& node : requests) {
				const auto& request = node.AsMap();
				if (request.at("type"s).AsString() == "Bus"s) {
					answears.push_back(detail::StatToJson(guide.RequestStatBusRoute(request.at("name"s).AsString()), request.at("id"s).AsInt()));
				}
				else if(request.at("type"s).AsString() == "Stop"s) {
					answears.push_back(detail::StatToJson(guide.RequestStatForStop(request.at("name"s).AsString()), request.at("id"s).AsInt()));
				}
				else if (request.at("type"s).AsString() == "Map"s) {
					std::ostringstream os;
					json_reader::PrintMapToSvg(doc, guide, os);
					answears.push_back(detail::SvgToJson(os.str(), request.at("id"s).AsInt()));
				}
			}
			json::Document doc_with_answears(std::move(answears));
			json::Print(doc_with_answears, os);
		}

		void LoadTransportGuide(const json::Document& doc, transport_catalogue::TransportCatalogue& guide) {
			using namespace std::literals;
			std::vector<detail::TransportObject> objects;
			for (const auto& node : doc.GetRoot().AsMap().at("base_requests"s).AsArray()) {
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

		renderer::MapRenderer CreateRenderer(const json::Document& doc) {
			renderer::RenderSettings settings(detail::LoadSettings(doc));
			return renderer::MapRenderer(settings);
		}

		svg::Document CreateSvgDocumentMap(const renderer::MapRenderer& renderer, transport_catalogue::TransportCatalogue guide) {
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

		void PrintMapToSvg(const json::Document& doc, transport_catalogue::TransportCatalogue guide, std::ostream& os) {
			CreateSvgDocumentMap(CreateRenderer(doc), guide).Render(os);
		}

		namespace detail {

			json::Node StatToJson(const transport_catalogue::StatBusRoute& stat, int request_id) {
				using namespace std::literals;
				json::Dict dict;
				dict["request_id"s] = json::Node(request_id);
				if (stat.count_stops_) {
					dict["curvature"s] = json::Node(stat.curvature_);
					dict["route_length"s] = json::Node(stat.route_length_);
					dict["stop_count"s] = json::Node(static_cast<int>(stat.count_stops_));
					dict["unique_stop_count"s] = json::Node(static_cast<int>(stat.count_unique_stops_));
				}
				else {
					dict["error_message"s] = json::Node("not found"s);
				}
				return json::Node(std::move(dict));
			}

			json::Node StatToJson(const transport_catalogue::StatForStop& stat, int request_id) {
				using namespace std::literals;
				json::Dict dict;
				dict["request_id"s] = json::Node(request_id);
				if (stat.buses_) {
					json::Array arr;
					arr.reserve(stat.buses_->size());
					for (const auto& bus : *stat.buses_) {
						arr.push_back(json::Node(std::string(bus)));
					}
					dict["buses"s] = json::Node(std::move(arr));
				}
				else {
					dict["error_message"s] = json::Node("not found"s);
				}
				return json::Node(std::move(dict));
			}

			json::Node SvgToJson(std::string svg_str, int request_id) {
				using namespace std::literals;
				json::Dict dict;
				dict["request_id"s] = json::Node(request_id);
				dict["map"s] = json::Node(svg_str);
				return json::Node(dict);
			}

			void ParseDistancesToStops(const json::Node& node, TransportObject& obj) {
				using namespace std::literals;
				for (const auto& [stop, distance] : node.AsMap()) {
					obj.distances_to.emplace_back(stop, distance.AsDouble());
				}
			}

			TransportObject ParseBusObject(const json::Node& node) {
				using namespace std::literals;
				TransportObject obj;
				obj.type = TypeTransportObject::BUS;
				const auto& bus = node.AsMap();
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
				const auto& stop = node.AsMap();
				obj.name = stop.at("name"s).AsString();
				obj.coordinates.lat = stop.at("latitude"s).AsDouble();
				obj.coordinates.lng = stop.at("longitude"s).AsDouble();
				ParseDistancesToStops(stop.at("road_distances"s), obj);
				return obj;
			}

			TransportObject ParseRequestToFillTransportGuide(const json::Node& node) {
				using namespace std::literals;
				const std::string& type = node.AsMap().at("type"s).AsString();
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

			renderer::RenderSettings LoadSettings(const json::Document& doc) {
				using namespace std::literals;
				renderer::RenderSettings settings;
				const json::Dict& render_settings = doc.GetRoot().AsMap().at("render_settings"s).AsMap();
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
