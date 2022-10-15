#include <iostream>

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

using namespace std;

int main() {
	transport_directory::transport_catalogue::TransportCatalogue guide;
	transport_directory::input_reader::LoadTransportGuide(guide);
	const auto stats = transport_directory::stat_reader::LoadRequestStat(guide);
	for (const auto& stat : stats) {
		cout << *stat << '\n';
	}
	return 0;
}