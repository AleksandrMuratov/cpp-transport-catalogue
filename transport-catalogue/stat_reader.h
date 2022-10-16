#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include "transport_catalogue.h"

namespace transport_directory {
	namespace stat_reader {

		void LoadRequestStat(transport_directory::transport_catalogue::TransportCatalogue& guide, std::istream& is = std::cin, std::ostream& os = std::cout);

	}//end namespace stat_reader
}//end namespace transport_directory