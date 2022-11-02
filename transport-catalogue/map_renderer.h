#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <memory>

namespace renderer {

	struct RenderSettings {

        using OffSet = svg::Point;

		double width_;
		double height_;
		double padding_;
		double line_width_;
		double stop_radius_;
		int bus_label_font_size_;
		OffSet bus_label_offset_;
		int stop_label_font_size_;
		OffSet stop_label_offset_;
		svg::Color underlayer_color_;
		double underlayer_width_;
		std::vector<svg::Color> color_palette_;
	};

	class Route : public svg::Drawable {
	public:
		Route(const std::vector<svg::Point>& stops, svg::Color stroke_color, double stroke_width);

		void Draw(svg::ObjectContainer& container) const override;

	private:
		svg::Polyline route_;
	};

    class NameOfRoutes : public svg::Drawable {
    public:
        NameOfRoutes(const svg::Point& pos, const svg::Point& offset, int font_size,
            const std::string& name_bus, svg::Color underlayer_color, double underlayer_width, svg::Color text_color);

        void Draw(svg::ObjectContainer& container) const override;

    private:
        svg::Text underlayer_;
        svg::Text text_;
    };

    class CirclForStop : public svg::Drawable {
    public:
        CirclForStop(const svg::Point& center, double radius);

        void Draw(svg::ObjectContainer& container) const override;

    private:
        svg::Circle circle_;
    };

    class NameOfStops : public svg::Drawable {
    public:
        NameOfStops(const svg::Point& pos, const svg::Point& offset, int font_size,
            const std::string& name_stop, svg::Color underlayer_color, double underlayer_width);

        void Draw(svg::ObjectContainer& container) const override;

    private:
        svg::Text underlayer_;
        svg::Text text_;
    };

    inline const double EPSILON = 1e-6;

    class SphereProjector {
    public:
        // points_begin � points_end ������ ������ � ����� ��������� ��������� geo::Coordinates
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
            double max_width, double max_height, double padding);

        // ���������� ������ � ������� � ���������� ������ SVG-�����������
        svg::Point operator()(const geo::Coordinates& coords) const;

        static bool IsZero(double value) {
            return std::abs(value) < EPSILON;
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    class MapRenderer {
    public:
        MapRenderer(const RenderSettings& settings);

        std::vector<std::unique_ptr<svg::Drawable>> CreateArrayObjects(std::vector<const transport_directory::domain::BusRoute*> bus_routes) const;
       
    private:
        RenderSettings settings_;
    };


    template <typename PointInputIt>
    SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding)
        : padding_(padding)
    {
        // ���� ����� ����������� ����� �� ������, ��������� ������
        if (points_begin == points_end) {
            return;
        }

        // ������� ����� � ����������� � ������������ ��������
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // ������� ����� � ����������� � ������������ �������
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // ��������� ����������� ��������������� ����� ���������� x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // ��������� ����������� ��������������� ����� ���������� y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // ������������ ��������������� �� ������ � ������ ���������,
            // ���� ����������� �� ���
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            // ����������� ��������������� �� ������ ���������, ���������� ���
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            // ����������� ��������������� �� ������ ���������, ���������� ���
            zoom_coeff_ = *height_zoom;
        }
    }

}// namespace renderer
