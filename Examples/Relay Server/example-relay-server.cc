#include <iostream>
#include <lacewing.h>
#include "relay\Server.h"
Lacewing::EventPump AppEventPump;

bool OnConnect(Lacewing::Relay::Server &Server, Lacewing::Relay::Server::Client &Client);
void OnError(Lacewing::Relay::Server &Server, Lacewing::Error &Error);


int main()
{
	//
	Lacewing::Relay::Server RelayServer(AppEventPump);
	RelayServer.Host(6121);
	RelayServer.onConnect(OnConnect);
	RelayServer.onError(OnError);

	std::cout << "Server hosting on port: " << RelayServer.Port() << "..." << std::endl;
	AppEventPump.StartEventLoop();

	return 0;
}

bool OnConnect(Lacewing::Relay::Server &Server, Lacewing::Relay::Server::Client &Client)
{
	std::cout << "Ready!" << std::endl;
	return true;
}

void OnError(Lacewing::Relay::Server &Server, Lacewing::Error &Error)
{
	std::cout << "Error: [" << Error.ToString() << std::endl;
}