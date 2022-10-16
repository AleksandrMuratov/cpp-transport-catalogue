#include <iostream>

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

using namespace std;

int main() {
	transport_directory::transport_catalogue::TransportCatalogue guide;
	transport_directory::input_reader::LoadTransportGuide(guide);
	transport_directory::stat_reader::LoadRequestStat(guide);
	return 0;
}