#include "json_builder.h"

#include <stdexcept>

namespace json {

	Builder::BaseContext::BaseContext(Builder& builder)
		: builder_(builder) {}

	// ------------------ Builder::DictItemContext ------------------------------------

	Builder::DictItemContext::DictItemContext(Builder& builder)
		: BaseContext(builder) {}

	Builder::KeyItemContext Builder::DictItemContext::Key(std::string key) {
		return builder_.Key(std::move(key));
	}

	Builder& Builder::DictItemContext::EndDict() {
		return builder_.EndDict();
	}

	// ------------------- Builder::StartContainerItemContext ----------------------------

	Builder::StartContainerItemContext::StartContainerItemContext(Builder& builder)
		: BaseContext(builder){}

	Builder::DictItemContext Builder::StartContainerItemContext::StartDict() {
		return builder_.StartDict();
	}

	Builder::ArrayItemContext Builder::StartContainerItemContext::StartArray() {
		return builder_.StartArray();
	}

	// ------------------- Builder::ArrayItemContext -------------------------------------

	Builder::ArrayItemContext::ArrayItemContext(Builder& builder)
		: StartContainerItemContext(builder) {}

	Builder::ArrayItemContext& Builder::ArrayItemContext::Value(Node::Value value) {
		builder_.Value(std::move(value));
		return *this;
	}

	Builder& Builder::ArrayItemContext::EndArray() {
		return builder_.EndArray();
	}

	// ------------------ Builder::KeyItemContext -------------------------------------

	Builder::KeyItemContext::KeyItemContext(Builder& builder)
		: StartContainerItemContext(builder) {}

	Builder::DictItemContext Builder::KeyItemContext::Value(Node::Value value) {
		builder_.Value(std::move(value));
		return DictItemContext(builder_);
	}

	// ------------------- Builder ----------------------------------------------

	Builder::Builder() {}

	Builder& Builder::Value(json::Node::Value value) {
		if (!root_) {
			root_ = Node();
			Node::Value& value_new = root_->GetValue();
			value_new = std::move(value);
		}
		else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
			Node::Value& value_arr = nodes_stack_.back()->GetValue();
			Array& arr = std::get<Array>(value_arr);
			Node& node_new = arr.emplace_back(Node());
			Node::Value& value_new = node_new.GetValue();
			value_new = std::move(value);
		}
		else if (nodes_stack_.size() > 1 && nodes_stack_.back()->IsNull() && nodes_stack_[nodes_stack_.size() - 2]->IsDict()) {
			Node::Value& value_new = nodes_stack_.back()->GetValue();
			value_new = std::move(value);
			nodes_stack_.pop_back();
		}
		else {
			throw std::logic_error("Builder.Value() error");
		}
		return *this;
	}

	Builder::KeyItemContext Builder::Key(std::string key) {
		if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
			throw std::logic_error("json::Builder.Key() error");
		}
		Node::Value& value_dict = nodes_stack_.back()->GetValue();
		Dict& dict = std::get<Dict>(value_dict);
		auto [it, is_add] = dict.emplace(std::move(key), Node());
		if (!is_add) {
			it->second = Node();
		}
		nodes_stack_.push_back(&(it->second));
		return KeyItemContext(*this);
	}

	Builder::DictItemContext Builder::StartDict() {
		if (!AddContainer(Dict())) {
			throw std::logic_error("Builder.StartDict() error");
		}
		return DictItemContext(*this);
	}

	Builder::ArrayItemContext Builder::StartArray() {
		if (!AddContainer(Array())) {
			throw std::logic_error("Builder.StartArray() error");
		}
		return ArrayItemContext(*this);
	}

	Builder& Builder::EndDict() {
		if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
			throw std::logic_error("Builder.EndDict() error");
		}
		nodes_stack_.pop_back();
		return *this;
	}

	Builder& Builder::EndArray() {
		if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
			throw std::logic_error("Builder.EndArray() error");
		}
		nodes_stack_.pop_back();
		return *this;
	}

	Node Builder::Build() {
		if (!root_ || !nodes_stack_.empty()) {
			throw std::logic_error("Builder.Build() error");
		}
		return std::move(*root_);
	}

	bool Builder::AddContainer(Node node) {
		if (!root_) {
			root_ = std::move(node);
			nodes_stack_.push_back(&(*root_));
		}
		else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
			Node::Value& value_arr = nodes_stack_.back()->GetValue();
			Array& arr = std::get<Array>(value_arr);
			Node& node_arr = arr.emplace_back(std::move(node));
			nodes_stack_.push_back(&node_arr);
		}
		else if (nodes_stack_.size() > 1 && nodes_stack_.back()->IsNull() && nodes_stack_[nodes_stack_.size() - 2]->IsDict()) {
			Node::Value& value = nodes_stack_.back()->GetValue();
			value = std::move(node.GetValue());
		}
		else {
			return false;
		}
		return true;
	}

}// namespace json