#include "json_builder.h"
#include <stdexcept>

namespace json {

	BaseContext::BaseContext(Builder& builder) : builder_(builder) {}

	// ------------------ DictItemContext ------------------------------------

	DictItemContext::DictItemContext(Builder& builder) : BaseContext(builder) {}

	KeyItemContext& DictItemContext::Key(std::string key) {
		return builder_.Key(std::move(key));
	}

	Builder& DictItemContext::EndDict() {
		return builder_.EndDict();
	}

	// ------------------- ArrayItemContext -------------------------------------

	ArrayItemContext::ArrayItemContext(Builder& builder) : BaseContext(builder) {}

	ArrayItemContext& ArrayItemContext::Value(Node::Value value) {
		return builder_.Value(std::move(value));
	}

	DictItemContext& ArrayItemContext::StartDict() {
		return builder_.StartDict();
	}

	ArrayItemContext& ArrayItemContext::StartArray() {
		return builder_.StartArray();
	}

	Builder& ArrayItemContext::EndArray() {
		return builder_.EndArray();
	}

	// ------------------ KeyItemContext -------------------------------------

	KeyItemContext::KeyItemContext(Builder& builder) : BaseContext(builder) {}

	DictItemContext& KeyItemContext::Value(Node::Value value) {
		return builder_.Value(std::move(value));
	}

	ArrayItemContext& KeyItemContext::StartArray() {
		return builder_.StartArray();
	}

	DictItemContext& KeyItemContext::StartDict() {
		return builder_.StartDict();
	}

	// ------------------- Builder ----------------------------------------------

	Builder::Builder() :BaseContext(*this), DictItemContext(*this), ArrayItemContext(*this), KeyItemContext(*this) {}

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

	KeyItemContext& Builder::Key(std::string key) {
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
		return *this;
	}

	DictItemContext& Builder::StartDict() {
		if (!root_) {
			root_ = Dict();
			nodes_stack_.push_back(&(*root_));
		}
		else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
			Node::Value& value_arr = nodes_stack_.back()->GetValue();
			Array& arr = std::get<Array>(value_arr);
			Node& node_dict = arr.emplace_back(Dict());
			nodes_stack_.push_back(&node_dict);
		}
		else if (nodes_stack_.size() > 1 && nodes_stack_.back()->IsNull() && nodes_stack_[nodes_stack_.size() - 2]->IsDict()) {
			Node::Value& value = nodes_stack_.back()->GetValue();
			value = Dict();
		}
		else {
			throw std::logic_error("Builder.StartDict() error");
		}
		return *this;
	}

	ArrayItemContext& Builder::StartArray() {
		if (!root_) {
			root_ = Array();
			nodes_stack_.push_back(&(*root_));
		}
		else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
			Node::Value& value_arr = nodes_stack_.back()->GetValue();
			Array& arr = std::get<Array>(value_arr);
			Node& node_arr = arr.emplace_back(Array());
			nodes_stack_.push_back(&node_arr);
		}
		else if (nodes_stack_.size() > 1 && nodes_stack_.back()->IsNull() && nodes_stack_[nodes_stack_.size() - 2]->IsDict()) {
			Node::Value& value = nodes_stack_.back()->GetValue();
			value = Array();
		}
		else {
			throw std::logic_error("Builder.StartArray() error");
		}
		return *this;
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


}// namespace json