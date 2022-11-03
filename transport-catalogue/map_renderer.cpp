#include "map_renderer.h"
#include <map>

namespace renderer {

	RenderRoute::RenderRoute(const std::vector<svg::Point>& stops, svg::Color stroke_color, double stroke_width) {
		for (const auto& point : stops) {
			route_.AddPoint(point);
		}
		route_.SetStrokeColor(stroke_color).SetStrokeWidth(stroke_width)
			.SetFillColor({}).SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
	}

	void RenderRoute::Draw(svg::ObjectContainer& container) const {
		container.Add(route_);
	}

    RenderNameOfRoutes::RenderNameOfRoutes(const svg::Point& pos, const svg::Point& offset, int font_size,
        const std::string& name_bus, svg::Color underlayer_color, double underlayer_width, svg::Color text_color) {
        using namespace std::literals;
        underlayer_.SetPosition(pos).SetOffset(offset).SetFontSize(font_size)
            .SetFontFamily("Verdana"s).SetFontWeight("bold"s).SetData(name_bus)
            .SetFillColor(underlayer_color).SetStrokeColor(underlayer_color)
            .SetStrokeWidth(underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        text_.SetPosition(pos).SetOffset(offset).SetFontSize(font_size)
            .SetFontFamily("Verdana"s).SetFontWeight("bold"s).SetData(name_bus)
            .SetFillColor(text_color);
    }

    void RenderNameOfRoutes::Draw(svg::ObjectContainer& container) const {
        container.Add(underlayer_);
        container.Add(text_);
    }

    RenderCirclForStop::RenderCirclForStop(const svg::Point& center, double radius) {
        using namespace std::literals;
        circle_.SetCenter(center).SetRadius(radius).SetFillColor("white"s);
    }

    void RenderCirclForStop::Draw(svg::ObjectContainer& container) const {
        container.Add(circle_);
    }

    RenderNameOfStops::RenderNameOfStops(const svg::Point& pos, const svg::Point& offset, int font_size,
        const std::string& name_stop, svg::Color underlayer_color, double underlayer_width) {
        using namespace std::literals;
        underlayer_.SetPosition(pos).SetOffset(offset).SetFontSize(font_size)
            .SetFontFamily("Verdana"s).SetData(name_stop)
            .SetFillColor(underlayer_color).SetStrokeColor(underlayer_color)
            .SetStrokeWidth(underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        text_.SetPosition(pos).SetOffset(offset).SetFontSize(font_size)
            .SetFontFamily("Verdana"s).SetData(name_stop)
            .SetFillColor("black"s);
    }

    void RenderNameOfStops::Draw(svg::ObjectContainer& container) const {
        container.Add(underlayer_);
        container.Add(text_);
    }

	svg::Point SphereProjector::operator()(const geo::Coordinates& coords) const {
		return {
			(coords.lng - min_lon_) * zoom_coeff_ + padding_,
			(max_lat_ - coords.lat) * zoom_coeff_ + padding_
		};
	}

    MapRenderer::MapRenderer(const RenderSettings& settings)
        : settings_(settings) {}

    std::vector<std::unique_ptr<svg::Drawable>> MapRenderer::CreateArrayObjects(std::vector<const transport_directory::domain::BusRoute*> bus_routes) const {
        std::map<std::string_view, transport_directory::domain::Stop*> stops_for_draw;
        std::vector<geo::Coordinates> coors;
        for (const auto& bus_route : bus_routes) {
            for (const auto& stop : bus_route->route) {
                if (stops_for_draw.insert({ stop->name, stop }).second) {
                    coors.push_back(stop->coordinates);
                }
            }
        }

        SphereProjector proj(coors.begin(), coors.end(), settings_.width_, settings_.height_, settings_.padding_);
        std::vector<std::unique_ptr<svg::Drawable>> result;
        std::vector<std::unique_ptr<svg::Drawable>> names_of_routes;
        std::vector<std::unique_ptr<svg::Drawable>> circles_for_stops;
        std::vector<std::unique_ptr<svg::Drawable>> names_of_stops;
        
        int index_color_palette = 0;
        result.reserve(bus_routes.size());
        for (const auto& bus_route : bus_routes) {
            std::vector<svg::Point> points;
            points.reserve(bus_route->route.size());
            for (const auto& stop : bus_route->route) {
                points.push_back(proj(stop->coordinates));
            }
            result.emplace_back(std::make_unique<RenderRoute>(points, settings_.color_palette_[index_color_palette], settings_.line_width_));

            names_of_routes.emplace_back(std::make_unique<RenderNameOfRoutes>(points[0], settings_.bus_label_offset_,
                settings_.bus_label_font_size_, bus_route->name, settings_.underlayer_color_,
                settings_.underlayer_width_, settings_.color_palette_[index_color_palette]));

            if (!(bus_route->is_roundtrip) && (bus_route->route[0] != bus_route->route[static_cast<int>(bus_route->route.size()) / 2])) {
                names_of_routes.emplace_back(std::make_unique<RenderNameOfRoutes>(points[static_cast<int>(bus_route->route.size()) / 2], 
                    settings_.bus_label_offset_, settings_.bus_label_font_size_, bus_route->name, settings_.underlayer_color_,
                    settings_.underlayer_width_, settings_.color_palette_[index_color_palette]));
            }
            ++index_color_palette %= static_cast<int>(settings_.color_palette_.size());
        }
        for (const auto& [name_stop, stop] : stops_for_draw) {
            svg::Point stop_center = proj(stop->coordinates);
            circles_for_stops.push_back(std::make_unique<RenderCirclForStop>(stop_center, settings_.stop_radius_));
            names_of_stops.push_back(std::make_unique<RenderNameOfStops>(stop_center, settings_.stop_label_offset_,
                settings_.stop_label_font_size_, stop->name, settings_.underlayer_color_, settings_.underlayer_width_));
        }
        for (auto& name_of_route : names_of_routes) {
            result.push_back(std::move(name_of_route));
        }
        for (auto& circle_for_stop : circles_for_stops) {
            result.push_back(std::move(circle_for_stop));
        }
        for (auto& name_of_stop : names_of_stops) {
            result.push_back(std::move(name_of_stop));
        }
        return result;
    }
}// namespace renderer