#ifndef __DarkWireSoftware_Relay_HeaderPlusPlus__
#define __DarkWireSoftware_Relay_HeaderPlusPlus__

#include <list>

#include "lacewing.h"

namespace lacewing
{
	namespace Relay
	{
		/* You should never call these functions manually; they are proxies from raw liblacewing -> Relay */
		void onConnect_Relay     (lacewing::_lw_server &server, lacewing::_lw_server_client &client);
		void onDisconnect_Relay  (lacewing::_lw_server &server, lacewing::_lw_server_client &client);
		void onReceive_Relay     (lacewing::_lw_server &server, lacewing::_lw_server_client &client, const char * data, size_t size);
		void onError_Relay       (lacewing::_lw_server &server, Error &error);

		struct Server: public lacewing::_lw_server
		{
		protected:
			List<size_t> usedIDs;
			size_t lowestCleanID;
			size_t GetFreeID();
			void SetFreeID(size_t ID);
		public:
			lacewing::Pump * const MsgPump;
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
				lacewing::_lw_server_client * container;
				void Write(lacewing::Stream &Str, unsigned char Type, unsigned char Variant);

				Client(Relay::Server & server, lacewing::_lw_server_client & client);
				~Client();
			};
			
			void CloseChannel(Relay::Server::Channel * Channel);

			typedef bool (lacewingHandler * HandlerConnectRelay)
				(Relay::Server &Server, Relay::Server::Client &Client);

			typedef void (lacewingHandler * HandlerDisconnectRelay)
				(Relay::Server &Server, Relay::Server::Client &Client);

			typedef void (lacewingHandler * HandlerServerMessageRelay)
				(Relay::Server &Server, Relay::Server::Client &Client, unsigned char Variant,
				 unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lacewingHandler * HandlerErrorRelay)
				(Relay::Server &Server, Error &Error);
			
			typedef bool (lacewingHandler * HandlerJoinChannelRelay)
				(Relay::Server &Server, Relay::Server::Client &Client, bool Blasted, Relay::Server::Channel &Channel,
				 bool &CloseWhenMasterLeaves, bool &ShowInChannelList, char * &DenyReason, bool &FreeDenyReason);
			
			typedef bool (lacewingHandler * HandlerChannelMessageRelay)
				(Relay::Server &Server, Relay::Server::Client &Client, bool Blasted, Relay::Server::Channel &Channel,
				 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

			typedef bool (lacewingHandler * HandlerPeerMessageRelay)
				(Relay::Server &Server, Relay::Server::Client &Client_Send, bool Blasted, Relay::Server::Channel &Channel,
				 Relay::Server::Client &Client_Recv, unsigned char Variant, unsigned char Subchannel,
				 const char * Data, size_t Size);

			typedef bool (lacewingHandler * HandlerLeaveChannelRelay)
				(Relay::Server &Server, Relay::Server::Client &Client, Relay::Server::Channel &Channel,
				 char * &DenyReason, bool &FreeDenyReason);

			/*	In this handler, you can call Client.Name() for the current name before approving the name change.
				If you deny the change, "Custom server deny reason." will be given by default. If you wish to specify
				the reason, modify DenyReason to point to a string literal or malloc()-style memory. In the case
				of using malloc(), set FreeDenyReason to true, else it will default to false. */

			typedef bool (lacewingHandler * HandlerNameSetRelay)
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


		Relay::Server::Client *    ToRelay(lacewing::_lw_server_client *);
		Relay::Server *            ToRelay(lacewing::_lw_server *);

		/////////////////////////////////////////////////////////////////////////////////

		/* You should never call these functions manually; they are proxies from raw liblacewing -> Relay */
		void onConnect_Relay     (lacewing::Client &client);
		void onDisconnect_Relay  (lacewing::Client &client);
		void onReceive_Relay     (lacewing::Client &client, const char * data, size_t size);
		void onError_Relay       (lacewing::Client &client, Error &error);

		struct Client : public lacewing::Client {
			
		public:
			lacewing::Pump * const MsgPump;
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
				//lacewing::Client * container; // any original lacewing struct?
				void Write(lacewing::Stream &Str, unsigned char Type, unsigned char Variant);

				Peer(Relay::Client & client, lacewing::Client::Peer & peer);
				~Peer();
			};
			
			void CloseChannel(Relay::Client::Channel * Channel);

			typedef void (lacewingHandler * HandlerConnectRelay)
				(Relay::Client &client);

			typedef void (lacewingHandler * HandlerDisconnectRelay)
				(Relay::Client &client);

			typedef void (lacewingHandler * HandlerServerMessageRelay)
				(Relay::Client &client, bool Blasted, unsigned char Variant,
				 unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lacewingHandler * HandlerErrorRelay)
				(Relay::Client &client, Error &Error);
			
			typedef bool (lacewingHandler * HandlerJoinChannelRelay)
				(Relay::Client &client, bool Blasted, Relay::Client::Channel &Channel,
				 bool &CloseWhenMasterLeaves, bool &ShowInChannelList, char * &DenyReason, bool &FreeDenyReason);
			
			typedef void (lacewingHandler * HandlerChannelMessageRelay)
				(Relay::Client &client, Relay::Client::Peer &peer, bool Blasted, Relay::Client::Channel &Channel,
				 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lacewingHandler * HandlerPeerMessageRelay)
				(Relay::Client &client, Relay::Client::Peer &peer, bool Blasted, Relay::Client::Channel &Channel,
				 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lacewingHandler * HandlerLeaveChannelRelay)
				(Relay::Client &client, Relay::Client::Peer &peer, Relay::Client::Channel &Channel,
				 char * &DenyReason, bool &FreeDenyReason);

			/*	In this handler, you can call Client.Name() for the current name before approving the name change.
				If you deny the change, "Custom server deny reason." will be given by default. If you wish to specify
				the reason, modify DenyReason to point to a string literal or malloc()-style memory. In the case
				of using malloc(), set FreeDenyReason to true, else it will default to false. */

			typedef void (lacewingHandler * HandlerNameSetRelay)
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


		Relay::Server::Client *    ToRelay(lacewing::_lw_server_client *);
		Relay::Server *            ToRelay(lacewing::_lw_server *);

	}
}

#endif //#ifndef __DarkWireSoftware_Relay_HeaderPlusPlus__
