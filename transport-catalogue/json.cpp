#include "json.h"
#include <cctype>

using namespace std;

namespace json {
    namespace detail {

        void Skip(istream& input, std::string skip_char) {
            for (char c; input >> c;) {
                if (skip_char.find(c) == std::string::npos) {
                    input.putback(c);
                    break;
                }
            }
        }

        Number LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            }
            else {
                read_digits();
            }

            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return std::stoi(parsed_num);
                    }
                    catch (...) {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return std::stod(parsed_num);
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        // Считывает содержимое строкового литерала JSON-документа
        // Функцию следует использовать после считывания открывающего символа ":
        Node LoadString(std::istream& input) {
            using namespace std::literals;

            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    // Поток закончился до того, как встретили закрывающую кавычку?
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    // Встретили закрывающую кавычку
                    ++it;
                    break;
                }
                else if (ch == '\\') {
                    // Встретили начало escape-последовательности
                    ++it;
                    if (it == end) {
                        // Поток завершился сразу после символа обратной косой черты
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                    switch (escaped_char) {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        // Встретили неизвестную escape-последовательность
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    // Строковый литерал внутри- JSON не может прерываться символами \r или \n
                    throw ParsingError("Unexpected end of line"s);
                }
                else {
                    // Просто считываем очередной символ и помещаем его в результирующую строку
                    s.push_back(ch);
                }
                ++it;
            }

            return s;
        }

        Node LoadArray(istream& input) {
            Array result;
            Skip(input);
            char c;
            for (; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
                Skip(input);
            }
            if (c != ']') {
                throw ParsingError("not valid node Array");
            }
            return result;
        }

        Node LoadDigit(istream& input) {
            const auto number = LoadNumber(input);
            if (std::holds_alternative<double>(number)) {
                return std::get<double>(number);
            }
            return std::get<int>(number);
        }

        Node LoadDict(istream& input) {
            Dict result;
            Skip(input);
            char c;
            for (; input >> c && c != '}';) {
                if (c == ',') {
                    Skip(input);
                    input >> c;
                }
                string key = std::get<std::string>(LoadString(input));
                Skip(input);
                input >> c;
                result.insert({ move(key), LoadNode(input) });
                Skip(input);
            }
            if (c != '}') {
                throw ParsingError("not valid node Dict");
            }

            return result;
        }

        Node LoadBool(istream& input) {
            char c;
            input >> c;
            std::string end_char = " ,\n\t\r}]"s;
            bool error = false;
            if (c == 't') {
                std::string true_ = "t"s;
                for (int i = 0; i < 3 && input >> c; ++i) {
                    true_ += c;
                }
                if (true_ == "true"s && ((input >> c && end_char.find(c) != std::string::npos) || !input)) {
                    input.putback(c);
                    return true;
                }
                else {
                    error = true;
                }
            }
            else if (c == 'f') {
                std::string false_ = "f"s;
                for (int i = 0; i < 4 && input >> c; ++i) {
                    false_ += c;
                }
                if (false_ == "false"s && ((input >> c && end_char.find(c) != std::string::npos) || !input)) {
                    input.putback(c);
                    return false;
                }
                else {
                    error = true;
                }
            }
            else {
                error = true;
            }
            if (error) {
                throw ParsingError("not valid node Bool");
            }
            return Node();
        }

        Node LoadNull(istream& input) {
            char c;
            input >> c;
            std::string end_char = " ,\n\t\r}]"s;
            if (c == 'n') {
                std::string nul = "n"s;
                for (int i = 0; i < 3 && input >> c; ++i) {
                    nul += c;
                }
                if (nul == "null"s && ((input >> c && end_char.find(c) != std::string::npos) || !input)) {
                    return Node();
                }
                else {
                    throw ParsingError("not valid node Null");
                }
            }
            throw ParsingError("not valid node Null");
            return Node();
        }

        Node LoadNode(istream& input) {
            Skip(input);
            char c;
            input >> c;
            if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if (std::isdigit(c) || c == '-') {
                input.putback(c);
                return LoadDigit(input);
            }
            else if (c == 't' || c == 'f') {
                input.putback(c);
                return LoadBool(input);
            }
            else if (c == 'n') {
                input.putback(c);
                return LoadNull(input);
            }
            else {
                throw ParsingError("not valid node");
            }
            return Node();
        }

        void PrintContext::PrintIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        PrintContext PrintContext::Indented() const {
            return { out, indent_step, indent_step + indent };
        }

        void PrintNode(const Node& node, const PrintContext& ctx) {
            std::visit(
                [&ctx](const auto& value) { PrintValue(value, ctx); },
                node.GetValue());
        }

        // Перегрузка функции PrintValue для вывода значений null
        void PrintValue(std::nullptr_t, const PrintContext& ctx) {
            ctx.out << "null"sv;
        }
        void PrintValue(int value, const PrintContext& ctx) {
            ctx.out << value;
        }

        void PrintValue(double value, const PrintContext& ctx) {
            ctx.out << value;
        }

        // Другие перегрузки функции PrintValue пишутся аналогично
        void PrintValue(const std::string& value, const PrintContext& ctx) {
            ctx.out << "\"";
            for (const char c : value) {
                if (c == '\n') {
                    ctx.out << "\\n";
                }
                else if (c == '\r') {
                    ctx.out << "\\r";
                }
                else if (c == '"') {
                    ctx.out << "\\\"";
                }
                else if (c == '\t') {
                    ctx.out << "\t";
                }
                else if (c == '\\') {
                    ctx.out << "\\\\";
                }
                else {
                    ctx.out << c;
                }
            }
            ctx.out << "\"";
        }

        void PrintValue(const Array& array, const PrintContext& ctx) {
            ctx.out << '\n';
            ctx.PrintIndent();
            ctx.out << "[\n";
            bool is_first = true;
            auto ctx2 = ctx.Indented();
            for (const auto& node : array) {
                if (!is_first) {
                    ctx.out << ",\n";
                }
                ctx2.PrintIndent();
                PrintNode(node, ctx2);
                is_first = false;
            }
            ctx.out << '\n';
            ctx.PrintIndent();
            ctx.out << "]\n";
        }

        void PrintValue(const Dict& dict, const PrintContext& ctx) {
            ctx.out << '\n';
            ctx.PrintIndent();
            ctx.out << "{\n";
            auto ctx2 = ctx.Indented();
            bool is_first = true;
            for (const auto& [key, value] : dict) {
                if (!is_first) {
                    ctx.out << ",\n";
                }
                ctx2.PrintIndent();
                PrintValue(key, ctx);
                ctx.out << ':';
                PrintNode(value, ctx2);
                is_first = false;
            }
            ctx.out << '\n';
            ctx.PrintIndent();
            ctx.out << "}\n";
        }

        void PrintValue(bool value, const PrintContext& ctx) {
            ctx.out << (value ? "true" : "false");
        }

    }  // namespace detail

    bool Node::IsInt() const {
        return std::holds_alternative<int>(*this);
    }
    bool Node::IsDouble() const {
        return std::holds_alternative<int>(*this) || std::holds_alternative<double>(*this);
    }
    bool Node::IsPureDouble() const {
        return std::holds_alternative<double>(*this);
    }
    bool Node::IsBool() const {
        return std::holds_alternative<bool>(*this);
    }
    bool Node::IsString() const {
        return std::holds_alternative<std::string>(*this);
    }
    bool Node::IsNull() const {
        return std::holds_alternative<nullptr_t>(*this);
    }
    bool Node::IsArray() const {
        return std::holds_alternative<Array>(*this);
    }
    bool Node::IsMap() const {
        return std::holds_alternative<Dict>(*this);
    }

    const Array& Node::AsArray() const {
        if (!IsArray()) {
            throw std::logic_error("logic_error not array");
        }
        return std::get<Array>(*this);
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) {
            throw std::logic_error("logic_error not map");
        }
        return std::get<Dict>(*this);
    }

    int Node::AsInt() const {
        if (!IsInt()) {
            throw std::logic_error("logic_error not int");
        }
        return std::get<int>(*this);
    }

    const std::string& Node::AsString() const {
        if (!IsString()) {
            throw std::logic_error("logic_error not string");
        }
        return std::get<std::string>(*this);
    }

    bool Node::AsBool() const {
        if (!IsBool()) {
            throw std::logic_error("logic_error not bool");
        }
        return std::get<bool>(*this);
    }

    double Node::AsDouble() const {
        double result;
        if (IsInt()) {
            result = std::get<int>(*this);
        }
        else if (IsDouble()) {
            result = std::get<double>(*this);
        }
        else {
            throw std::logic_error("logic_error not double or int");
        }
        return result;
    }

    const std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>& Node::GetValue() const {
        return *this;
    }

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ detail::LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        detail::PrintNode(doc.GetRoot(), { output });
    }

    bool operator==(const Node& lhs, const Node& rhs) {
        return lhs.GetValue() == rhs.GetValue();
    }

    bool operator!=(const Node& lhs, const Node& rhs) {
        return !(lhs == rhs);
    }

    bool operator==(const json::Document& lhs, const json::Document& rhs) {
        return lhs.GetRoot() == rhs.GetRoot();
    }

    bool operator!=(const json::Document& lhs, const json::Document& rhs) {
        return !(lhs == rhs);
    }

}  // namespace json

