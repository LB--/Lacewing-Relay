#include <iostream>

#include "Relay.hpp"

bool OnConnect(Lacewing::Relay::Client &Client);
void OnError(Lacewing::Relay::Client &Client, Lacewing::Error &Error);

int main()
{
	Lacewing::EventPump AppEventPump;
	Lacewing::Relay::Server RelayServer(AppEventPump);
	RelayServer.Host(6121);
	RelayServer.onConnect(OnConnect);
	RelayServer.onError(OnError);

	AppEventPump.StartEventLoop();

	return 0;
}

bool OnConnect(Lacewing::Relay::Client &Client)
{
	std::cout << "This program's client has connected!" << std::endl;
	return true;
}

void OnError(Lacewing::Relay::Client &Client, Lacewing::Error &Error)
{
	std::cout << "Error: [" << Error.ToString() << std::endl;
}