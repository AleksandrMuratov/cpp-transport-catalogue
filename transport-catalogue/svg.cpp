#include "svg.h"

namespace svg {

    using namespace std::literals;

    Rgb::Rgb(unsigned int r, unsigned int g, unsigned int b) :
        red(r),
        green(g),
        blue(b) {}

    Rgba::Rgba(unsigned int r, unsigned int g, unsigned int b, double o) :
        red(r),
        green(g),
        blue(b),
        opacity(o) {}

    PrintColor::PrintColor(std::ostream& stream) : os(stream) {}

    void PrintColor::operator()(std::monostate) const {
        os << "none";
    }
    void PrintColor::operator()(const std::string& color) const {
        os << color;
    }
    void PrintColor::operator()(const Rgb& rgb) const {
        os << "rgb(" << static_cast<int>(rgb.red) << ',' << static_cast<int>(rgb.green) << ',' << static_cast<int>(rgb.blue) << ')';
    }
    void PrintColor::operator()(const Rgba& rgba) const {
        os << "rgba(" << static_cast<int>(rgba.red) << ',' << static_cast<int>(rgba.green) << ',' << static_cast<int>(rgba.blue) << ',' << rgba.opacity << ')';
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(context.out);
        out << " />"sv;
    }

    //--------- Polyline --------------------
    Polyline& Polyline::AddPoint(Point point) {
        polyline_.push_back(std::move(point));
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\"";
        bool is_first = true;
        for (const auto& point : polyline_) {
            if (!is_first) {
                out << ' ';
            }
            out << point.x << ',' << point.y;
            is_first = false;
        }
        out << "\"";
        RenderAttrs(context.out);
        out << "/>";
    }

    //------------ Text --------------------
    Text& Text::SetPosition(Point pos) {
        pos_ = pos;
        return *this;
    }

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    // Задаёт размеры шрифта (атрибут font-size)
    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    // Задаёт название шрифта (атрибут font-family)
    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& Text::SetData(std::string data) {
        text_ = std::move(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text x=\"" << pos_.x << "\" y=\"" << pos_.y << "\""
            << " dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\""
            << " font-size=\"" << font_size_ << "\"";
        if (!font_family_.empty()) {
            out << " font-family=\"" << font_family_ << "\"";
        }
        if (!font_weight_.empty()) {
            out << " font-weight=\"" << font_weight_ << "\"";
        }
        RenderAttrs(context.out);
        out << ">" << text_ << "</text>";
    }

    //-------------- Document ---------------

    // Добавляет в svg-документ объект-наследник svg::Object
    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

    // Выводит в ostream svg-представление документа
    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << '\n'
            << "<svg xmlns=\"http://www.w3.org/2000/svg" << "\" version = \"1.1\">" << '\n';
        for (const auto& obj : objects_) {
            obj->Render(RenderContext(out, 2, 2));
        }
        out << "</svg>";
    }

}  // namespace svg

std::ostream& operator<<(std::ostream& os, const svg::StrokeLineCap& line_cap) {
    if (line_cap == svg::StrokeLineCap::BUTT) {
        os << "butt";
    }
    else if (line_cap == svg::StrokeLineCap::ROUND) {
        os << "round";
    }
    else if (line_cap == svg::StrokeLineCap::SQUARE) {
        os << "square";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const svg::StrokeLineJoin& line_join) {
    if (line_join == svg::StrokeLineJoin::ARCS) {
        os << "arcs";
    }
    else if (line_join == svg::StrokeLineJoin::BEVEL) {
        os << "bevel";
    }
    else if (line_join == svg::StrokeLineJoin::MITER) {
        os << "miter";
    }
    else if (line_join == svg::StrokeLineJoin::MITER_CLIP) {
        os << "miter-clip";
    }
    else if (line_join == svg::StrokeLineJoin::ROUND) {
        os << "round";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const svg::Color& color) {
    std::visit(svg::PrintColor(os), color);
    return os;
}