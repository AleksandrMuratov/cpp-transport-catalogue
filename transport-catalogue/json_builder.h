#pragma once
#include "json.h"
#include <string>
#include <optional>
#include <vector>


namespace json {

	class Builder;
	class KeyItemContext;
	class DictItemConctext;
	class ArrayItemContext;

	class BaseContext {
	protected:
		BaseContext(Builder& builder);
		Builder& builder_;
	};

	class DictItemContext : virtual public BaseContext {
	public:
		DictItemContext(Builder& builder);

		KeyItemContext& Key(std::string key);

		Builder& EndDict();
	};

	class ArrayItemContext : virtual public BaseContext {
	public:
		ArrayItemContext(Builder& builder);

		ArrayItemContext& Value(Node::Value value);

		DictItemContext& StartDict();

		ArrayItemContext& StartArray();

		Builder& EndArray();

	};

	class KeyItemContext : virtual public BaseContext {
	public:
		KeyItemContext(Builder& builder);

		DictItemContext& Value(Node::Value value);

		ArrayItemContext& StartArray();

		DictItemContext& StartDict();
	};


	class Builder : virtual public DictItemContext,
		virtual public ArrayItemContext,
		virtual public KeyItemContext {
	public:

		Builder();

		Builder& Value(Node::Value value);

		DictItemContext& StartDict();

		ArrayItemContext& StartArray();

		json::Node Build();

		Builder& EndDict();

		Builder& EndArray();

		KeyItemContext& Key(std::string key);

	private:
		std::optional<Node> root_;
		std::vector<Node*> nodes_stack_;
	};
}// namespace json