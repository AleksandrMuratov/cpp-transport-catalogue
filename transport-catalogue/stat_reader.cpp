#include "stat_reader.h"
#include "input_reader.h"
#include <string>
#include <string_view>
#include <vector>

namespace transport_directory {
	namespace stat_reader {

		void LoadRequestStat(transport_directory::transport_catalogue::TransportCatalogue& guide, std::istream& is, std::ostream& os) {
			using namespace std::literals;
			int n;
			is >> n;
			std::string input_str;
			std::string type_transport_object;
			while (n > 0) {
				is >> type_transport_object;
				if (type_transport_object == "Bus"s) {
					std::getline(is, input_str);
					std::string_view name_bus = input_reader::detail::LRStrip(input_str, ' ');
					os << *(guide.RequestStatBusRoute(name_bus)) << '\n';
				}
				else if (type_transport_object == "Stop"s) {
					std::getline(is, input_str);
					std::string_view name_stop = input_reader::detail::LRStrip(input_str, ' ');
					os << *(guide.RequestStatForStop(name_stop)) << '\n';
				}
				--n;
			}
		}

	}//end namespace stat_reader
}//end namespace transport_directory