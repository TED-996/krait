#pragma once

enum WebsocketsOpcode {
	Continuation = 0,
	Text = 1,
	Binary = 2,
	Close = 8,
	Ping = 9,
	Pong = 10
};

struct WebsocketsMessage {
	WebsocketsOpcode opcode;
	std::string message;
};

struct WebsocketsFrame {
	bool isFin;
	WebsocketsOpcode opcode;
	std::string message;
};