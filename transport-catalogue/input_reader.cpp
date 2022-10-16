#include "input_reader.h"
#include <stdexcept>
#include <algorithm>

namespace transport_directory {
    namespace input_reader {

        void LoadTransportGuide(transport_catalogue::TransportCatalogue& guide, std::istream& is) {
            int n;
            is >> n;
            std::string input_str;
            std::vector<std::string> requests;
            std::getline(is, input_str); //skip '\n'
            for (int i = 0; i < n; ++i) {
                std::getline(is, input_str);
                requests.push_back(std::move(input_str));
            }
            std::vector<detail::TransportObject> objects;
            for (const auto& request : requests) {
                objects.push_back(detail::ParseRequestToFillTransportGuide(request));
            }
            std::for_each(objects.begin(), objects.end(), [&guide](const detail::TransportObject& tr_obj) {
                if (tr_obj.type == detail::TypeTransportObject::STOP) {
                    guide.AddStop(std::string(tr_obj.name), tr_obj.coordinates);
                }
                });
            std::for_each(objects.begin(), objects.end(), [&guide](const detail::TransportObject& tr_obj) {
                if (tr_obj.type == detail::TypeTransportObject::STOP) {
                    const transport_catalogue::TransportCatalogue::Stop* from = guide.SearchStop(tr_obj.name);
                    for (const auto& [stop_to, distance] : tr_obj.distances_to) {
                        const transport_catalogue::TransportCatalogue::Stop* to = guide.SearchStop(stop_to);
                        guide.SetDistance(from, to, distance);
                    }
                }
                });
            std::for_each(objects.begin(), objects.end(), [&guide](detail::TransportObject& tr_obj) {
                if (tr_obj.type == detail::TypeTransportObject::BUS) {
                    guide.AddBusRoute(std::string(tr_obj.name), std::move(tr_obj.stops));
                }
                });
        }
        namespace detail {
            std::string_view RStrip(std::string_view str, char by) {
                size_t pos = str.find_last_not_of(by);
                if (pos == std::string::npos) {
                    return std::string_view();
                }
                str.remove_suffix(str.size() - pos - 1);
                return str;
            }

            std::string_view LStrip(std::string_view str, char by) {
                size_t pos = str.find_first_not_of(by);
                if (pos == std::string::npos) {
                    return std::string_view();
                }
                str.remove_prefix(pos);
                return str;
            }

            std::string_view LRStrip(std::string_view str, char by) {
                str = LStrip(str, by);
                str = RStrip(str, by);
                return str;
            }

            std::pair<std::string_view, std::string_view> Split(std::string_view line, char by) {
                size_t pos = line.find(by);
                std::string_view left = line.substr(0, pos);
                left = RStrip(left, ' ');
                if (pos < line.size() && pos + 1 < line.size()) {
                    std::string_view right = line.substr(pos + 1);
                    right = LStrip(right, ' ');
                    return { left, right };
                }
                else {
                    return { left, std::string_view() };
                }
            }

            std::vector<std::string> SplitInToStops(std::string_view line) {
                std::vector<std::string> result;
                line = LRStrip(line, ' ');
                char separating_symbol = '>';
                size_t pos = line.find(separating_symbol);
                if (pos == std::string::npos) {
                    separating_symbol = '-';
                }
                while (!line.empty()) {
                    auto [left, right] = Split(line, separating_symbol);
                    result.push_back(std::string(left));
                    line = right;
                }
                if (separating_symbol == '-') {
                    int pos = result.size() - 2;
                    while (pos >= 0) {
                        result.push_back(result[pos--]);
                    }
                }
                return result;
            }

            void ParseDistancesToStops(std::string_view str, TransportObject& tr_obj) {
                while (!str.empty()) {
                    auto [distance, right] = Split(str, 'm');
                    distance = LStrip(distance, ' ');
                    right = LStrip(right, ' ');
                    auto [_, right2] = Split(right, ' ');
                    auto [stop, right3] = Split(right2, ',');
                    stop = LRStrip(stop, ' ');
                    tr_obj.distances_to.emplace_back(stop, std::stoi(std::string(distance)));
                    str = right3;
                }
            }

            TransportObject ParseStopObject(std::string_view str) {
                TransportObject tr_obj;
                tr_obj.type = TypeTransportObject::STOP;
                str = LRStrip(str, ' ');
                auto [name_stop, right] = Split(str, ':');
                name_stop = RStrip(name_stop, ' ');
                tr_obj.name = name_stop;
                right = LStrip(right, ' ');
                auto [latitude, right2] = Split(right, ',');
                latitude = RStrip(latitude, ' ');
                auto [longitude, distances_to_stops] = Split(right2, ',');
                longitude = LRStrip(longitude, ' ');
                tr_obj.coordinates.lat = std::stod(std::string(latitude));
                tr_obj.coordinates.lng = std::stod(std::string(longitude));
                ParseDistancesToStops(distances_to_stops, tr_obj);
                return tr_obj;
            }

            TransportObject ParseBusObject(std::string_view str) {
                TransportObject tr_obj;
                tr_obj.type = TypeTransportObject::BUS;
                str = LRStrip(str, ' ');
                auto [name_bus, str_of_stops] = Split(str, ':');
                name_bus = RStrip(name_bus, ' ');
                tr_obj.name = name_bus;
                tr_obj.stops = std::move(SplitInToStops(str_of_stops));
                return tr_obj;
            }

            TransportObject ParseRequestToFillTransportGuide(const std::string& str) {
                using namespace std::literals;
                std::string_view str_view = LRStrip(str, ' ');
                auto [type, right] = Split(str_view, ' ');
                if (type == "Stop"s) {
                    return ParseStopObject(right);
                }
                else if (type == "Bus"s) {
                    return ParseBusObject(right);
                }
                else {
                    throw std::invalid_argument("Wrong format of request");
                }
                return {};
            }
        }//end namespace detail
    }//end namespace input_reader
}//end namespace transport_directory