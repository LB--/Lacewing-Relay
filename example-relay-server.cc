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
typedef LwRelay::Server::Deny Deny;

struct Main
{
	lacewing::eventpump Pump;
	LwRelay::Server Server;

	Main(unsigned nargs, char const *const *args) : Pump(lacewing::eventpump_new()), Server(Pump)
	{
		Server.Tag = static_cast<void *>(this);
		Server.onError(OnError);
		Server.onConnect(OnConnect);
	}
	~Main()
	{
		Server.Tag = nullptr;
		lacewing::pump_delete(Pump), Pump = nullptr;
	}

	int Go()
	{
		Server.Host(/*6121*/);
		if(Server.Hosting())
		{
			std::clog << "Server hosting on port: " << Server.Port() << std::endl;
			lacewing::error e = Pump->start_eventloop();
			if(e)
			{
				std::cerr << e->tostring() << std::endl;
				lacewing::error_delete(e), e = nullptr;
				return -1;
			}
		}
		else
		{
			std::cerr << "Hosting failed" << std::endl;
			return -1;
		}
		return 0;
	}
	
	static void (lw_callback OnError)(LwRelay::Server &Server, lacewing::error Error)
	{
		std::cerr << "Error: \"" << Error->tostring() << '"' << std::endl;
		static_cast<Main *>(Server.Tag)->Pump->post_eventloop_exit();
	}
	
	static Deny (lw_callback OnConnect)(LwRelay::Server &Server, LwRelay::Server::Client &Client)
	{
		std::clog << "Client connected from " << Client.Address()->tostring() << std::endl;
		return true; //allow connection
		//return Deny("Your IP is banned from this server");
	}
};

int main(/*un*/signed nargs, char const *const *args)
{
	return Main(static_cast<unsigned>(nargs), args).Go();
}
