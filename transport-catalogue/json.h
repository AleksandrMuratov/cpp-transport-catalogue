#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

    class Node;
    // Сохраните объявления Dict и Array без изменения
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;
    using Number = std::variant<int, double>;
    // Эта ошибка должна выбрасываться при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node {
    public:
        /* Реализуйте Node, используя std::variant */
        using Value = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;

        Node() = default;
        Node(Array array);
        Node(Dict map);
        Node(int value);
        Node(std::string value);
        Node(double value);
        Node(bool value);
        Node(nullptr_t);

        const Array& AsArray() const;
        const Dict& AsMap() const;
        int AsInt() const;
        const std::string& AsString() const;
        bool AsBool() const;
        double AsDouble() const;

        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;

        const Value& GetValue() const;

    private:
        Value value_;
    };

    namespace detail {
        using std::literals::operator ""s;
        void Skip(std::istream& input, std::string skip_char = " \n\t\r"s);

        Node LoadNode(std::istream& input);

        Number LoadNumber(std::istream& input);

        std::string LoadString(std::istream& input);

        Node LoadArray(std::istream& input);

        Node LoadDigit(std::istream& input);

        Node LoadNodeString(std::istream& input);

        Node LoadDict(std::istream& input);

        Node LoadBool(std::istream& input);

        Node LoadNull(std::istream& input);



        struct PrintContext {
            std::ostream& out;
            int indent_step = 4;
            int indent = 0;

            void PrintIndent() const;

            // Возвращает новый контекст вывода с увеличенным смещением
            PrintContext Indented() const;
        };

        void PrintNode(const Node& node, const PrintContext& ctx);

        void PrintValue(int value, const PrintContext& ctx);

        void PrintValue(double value, const PrintContext& ctx);

        void PrintValue(std::nullptr_t, const PrintContext& ctx);

        void PrintValue(const std::string& str, const PrintContext& ctx);

        void PrintValue(const Array& array, const PrintContext& ctx);

        void PrintValue(const Dict& dict, const PrintContext& ctx);

        void PrintValue(bool value, const PrintContext& ctx);
    } // namespace detail

    // Контекст вывода, хранит ссылку на поток вывода и текущий отсуп

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

}  // namespace json

bool operator==(const json::Node& lhs, const json::Node& rhs);

bool operator==(const json::Array& lhs, const json::Array& rhs);

bool operator==(const json::Dict& lhs, const json::Dict& rhs);

bool operator!=(const json::Node& lhs, const json::Node& rhs);

bool operator==(const json::Document& lhs, const json::Document& rhs);

bool operator!=(const json::Document& lhs, const json::Document& rhs);