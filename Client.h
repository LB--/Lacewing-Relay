
/* Relay server: implements the Relay protocol via extending the Lacewing class */
#include "Lacewing.h"
#include <new.h>
#include <assert.h>
#include "list.h"


namespace Lacewing {
namespace Relay {
/* You should never call these functions manually; they are proxies from raw liblacewing -> Relay */
void onConnect_Relay     (Lacewing::Client &client);
void onDisconnect_Relay  (Lacewing::Client &client);
void onReceive_Relay     (Lacewing::Client &client, const char * data, size_t size);
void onError_Relay       (Lacewing::Client &client, Error &error);

struct Client : public Lacewing::Client {
	
public:
	Lacewing::Pump * const MsgPump;
	char * WelcomeMessage;
	bool EnableChannelListing;

	struct Peer;
	struct Channel {
		List<Peer *> listOfPeers;
		Peer * master;
		Client * client;
		unsigned short ID;
		char * Name;
		bool closeWhenMasterLeaves, listInPublicChannelList;

		void PeerJoin(Client & client, Client::Peer & peer);
		void PeerLeave(Client & client, Client::Peer & peer);
		
		Channel(Client & client, const char * Name);
		~Channel();
	};

	struct Peer
	{
		unsigned short ID; /* See spec 2.1.2 */
		List<Channel *> listOfChannels;
		const char * Name;

		Relay::Client * client;
		//Lacewing::Client * container; // any original Lacewing struct?
		void Write(Lacewing::Stream &Str, unsigned char Type, unsigned char Variant);

		Peer(Relay::Client & client, Lacewing::Client::Peer & peer);
		~Peer();
	};
	
	void CloseChannel(Relay::Client::Channel * Channel);

	typedef void (LacewingHandler * HandlerConnectRelay)
		(Relay::Client &client);

	typedef void (LacewingHandler * HandlerDisconnectRelay)
		(Relay::Client &client);

	typedef void (LacewingHandler * HandlerServerMessageRelay)
		(Relay::Client &client, bool Blasted, unsigned char Variant,
		 unsigned char Subchannel, const char * Data, size_t Size);

	typedef void (LacewingHandler * HandlerErrorRelay)
		(Relay::Client &client, Error &Error);
	
	typedef bool (LacewingHandler * HandlerJoinChannelRelay)
		(Relay::Client &client, bool Blasted, Relay::Client::Channel &Channel,
		 bool &CloseWhenMasterLeaves, bool &ShowInChannelList, char * &DenyReason, bool &FreeDenyReason);
	
	typedef void (LacewingHandler * HandlerChannelMessageRelay)
		(Relay::Client &client, Relay::Client::Peer &peer, bool Blasted, Relay::Client::Channel &Channel,
		 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

	typedef void (LacewingHandler * HandlerPeerMessageRelay)
		(Relay::Client &client, Relay::Client::Peer &peer, bool Blasted, Relay::Client::Channel &Channel,
		 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

	typedef void (LacewingHandler * HandlerLeaveChannelRelay)
		(Relay::Client &client, Relay::Client::Peer &peer, Relay::Client::Channel &Channel,
		 char * &DenyReason, bool &FreeDenyReason);

	/*	In this handler, you can call Client.Name() for the current name before approving the name change.
		If you deny the change, "Custom server deny reason." will be given by default. If you wish to specify
		the reason, modify DenyReason to point to a string literal or malloc()-style memory. In the case
		of using malloc(), set FreeDenyReason to true, else it will default to false. */

	typedef void (LacewingHandler * HandlerNameSetRelay)
		(Relay::Client &client, const char * Name, char * &DenyReason, bool FreeDenyReason);

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
	
	Client(Pump &);
	~Client();
};


Relay::Server::Client *    ToRelay(Lacewing::Server::Client *);
Relay::Server *            ToRelay(Lacewing::Server *);

}
}