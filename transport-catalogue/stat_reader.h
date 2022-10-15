#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include "transport_catalogue.h"

namespace transport_directory {
	namespace stat_reader {

		std::vector<std::shared_ptr<transport_directory::transport_catalogue::Statistics>> LoadRequestStat(transport_directory::transport_catalogue::TransportCatalogue& guide, std::istream& is = std::cin);

	}//end namespace stat_reader
}//end namespace transport_directory