#define NOMINMAX /*for Windows.h*/

#include <iostream>
#include <limits>

//Keep console window open at end of application until user presses enter key
struct KR{~KR()
{
	std::cin.sync();
	std::cout << std::endl << "End of application, press Enter..." << std::flush;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}}kr;

#include "Relay.hpp"

struct Main
{
	lacewing::eventpump Pump;
	LwRelay::Client Client;

	Main(unsigned nargs, char const *const *args) : Pump(lacewing::eventpump_new()), Client(Pump)
	{
		Client.Tag = static_cast<void *>(this);
		Client.onError(OnError);
		Client.onConnect(OnConnect);
		Client.onConnectionDenied(OnConnectionDenied);
	}
	~Main()
	{
		Client.Tag = nullptr;
		lacewing::pump_delete(Pump), Pump = nullptr;
	}

	int Go()
	{
		Client.Connect("localhost"/*, 6121*/);
		std::clog << "Connecting to " << Client.ServerAddress()->tostring() << std::endl;

		lacewing::error e = Pump->start_eventloop();
		if(e)
		{
			std::cerr << e->tostring() << std::endl;
			lacewing::error_delete(e), e = nullptr;
			return -1;
		}

		return 0;
	}

	static void (lw_callback OnError)(LwRelay::Client &Client, lacewing::error Error)
	{
		std::cerr << "Error: \"" << Error->tostring() << '"' << std::endl;
		static_cast<Main *>(Client.Tag)->Pump->post_eventloop_exit();
	}
	static void (lw_callback OnConnect)(LwRelay::Client &Client)
	{
		std::clog << "Connected to " << Client.ServerAddress()->tostring() << std::endl;
	}
	static void (lw_callback OnConnectionDenied)(LwRelay::Client &Client, char const*reason)
	{
		std::clog << "Connection refused: \"" << reason << '"' << std::endl;
	}
};

int main(/*un*/signed nargs, char const *const *args)
{
	return Main(static_cast<unsigned>(nargs), args).Go();
}
