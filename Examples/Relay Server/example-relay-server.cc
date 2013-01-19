#include <iostream>

#include "Relay.hpp"

bool OnConnect(Lacewing::Relay::Server &Server, Lacewing::Relay::Server::Client &Client);
void OnError(Lacewing::Relay::Server &Server, Lacewing::Error &Error);

int main(unsigned nargs, const char const * const * args)
{
	lacewing::_eventpump Pump;
	lacewing::relay::_server server (Pump);
	server.onConnect(OnConnect);
	server.onError(OnError);
	server.Host(6121);
	lw_server lwsz;
	lwsz->

	std::cout << "Server hosting on port: " << server.port() << "..." << std::endl;
	Pump.start_eventloop();

	return 0;
}

bool OnConnect(Lacewing::Relay::Server &Server, Lacewing::Relay::Server::Client &Client)
{
	std::cout << "Ready!" << std::endl;
	return true;
}

void OnError(Lacewing::Relay::Server &Server, Lacewing::Error &Error)
{
	std::cout << "Error: \"" << Error.ToString() << '"' << std::endl;
}