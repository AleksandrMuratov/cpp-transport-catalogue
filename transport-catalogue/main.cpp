#include <iostream>
#include <fstream>
#include <string_view>
#include "json_reader.h"

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {

        // make base here
        json::Document doc(json::Load(std::cin));
        transport_directory::json_reader::DownloadedDataForTransportRouter data = transport_directory::json_reader::LoadDataFromJson(doc);
        transport_directory::transport_router::TransportRouter router(data.guide, data.routing_settings);
        data.data_for_router = std::make_unique<transport_directory::transport_router::TransportRouter::DownloadedData>(router.GetDataForTransRouter());
        transport_directory::json_reader::SaveDataToFile(doc, data);
    }
    else if (mode == "process_requests"sv) {

        // process requests here
        json::Document doc(json::Load(std::cin));
        transport_directory::json_reader::DownloadedDataForTransportRouter data = transport_directory::json_reader::LoadDataFromFile(doc);
        transport_directory::json_reader::PrintAnswearsForRequests(doc, data);
    }
    else {
        PrintUsage();
        return 1;
    }
    return 0;
}