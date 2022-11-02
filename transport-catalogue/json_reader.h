#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <string_view>

#include "geo.h"
#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "svg.h"

namespace transport_directory {
	namespace json_reader {

		void PrintAnswearsForRequests(const json::Document& doc, transport_catalogue::TransportCatalogue guide, std::ostream& os = std::cout);

		void LoadTransportGuide(const json::Document& doc, transport_catalogue::TransportCatalogue& guide);

		renderer::MapRenderer CreateRenderer(const json::Document& doc);

		svg::Document CreateSvgDocumentMap(const renderer::MapRenderer& renderer, transport_catalogue::TransportCatalogue guide);

		void PrintMapToSvg(const json::Document& doc, transport_catalogue::TransportCatalogue guide, std::ostream& os = std::cout);

		namespace detail {

			json::Node StatToJson(const transport_catalogue::StatBusRoute& stat, int request_id);

			json::Node StatToJson(const transport_catalogue::StatForStop& stat, int request_id);

			json::Node SvgToJson(std::string svg_str, int request_id);

			enum class TypeTransportObject {
				BUS,
				STOP
			};

			struct TransportObject {
				TypeTransportObject type;
				std::string name;
				std::vector<std::string> stops;
				geo::Coordinates coordinates;
				std::vector<std::pair<std::string, int>> distances_to;
				bool is_roundtrip = false;
			};

			void ParseDistancesToStops(const json::Node& node, TransportObject& obj);

			TransportObject ParseBusObject(const json::Node& node);

			TransportObject ParseStopObject(const json::Node& node);

			TransportObject ParseRequestToFillTransportGuide(const json::Node& doc);

			svg::Rgba LoadRgba(const json::Node& node);
			svg::Rgb LoadRgb(const json::Node& node);
			svg::Color LoadColor(const json::Node& node);
			std::vector<svg::Color> LoadColorPalette(const json::Node& node);
			renderer::RenderSettings LoadSettings(const json::Document& doc);
		}// namespace detail

	}// namespace json_reader
}// namespace transport_directory
