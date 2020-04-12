#pragma once
#include "XPLMDataAccess.h"
#include <string>
#include <optional>

enum class TopicType
{
	PUBLISHER,
	SUBSCRIBER
};

enum class DatarefType {
	STRING,
	INT,
	FLOAT,
	DOUBLE
};

struct DatarefInfo {
	std::string name{}; // Name user defined for the dataref
	XPLMDataRef dataref{};
	DatarefType type{};
	std::optional<int> start_index{};
	std::optional<int> num_value{}; // Number of values in the array to get; starts at start_index
};
