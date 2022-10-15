#include "stat_reader.h"
#include "input_reader.h"
#include <string>
#include <string_view>
#include <vector>

namespace transport_directory {
	namespace stat_reader {

		std::vector<std::shared_ptr<transport_directory::transport_catalogue::Statistics>> LoadRequestStat(transport_directory::transport_catalogue::TransportCatalogue& guide, std::istream& is) {
			using namespace std::literals;
			std::vector<std::shared_ptr<transport_directory::transport_catalogue::Statistics>> stats;
			int n;
			is >> n;
			std::string input_str;
			std::string type_transport_object;
			while (n > 0) {
				is >> type_transport_object;
				if (type_transport_object == "Bus"s) {
					std::getline(is, input_str);
					std::string_view name_bus = input_reader::detail::LRStrip(input_str, ' ');
					stats.push_back(guide.RequestStatBusRoute(name_bus));
				}
				else if (type_transport_object == "Stop"s) {
					std::getline(is, input_str);
					std::string_view name_stop = input_reader::detail::LRStrip(input_str, ' ');
					stats.push_back(guide.RequestStatForStop(name_stop));
				}
				--n;
			}
			return stats;
		}

	}//end namespace stat_reader
}//end namespace transport_directory