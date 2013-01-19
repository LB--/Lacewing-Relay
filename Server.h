
/* Relay server: implements the Relay protocol via extending the Lacewing class */
#include "Lacewing.h"
#include <new.h>
#include <assert.h>
#include "list.h"


namespace Lacewing {
namespace Relay {
/* You should never call these functions manually; they are proxies from raw liblacewing -> Relay */
void onConnect_Relay     (Lacewing::Server &server, Lacewing::Server::Client &client);
void onDisconnect_Relay  (Lacewing::Server &server, Lacewing::Server::Client &client);
void onReceive_Relay     (Lacewing::Server &server, Lacewing::Server::Client &client, const char * data, size_t size);
void onError_Relay       (Lacewing::Server &server, Error &error);

struct Server : public Lacewing::Server {
	
protected:
	List<size_t> usedIDs;
	size_t lowestCleanID;
	size_t GetFreeID();
	void SetFreeID(size_t ID);
public:
	Lacewing::Pump * const MsgPump;
	char * WelcomeMessage;
	bool EnableChannelListing;

	struct Client;
	struct Channel {
		List<Client *> listOfPeers;
		Client * master;
		Server * server;
		unsigned short ID;
		char * Name;
		bool closeWhenMasterLeaves, listInPublicChannelList;

		void Join(Server & server, Server::Client & client);
		void Leave(Server & server, Server::Client & client);
		
		Channel(Server & server, const char * Name);
		~Channel();
	};

	struct Client
	{
		unsigned short ID; /* See spec 2.1.2 */
		List<Channel *> listOfChannels;
		const char * Name;

		Relay::Server * server;
		Lacewing::Server::Client * container;
		void Write(Lacewing::Stream &Str, unsigned char Type, unsigned char Variant);

		Client(Relay::Server & server, Lacewing::Server::Client & client);
		~Client();
	};
	
	void CloseChannel(Relay::Server::Channel * Channel);

	typedef bool (LacewingHandler * HandlerConnectRelay)
		(Relay::Server &Server, Relay::Server::Client &Client);

	typedef void (LacewingHandler * HandlerDisconnectRelay)
		(Relay::Server &Server, Relay::Server::Client &Client);

	typedef void (LacewingHandler * HandlerServerMessageRelay)
		(Relay::Server &Server, Relay::Server::Client &Client, unsigned char Variant,
		 unsigned char Subchannel, const char * Data, size_t Size);

	typedef void (LacewingHandler * HandlerErrorRelay)
		(Relay::Server &Server, Error &Error);
	
	typedef bool (LacewingHandler * HandlerJoinChannelRelay)
		(Relay::Server &Server, Relay::Server::Client &Client, bool Blasted, Relay::Server::Channel &Channel,
		 bool &CloseWhenMasterLeaves, bool &ShowInChannelList, char * &DenyReason, bool &FreeDenyReason);
	
	typedef bool (LacewingHandler * HandlerChannelMessageRelay)
		(Relay::Server &Server, Relay::Server::Client &Client, bool Blasted, Relay::Server::Channel &Channel,
		 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

	typedef bool (LacewingHandler * HandlerPeerMessageRelay)
		(Relay::Server &Server, Relay::Server::Client &Client_Send, bool Blasted, Relay::Server::Channel &Channel,
		 Relay::Server::Client &Client_Recv, unsigned char Variant, unsigned char Subchannel,
		 const char * Data, size_t Size);

	typedef bool (LacewingHandler * HandlerLeaveChannelRelay)
		(Relay::Server &Server, Relay::Server::Client &Client, Relay::Server::Channel &Channel,
		 char * &DenyReason, bool &FreeDenyReason);

	/*	In this handler, you can call Client.Name() for the current name before approving the name change.
		If you deny the change, "Custom server deny reason." will be given by default. If you wish to specify
		the reason, modify DenyReason to point to a string literal or malloc()-style memory. In the case
		of using malloc(), set FreeDenyReason to true, else it will default to false. */

	typedef bool (LacewingHandler * HandlerNameSetRelay)
		(Relay::Server &Server, Relay::Server::Client &Client, const char * Name, char * &DenyReason,
		 bool FreeDenyReason);

	struct
	{
		HandlerConnectRelay onConnect;
		HandlerDisconnectRelay onDisconnect;
		HandlerNameSetRelay onNameSet;
		HandlerErrorRelay onError;
		HandlerJoinChannelRelay onJoinChannel;
		HandlerLeaveChannelRelay onLeaveChannel;
		HandlerServerMessageRelay onServerMessage;
		HandlerChannelMessageRelay onChannelMessage;
		HandlerPeerMessageRelay onPeerMessage;
	} Handlers;


	void onConnect(HandlerConnectRelay);
	void onDisconnect(HandlerDisconnectRelay);
	void onNameSet(HandlerNameSetRelay);
	void onError(HandlerErrorRelay);
	void onJoinChannel(HandlerJoinChannelRelay);
	void onLeaveChannel(HandlerLeaveChannelRelay);
	void onServerMessage(HandlerServerMessageRelay);
	void onChannelMessage(HandlerChannelMessageRelay);
	void onPeerMessage(HandlerPeerMessageRelay);

	List<Channel *> listOfChannels;
	
	Server(Pump &);
	~Server();
};


Relay::Server::Client *    ToRelay(Lacewing::Server::Client *);
Relay::Server *            ToRelay(Lacewing::Server *);

}
}