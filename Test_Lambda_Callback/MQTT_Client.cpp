#include "MQTT_Client.h"

void MQTT_Client::initialize()
{
	try {
		//conn_options_.set_keep_alive_interval(20);
		conn_options_.set_clean_session(true);
		client_->set_callback(*callback_);
		client_->connect(conn_options_)->wait();
	}
	catch (const mqtt::exception& exc) {
		XPLMDebugString(fmt::format("Ditto: Initialize error: {}\n", exc.what()).c_str());
	}
}

MQTT_Client::MQTT_Client(std::string address, std::string topic, int qos) :
	address_(std::move(address)),
	topic_(std::move(topic)),
	qos_(qos),
	client_(std::make_shared<mqtt::async_client>(address_, "")), // Force random clientID
	conn_options_{},
	buffer_(nullptr),
	callback_(std::make_shared<action_callback>(*client_, conn_options_, topic_)),
	publish_listener_(std::make_shared<publish_listener>())
{
	initialize();
}

MQTT_Client::MQTT_Client(std::string address, std::string topic, int qos, std::shared_ptr<synchronized_value<std::string>> buffer) :
	address_(std::move(address)),
	topic_(std::move(topic)),
	qos_(qos),
	client_(std::make_shared<mqtt::async_client>(address_, "")), // Force random clientID
	conn_options_{},
	buffer_(std::move(buffer)),
	callback_(std::make_shared<action_callback>(*client_, conn_options_, topic_, buffer_)),
	publish_listener_(nullptr)
{
	initialize();
}

MQTT_Client::~MQTT_Client()
{
	if (client_) {
		try {
			client_->unsubscribe(topic_)->wait();
			client_->stop_consuming();
			client_->disconnect()->wait();
			callback_.reset();
			buffer_.reset();
			publish_listener_.reset();
		}
		catch (const mqtt::exception& exc) {
			XPLMDebugString(fmt::format("Ditto: {}\n", exc.what()).c_str());
		}
	}
}

MQTT_Client::MQTT_Client(MQTT_Client&& other) noexcept :
	address_(std::exchange(other.address_, {})),
	topic_(std::exchange(other.topic_, {})),
	qos_(std::exchange(other.qos_, 0)),
	client_(std::move(other.client_)),
	conn_options_(std::move(other.conn_options_)),
	buffer_(std::exchange(other.buffer_, {})),
	callback_(std::exchange(other.callback_, nullptr)),
	publish_listener_(std::exchange(other.publish_listener_, nullptr))
{
}

MQTT_Client& MQTT_Client::operator=(MQTT_Client&& other) noexcept
{
	std::swap(address_, other.address_);
	std::swap(topic_, other.topic_);
	std::swap(qos_, other.qos_);

	if (client_) {
		client_.reset();
	}
	std::swap(client_, other.client_);

	std::swap(conn_options_, other.conn_options_);

	if (buffer_) {
		buffer_.reset();
	}
	std::swap(buffer_, other.buffer_);

	if (callback_) {
		callback_.reset();
	}
	std::swap(callback_, other.callback_);

	if (publish_listener_) {
		publish_listener_.reset();
	}
	std::swap(publish_listener_, other.publish_listener_);

	return *this;
}

void MQTT_Client::send_message(const std::string& message)
{
	if (client_->is_connected()) {
		try {
			mqtt::message_ptr pubmsg = mqtt::make_message(topic_, message.c_str(), message.size(), qos_, false);
			client_->publish(pubmsg, nullptr, *publish_listener_);
		}
		catch (const mqtt::exception& ex) {
			XPLMDebugString(fmt::format("Ditto: Publisher send failed: {}\n", ex.get_message()).c_str());
		}
	}
}

void MQTT_Client::send_message(const std::vector<uint8_t>& message)
{
	if (client_->is_connected()) {
		try {
			mqtt::message_ptr pubmsg = mqtt::make_message(topic_, message.data(), message.size(), qos_, false);
			client_->publish(pubmsg, nullptr, *publish_listener_);
		}
		catch (const mqtt::exception& ex) {
			XPLMDebugString(fmt::format("Ditto: Publisher send failed: {}\n", ex.get_message()).c_str());
		}
	}
}

void subscribe_listener::on_failure(const mqtt::token& tok)
{
	XPLMDebugString(fmt::format("Ditto: Subscribe failure").c_str());
	if (tok.get_message_id() != 0) {
		XPLMDebugString(fmt::format(" for token [{}].\n", tok.get_message_id()).c_str());
	}
	auto topic = tok.get_topics();
	if (topic && !topic->empty()) {
		XPLMDebugString(fmt::format("Ditto: Token topic: {}\n", (*topic)[0]).c_str());
	}
}

void subscribe_listener::on_success(const mqtt::token& tok)
{
	XPLMDebugString(fmt::format("Ditto: Subscribe success").c_str());
	if (tok.get_message_id() != 0) {
		XPLMDebugString(fmt::format(" for token [{}]\n", tok.get_message_id()).c_str());
	}
	auto topic = tok.get_topics();
	if (topic && !topic->empty()) {
		XPLMDebugString(fmt::format("Ditto: Token topic: {}\n", (*topic)[0]).c_str());
	}
}

void publish_listener::on_failure(const mqtt::token& tok)
{
	XPLMDebugString(fmt::format("Ditto: Publish failure").c_str());
	if (tok.get_message_id() != 0) {
		XPLMDebugString(fmt::format(" for token [{}].\n", tok.get_message_id()).c_str());
	}
	auto topic = tok.get_topics();
	if (topic && !topic->empty()) {
		XPLMDebugString(fmt::format("Ditto: Token topic: {}\n", (*topic)[0]).c_str());
	}
}

void publish_listener::on_success(const mqtt::token& tok)
{
	// Don't need to log every success publish message for now

	//XPLMDebugString(fmt::format("Ditto: Publish success").c_str());
	//if (tok.get_message_id() != 0) {
	//	XPLMDebugString(fmt::format(" for token [{}]\n", tok.get_message_id()).c_str());
	//}
	//auto topic = tok.get_topics();
	//if (topic && !topic->empty()) {
	//	XPLMDebugString(fmt::format("Ditto: Token topic: {}\n", (*topic)[0]).c_str());
	//}
}

void action_callback::reconnect()
{
	try {
		mqtt::token_ptr token = cli_.connect(connOpts_, nullptr, *this);
	}
	catch (const mqtt::exception& exc) {
		XPLMDebugString(fmt::format("Ditto: Error: {}\n", exc.what()).c_str());
	}
}

void action_callback::on_failure(const mqtt::token& tok)
{
	XPLMDebugString(fmt::format("Ditto: Connection attempt failed.\n").c_str());
	reconnect();
}

void action_callback::on_success(const mqtt::token& tok)
{
}

void action_callback::connected(const std::string& cause)
{
	XPLMDebugString(fmt::format("Ditto: Connection success.\n").c_str());
	if (buffer_) {
		// Subscriber
		XPLMDebugString(fmt::format("Ditto: Subscribing to: {}\n", topic_).c_str());
		mqtt::token_ptr token = cli_.subscribe(topic_, 0, nullptr, *subscribe_listener_);
	}
	else {
		// Publisher
		XPLMDebugString(fmt::format("Ditto: Publishing to: {}\n", topic_).c_str());
	}
}

void action_callback::connection_lost(const std::string& cause)
{
	XPLMDebugString(fmt::format("Ditto: Connection lost.\n").c_str());
	if (!cause.empty()) {
		XPLMDebugString(fmt::format("Cause: {}\n", cause).c_str());
	}
	XPLMDebugString(fmt::format("Ditto: Reconnecting.\n").c_str());
	reconnect();
}

void action_callback::message_arrived(mqtt::const_message_ptr msg)
{
	if (buffer_) {
		apply([new_val = msg->get_payload_str()](std::string& val) mutable {
			val = std::move(new_val);
		}, * buffer_);
	}
}

void action_callback::delivery_complete(mqtt::delivery_token_ptr tok)
{
	XPLMDebugString(fmt::format("Ditto: Message delivered").c_str());
	if (tok->get_message_id() != 0) {
		XPLMDebugString(fmt::format(" for token [{}]\n", tok->get_message_id()).c_str());
	}
}

action_callback::action_callback(mqtt::async_client& cli,
	mqtt::connect_options& connOpts,
	std::string topic,
	std::shared_ptr<synchronized_value<std::string>> buffer) :
		cli_(cli),
		connOpts_(connOpts),
		subscribe_listener_(std::make_shared<subscribe_listener>()),
		topic_(std::move(topic)),
		buffer_(std::move(buffer))
{
}

action_callback::action_callback(mqtt::async_client& cli,
	mqtt::connect_options& connOpts,
	std::string topic) :
		cli_(cli),
		connOpts_(connOpts),
		subscribe_listener_(nullptr),
		topic_(std::move(topic)),
		buffer_(nullptr)
{
}
