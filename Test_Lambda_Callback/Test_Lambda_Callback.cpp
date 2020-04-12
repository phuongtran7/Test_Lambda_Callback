// Test_Lamda_Callback.cpp : Defines the entry point for the application.
//

#include "Test_Lambda_Callback.h"

std::vector<XPLMFlightLoopID> flightloops;
std::vector<std::unique_ptr<Topic>> topics;

void read_initial_config() {
	YAML::Node config = YAML::LoadFile("G:/X-Plane/X-Plane 11/Aircraft/Laminar Research/Stinson L5/plugins/Test_Lambda/Config.yaml");

	auto address = config["Address"].as<std::string>();

	if (config["Publish Topic"])
	{
		auto pub = config["Publish Topic"].as<YAML::Node>();
		for (auto&& item : pub) {
			auto current_topic = item.as<std::string>();
			auto node = config[current_topic].as<YAML::Node>();
			auto temp_topic = std::make_unique<Topic>(address, current_topic, TopicType::PUBLISHER, node);
			topics.push_back(std::move(temp_topic));
		}
	}
	
	if (config["Subscribe Topic"]) 
	{
		auto sub = config["Subscribe Topic"].as<YAML::Node>();
		for (auto&& item : sub) {
			auto current_topic = item.as<std::string>();
			auto node = config[current_topic].as<YAML::Node>();
			auto temp_topic = std::make_unique<Topic>(address, current_topic, TopicType::SUBSCRIBER, node);
			topics.push_back(std::move(temp_topic));
		}
	}
}

PLUGIN_API int XPluginStart(
	char* outName,
	char* outSig,
	char* outDesc)
{
	std::string name = "Test Lambda";
	std::string signature = "x-plane.plugin.phuong.lambda";
	std::string description = "Test Lambda callback.";

	strcpy_s(outName, 256, name.c_str());
	strcpy_s(outSig, 256, signature.c_str());
	strcpy_s(outDesc, 256, description.c_str());

	read_initial_config();

	return 1;
}

PLUGIN_API void	XPluginStop(void) {}

PLUGIN_API void XPluginDisable(void)
{
	for (auto&& topic : topics) {
		topic.reset();
	}

	for (auto&& id : flightloops) {
		XPLMDestroyFlightLoop(id);
	}
}

PLUGIN_API int XPluginEnable(void)
{
	if (topics.empty()) {
		// Don't enable the plugin if there isn't any topic
		return 0;
	}

	// Register flight loop for writing the data
	XPLMCreateFlightLoop_t data_params{ sizeof(XPLMCreateFlightLoop_t), xplm_FlightLoop_Phase_AfterFlightModel,
		[] (float inElapsedSinceLastCall,
			float inElapsedTimeSinceLastFlightLoop,
			int inCounter,
			void* inRefcon) -> float
		{
			auto topic_vector = reinterpret_cast<std::vector<std::unique_ptr<Topic>>*>(inRefcon);
			if (topic_vector) {
				for (auto&& item : *topic_vector) {
					item->Update();
				}
			}
			return -1.0;
		}
	, &topics };

	auto id = XPLMCreateFlightLoop(&data_params);
	if (id == nullptr)
	{
		XPLMDebugString("Cannot create flight loop. Exiting.\n");
		return 0;
	}
	else {
		XPLMScheduleFlightLoop(id, -1.0f, true);
	}

	flightloops.push_back(std::move(id));

	return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) {}
