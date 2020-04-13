#include "Topic.h"

void Topic::init()
{
	// Prepare datarefs
	read_config();

	switch (type_)
	{
	case TopicType::PUBLISHER: {
		flexbuffers_builder_ = std::make_unique<flexbuffers::Builder>();
		client_ = std::make_unique<MQTT_Client>(address_, topic_, 0);
		break;
	}
	case TopicType::SUBSCRIBER: {
		buffer_ = std::make_shared<synchronized_value<std::string>>();
		client_ = std::make_unique<MQTT_Client>(address_, topic_, 0, buffer_);
		break;
	}
	default:
		break;
	}
}

void Topic::read_config()
{
	// Each of dataref in the list is a map with value is a array of nested key-value pairs
	for (auto&& data : config_) {

		DatarefInfo dataref{};

		// Unfortunately, it seems that we don't have a simple way to get the map name
		// So instead of iterate over all the map keys, this will grab the first key
		// which guaranteed as the map name in our config.
		dataref.name = data.begin()->first.as<std::string>();

		auto node_value = data.begin()->second.as<YAML::Node>();

		dataref.dataref = XPLMFindDataRef(node_value["dataref"].as<std::string>().c_str());

		auto type = node_value["type"].as<std::string>();
		if (type == "string") {
			dataref.type = DatarefType::STRING;
		}
		else if (type == "int") {
			dataref.type = DatarefType::INT;
		}
		else if (type == "float") {
			dataref.type = DatarefType::FLOAT;
		}
		else if (type == "double") {
			dataref.type = DatarefType::DOUBLE;
		}

		if (data["start"]) {
			dataref.start_index = data["start"].as<int>();
		}
		if (data["end"]) {
			dataref.num_value = data["num_value"].as<int>();
		}

		dataref_list_.emplace_back(std::move(dataref));
	}
}

void Topic::send_data()
{
	const auto map_start = flexbuffers_builder_->StartMap();

	for (const auto& dataref : dataref_list_) {
		switch (dataref.type) {
		case DatarefType::INT: {
			if (dataref.start_index.has_value()) {
				// If start index exist then it's an array
				auto int_num = get_value<std::vector<int>>(dataref);
				if (2 <= dataref.num_value.value() && dataref.num_value.value() <= 4) {
					flexbuffers_builder_->FixedTypedVector(dataref.name.c_str(),
						int_num.data(),
						dataref.num_value.value());
				}
				else {
					flexbuffers_builder_->TypedVector(dataref.name.c_str(), [&] {
						for (auto i : int_num) {
							flexbuffers_builder_->Int(i);
						}
						});
				}
			}
			else {
				// Just single value
				auto return_value = get_value<int>(dataref);
				flexbuffers_builder_->Int(dataref.name.c_str(), return_value);
			}
			break;
		}
		case DatarefType::FLOAT: {
			if (dataref.start_index.has_value()) {
				auto float_num = get_value<std::vector<float>>(dataref);
				if (2 <= dataref.num_value.value() && dataref.num_value.value() <= 4) {
					flexbuffers_builder_->FixedTypedVector(dataref.name.c_str(),
						float_num.data(),
						dataref.num_value.value());
				}
				else {
					flexbuffers_builder_->TypedVector(dataref.name.c_str(), [&] {
						for (auto i : float_num) {
							flexbuffers_builder_->Float(i);
						}
						});
				}
			}
			else {
				flexbuffers_builder_->Float(dataref.name.c_str(), get_value<float>(dataref));
			}
			break;
		}
		case DatarefType::DOUBLE: {
			flexbuffers_builder_->Double(dataref.name.c_str(), get_value<double>(dataref));
			break;
		}
		case DatarefType::STRING: {
			flexbuffers_builder_->String(dataref.name.c_str(), get_value<std::string>(dataref).c_str());
			break;
		}
		default:
			break;
		}
	}
	flexbuffers_builder_->EndMap(map_start);
	flexbuffers_builder_->Finish();

	client_->send_message(flexbuffers_builder_->GetBuffer());
	flexbuffers_builder_->Clear();
}

void Topic::read_data()
{
	// Currently Not Implemented
}


Topic::Topic(const std::string& address, const std::string& topic, TopicType type, YAML::Node& config) :
	address_(address),
	topic_(topic),
	buffer_{ nullptr },
	client_{ nullptr },
	type_(type),
	config_(config),
	dataref_list_{},
	flexbuffers_builder_{ nullptr }
{
	init();
}

Topic::~Topic()
{
	client_.reset();
	buffer_.reset();
	dataref_list_.clear();
	flexbuffers_builder_.reset();
}

Topic::Topic(Topic&& other) noexcept :
	address_(std::move(other.address_)),
	topic_(std::move(other.topic_)),
	buffer_(std::move(other.buffer_)),
	client_(std::move(other.client_)),
	type_(std::move(other.type_)),
	config_(std::move(other.config_)),
	dataref_list_(std::move(other.dataref_list_)),
	flexbuffers_builder_(std::move(other.flexbuffers_builder_))
{
	init();
}

Topic& Topic::operator=(Topic&& other) noexcept
{
	std::swap(address_, other.address_);
	std::swap(topic_, other.topic_);
	std::swap(buffer_, other.buffer_);
	std::swap(client_, other.client_);
	std::swap(type_, other.type_);
	std::swap(config_, other.config_);
	std::swap(dataref_list_, other.dataref_list_);
	std::swap(flexbuffers_builder_, other.flexbuffers_builder_);
	return *this;
}

void Topic::Update()
{
	switch (type_)
	{
	case TopicType::PUBLISHER:
	{
		send_data();
		break;
	}
	case TopicType::SUBSCRIBER:
	{
		read_data();
		break;
	}
	default:
		break;
	}
}
