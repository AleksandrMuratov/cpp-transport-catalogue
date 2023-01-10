#pragma once
#include "json_reader.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "router.h"
#include "map_renderer.h"
#include "svg.h"
#include "graph.h"

#include <transport_catalogue.pb.h>
#include <transport_router.pb.h>
#include <map_renderer.pb.h>
#include <svg.pb.h>
#include <graph.pb.h>

#include <iostream>



namespace transport_directory {
	namespace serialization_tr_catalogue {

		namespace detail {

			//----------------------------------- Create Protobuf-object from TransportCatalogue ---------------------------------

			struct BusRoute {
				std::string name;
				std::vector<std::string> stops;
				bool is_roundtrip;
			};

			struct DistanceBetweenStops {
				std::string_view from;
				std::string_view to;
				int distance;
			};

			transport_catalogue_serialize::Stop CreateProtoStop(const domain::Stop& stop);
			transport_catalogue_serialize::BusRoute CreateProtoBusRoute(const domain::BusRoute& bus_route,
				const std::unordered_map<std::string_view, size_t>& stops_indexes);
			transport_catalogue_serialize::Distance CreateProtoDistance(const std::pair<const domain::Stop*, const domain::Stop*>& stops,
				int distance, const std::unordered_map<std::string_view, size_t>& stops_indexes);
			transport_catalogue_serialize::TransportCatalogue CreateProtoTransportCatalogue(const transport_catalogue::TransportCatalogue& guide);

			//--------------------------------- Get TransportCatalogue from Protobuf-object --------------------------------------

			std::vector<BusRoute> GetBusRoutes(const transport_catalogue_serialize::TransportCatalogue& proto_guide);
			std::vector<domain::Stop> GetStops(const transport_catalogue_serialize::TransportCatalogue& proto_guide);
			std::vector<DistanceBetweenStops> GetDistances(const transport_catalogue_serialize::TransportCatalogue& proto_guide);
			transport_catalogue::TransportCatalogue GetTransportCatalogue(const transport_catalogue_serialize::TransportCatalogue& proto_guide);

			//-------------------------------- Create Protobuf-object from RenderSettings ----------------------------------------

			transport_catalogue_serialize::Color CreateProtoColor(const svg::Color& color);
			transport_catalogue_serialize::Offset CreateProtoOffset(const renderer::RenderSettings::OffSet& offset);
			transport_catalogue_serialize::RenderSettings CreateProtoRenderSettings(const renderer::RenderSettings& render_settings);

			//-------------------------------- Get RenderSettings from Prtotobuf-object -------------------------------------------

			svg::Rgb GetRgb(const transport_catalogue_serialize::Rgb& proto_rgb);
			svg::Rgba GetRgba(const transport_catalogue_serialize::Rgba& proto_rgba);
			svg::Color GetColor(const transport_catalogue_serialize::Color& proto_color);
			renderer::RenderSettings::OffSet GetOffset(const transport_catalogue_serialize::Offset& proto_offset);
			renderer::RenderSettings GetRenderSettings(const transport_catalogue_serialize::RenderSettings& proto_render_settings);

			//------------------------------- Create Protobuf-object from RoutingSettings ------------------------------------------

			transport_catalogue_serialize::RoutingSettings CreateProtoRoutingSettings(
				const transport_router::TransportRouter::RoutingSettings& routing_settings);

			//------------------------------- Get RoutingSettings from Protobuf-object ---------------------------------------------

			transport_router::TransportRouter::RoutingSettings GetRoutingSettings(
				const transport_catalogue_serialize::RoutingSettings proto_routing_settings);

			//------------------------------ Create Protobuf-object from Graph -----------------------------------------------------

			transport_catalogue_serialize::Edge CreateProtoEdge(const graph::Edge<transport_router::Weight>& edge);
			transport_catalogue_serialize::Graph CreateProtoGraph(const transport_router::Graph& graph);

			//------------------------------ Get Graph from Protobuf-object -------------------------------------------------------

			graph::Edge<transport_router::Weight> GetEdge(const transport_catalogue_serialize::Edge& proto_edge);
			transport_router::Graph GetGraph(const transport_catalogue_serialize::Graph& proto_graph);

			//------------------------------ Create Protobuf-objects from TransporRouter::DownloadedData -----------------------------

			transport_catalogue_serialize::RouteInternalData CreateProtoRouteInternalData(
				const graph::Router<transport_router::Weight>::RouteInternalData& route_internal_data);
			transport_catalogue_serialize::ArrayRouteInternalData CreateProtoArrayRouteInternalData(
				std::vector<std::optional<graph::Router<transport_router::Weight>::RouteInternalData>> array_route_internal_data);
			transport_catalogue_serialize::StopId CreateProtoPairStopId(
				const std::pair<const domain::Stop*, transport_router::EdgeId>& stop_id,
				const transport_catalogue::TransportCatalogue& guide);
			transport_catalogue_serialize::DataEdge CreateProtoDataEdge(const transport_router::TransportRouter::DataEdge& data_edge, 
				const transport_catalogue::TransportCatalogue& guide);
			transport_catalogue_serialize::EdgeIdData CreateProtoPairEdgeIdData(
				const std::pair<transport_router::EdgeId, transport_router::TransportRouter::DataEdge>& edge_id_data, 
				const transport_catalogue::TransportCatalogue& guide);
			
			//----------------------------- Get TransporRouter::DownloadedData from Protobuf_objects -----------------------------------
			
			graph::Router<transport_router::Weight>::RouteInternalData GetRouteInternalData(
				const transport_catalogue_serialize::RouteInternalData& proto_route_internal_data);
			std::vector<std::optional<graph::Router<transport_router::Weight>::RouteInternalData>> GetArrayRouteInternalData(
				const transport_catalogue_serialize::ArrayRouteInternalData& proto_array_route_internal_data);
			std::pair<const domain::Stop*, transport_router::EdgeId> GetPairStopId(transport_catalogue_serialize::StopId proto_stop_id,
				const transport_catalogue::TransportCatalogue& guide);
			transport_router::TransportRouter::DataEdge GetDataEdge(const transport_catalogue_serialize::DataEdge& proto_data_edge,
				const transport_catalogue::TransportCatalogue& guide);
			std::pair<transport_router::EdgeId, transport_router::TransportRouter::DataEdge> GetPairEdgeIdData(
				const transport_catalogue_serialize::EdgeIdData& proto_edge_id_data, const transport_catalogue::TransportCatalogue& guide);

			//--------------------------- Create general Protobuf-object from DownloadedDataForTransportRouter --------------------------

			transport_catalogue_serialize::DataForTransportRouter CreateProtoDataForTransportRouter(
				const json_reader::DownloadedDataForTransportRouter& data);

			//--------------------------- Get DownloadedDataForTransportRouter from Protobuf-object
			
			json_reader::DownloadedDataForTransportRouter GetDataForTransportRouter(
				const transport_catalogue_serialize::DataForTransportRouter& proto_data);

		}//namespace detail

		bool SaveDataForTransportRouter(std::ostream& out, const json_reader::DownloadedDataForTransportRouter& data);

		transport_directory::json_reader::DownloadedDataForTransportRouter LoadDataForTransportRouter(std::istream& ifs);

	}// namespace serialization_tr_catalogue
}// namespace transport_directory