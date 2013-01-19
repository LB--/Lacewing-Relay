#include <iostream>

#include "Relay.hpp"

bool OnConnect(Lacewing::Relay::Server &Server, Lacewing::Relay::Server::Client &Client);
void OnError(Lacewing::Relay::Server &Server, Lacewing::Error &Error);

int main(unsigned nargs, const char const * const * args)
{
	Lacewing::EventPump Pump;
	Lacewing::Relay::Server Server (Pump);
	Server.onConnect(OnConnect);
	Server.onError(OnError);
	Server.Host(6121);

	std::cout << "Server hosting on port: " << Server.Port() << "..." << std::endl;
	Pump.StartEventLoop();

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