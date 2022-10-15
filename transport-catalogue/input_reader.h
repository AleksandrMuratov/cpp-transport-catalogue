#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <vector>
#include "transport_catalogue.h"

namespace transport_directory {
	namespace input_reader {

		void LoadTransportGuide(transport_catalogue::TransportCatalogue& guide, std::istream& is = std::cin);

		namespace detail {
			enum class TypeTransportObject {
				BUS,
				STOP
			};

			struct TransportObject {
				TypeTransportObject type;
				std::string_view name;
				std::vector<std::string> stops;
				Coordinates coordinates;
				std::vector<std::pair<std::string_view, int>> distances_to;
			};

			std::string_view RStrip(std::string_view str, char by);

			std::string_view LStrip(std::string_view str, char by);

			std::string_view LRStrip(std::string_view str, char by);

			std::pair<std::string_view, std::string_view> Split(std::string_view line, char by);

			std::vector<std::string> SplitInToStops(std::string_view line);

			TransportObject ParseRequestToFillTransportGuide(const std::string& str);
		}//end namespace detail
	}//end namespace input_reader
}//end namespace transport_directory