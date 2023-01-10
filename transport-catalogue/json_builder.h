#pragma once
#include "json.h"

#include <string>
#include <optional>
#include <vector>


namespace json {

	class Builder {
	public:

		class KeyItemContext;
		class DictItemContext;
		class ArrayItemContext;

		class BaseContext {
		protected:
			BaseContext(Builder& builder);

			Builder& builder_;
		};

		class DictItemContext : public BaseContext {
		public:
			DictItemContext(Builder& builder);

			KeyItemContext Key(std::string key);

			Builder& EndDict();

		};

		class StartContainerItemContext: public BaseContext {
		public:
			StartContainerItemContext(Builder& builder);

			DictItemContext StartDict();

			ArrayItemContext StartArray();
		};

		class ArrayItemContext : public StartContainerItemContext {
		public:
			ArrayItemContext(Builder& builder);

			ArrayItemContext& Value(Node::Value value);

			Builder& EndArray();

		};

		class KeyItemContext : public StartContainerItemContext {
		public:
			KeyItemContext(Builder& builder);

			DictItemContext Value(Node::Value value);

		};

		Builder();

		Builder& Value(Node::Value value);

		DictItemContext StartDict();

		ArrayItemContext StartArray();

		json::Node Build();

		Builder& EndDict();

		Builder& EndArray();

		KeyItemContext Key(std::string key);

	private:

		bool AddContainer(Node node);

		std::optional<Node> root_;
		std::vector<Node*> nodes_stack_;
	};

}// namespace json