#define NOMINMAX /*for Windows.h*/

#include <iostream>
#include <limits>

//Keep console window open at end of application until user presses enter key
struct KR{~KR()
{
	std::cin.sync();
	std::cout << std::endl << "End of application, press Enter...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}}kr;

#include "Relay.hpp"

bool OnConnect(LwRelay::Server &Server, LwRelay::Server::Client &Client);
void OnError(LwRelay::Server &Server, lacewing::error Error);

int main(unsigned nargs, char const *const *args)
{
	lacewing::eventpump Pump (lacewing::eventpump_new());
	LwRelay::Server Server (Pump);
	server.onConnect(OnConnect);
	server.onError(OnError);

	server.Host(6121);
	std::clog << "Server hosting on port: " << Server.Port() << "..." << std::endl;

	lacewing::error e = Pump->start_eventloop();
	lacewing::pump_delete(Pump);
	if(e)
	{
		std::cerr << e->tostring() << std::endl;
		lacewing::error_delete(e);
		return -1;
	}
}

bool OnConnect(LwRelay::Server &Server, LwRelay::Server::Client &Client)
{
	std::clog << "Client connected" << std::endl;
	return true;
}

void OnError(LwRelay::Server &Server, lacewing::error Error)
{
	std::cerr << "Error: \"" << Error->tostring() << '"' << std::endl;
}