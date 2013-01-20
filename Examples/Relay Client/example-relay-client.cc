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

bool OnConnect(LwRelay::Client &Client);
void OnError(LwRelay::Client &Client, lacewing::error Error);

int main(unsigned nargs, char const *const *args)
{
	lacewing::eventpump Pump (lacewing::eventpump_new());
	LwRelay::Client Client (Pump);
	Client.onConnect(OnConnect);
	Client.onError(OnError);

	Client.Connect("localhost", 6121);

	lacewing::error e = Pump->start_eventloop();
	lacewing::pump_delete(Pump);
	if(e)
	{
		std::cerr << e->tostring() << std::endl;
		lacewing::error_delete(e);
		return -1;
	}

	return 0;
}

bool OnConnect(LwRelay::Client &Client)
{
	std::clog << "Connected to server" << std::endl;
	return true;
}

void OnError(LwRelay::Client &Client, lacewing::error Error)
{
	std::cerr << "Error: \"" << Error->tostring() << '"' << std::endl;
}