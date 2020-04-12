cmake_minimum_required (VERSION 3.15)

add_library(Test_Lamda_Callback SHARED "Test_Lamda_Callback.cpp" "Test_Lamda_Callback.h"
	"MQTT_Client.cpp" "Topic.cpp")

set_target_properties(Test_Lamda_Callback PROPERTIES CXX_STANDARD 17)
target_compile_definitions(Test_Lamda_Callback PRIVATE IBM=1 XPLM200 XPLM210 XPLM300 XPLM301)
set_target_properties(Test_Lamda_Callback PROPERTIES SUFFIX ".xpl")

find_package(fmt CONFIG REQUIRED)
find_package(PahoMqttCpp CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

# Due to the fact that the lib file doesn't comfort to the standard naming convention
# So we have to find and link with absolute path
find_library(XP_LIBRARY XPLM_64)

target_link_libraries(Test_Lamda_Callback PRIVATE 
		fmt::fmt fmt::fmt-header-only
		PahoMqttCpp::paho-mqttpp3
		yaml-cpp
		${XP_LIBRARY})
