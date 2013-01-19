#ifndef __DarkWireSoftware_Relay_HeaderPlusPlus__
#define __DarkWireSoftware_Relay_HeaderPlusPlus__

#include <list>

#include "lacewing.h"

namespace lacewing
{
	namespace relay
	{
		struct server
		{
		private:
			List<size_t> usedIDs;
			size_t lowestCleanID;
			size_t GetFreeID();
			void SetFreeID(size_t ID);
		protected:
			lacewing::pump * const msgPump;
			char * welcomeMessage;
			bool enableChannelListing;

			struct channel
			{
				List<client *> listOfPeers;
				client * master;
				server * server;
				unsigned short id;
				char * name;
				bool closeWhenMasterLeaves, listInPublicChannelList;

				void Join(server & Server, server::client & Client);
				void Leave(server & Server, server::client & Client);
				
				channel(server & Server, const char * const Name);
				~channel();
			};

			struct client
			{
				unsigned short id; /* See spec 2.1.2 */
				List<channel *> listOfChannels;
				const char * name;

				relay::server * server;
				lacewing::_lw_server_client * container;
				void Write(lacewing::stream & Str, unsigned char Type, unsigned char Variant);

				client(relay::server & Server, lacewing::_lw_server_client & Client);
				~client();
			};
			
			void CloseChannel(relay::server::channel * Channel);

			typedef bool (lacewingHandler * handlerConnectRelay)
				(relay::server & Server, relay::server::client & Client);

			typedef void (lacewingHandler * handlerDisconnectRelay)
				(relay::server & Server, relay::server::client & Client);

			typedef void (lacewingHandler * handlerServerMessageRelay)
				(relay::server & Server, relay::server::client & Client, unsigned char Variant,
				 unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lacewingHandler * handlerErrorRelay)
				(relay::server &Server, error &Error);
			
			typedef bool (lacewingHandler * handlerJoinChannelRelay)
				(relay::server & Server, relay::server::client & Client, bool Blasted, relay::server::channel & Channel,
				 bool &CloseWhenMasterLeaves, bool &ShowInChannelList, char * &DenyReason, bool &FreeDenyReason);
			
			typedef bool (lacewingHandler * handlerChannelMessageRelay)
				(relay::server & Server, relay::server::client & Client, bool Blasted, relay::server::channel & Channel,
				 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

			typedef bool (lacewingHandler * handlerPeerMessageRelay)
				(relay::server & Server, relay::server::client & Client_Send, bool Blasted, relay::server::channel & Channel,
				 relay::server::client & Client_Recv, unsigned char Variant, unsigned char Subchannel,
				 const char * Data, size_t Size);

			typedef bool (lacewingHandler * handlerLeaveChannelRelay)
				(relay::server & Server, relay::server::client & Client, relay::server::channel & Channel,
				 char * & DenyReason, bool & FreeDenyReason);

			/*	In this handler, you can call Client.Name() for the current name before approving the name change.
				If you deny the change, "Custom server deny reason." will be given by default. If you wish to specify
				the reason, modify DenyReason to point to a string literal or malloc()-style memory. In the case
				of using malloc(), set FreeDenyReason to true, else it will default to false. */

			typedef bool (lacewingHandler * handlerNameSetRelay)
				(relay::server & Server, relay::server::client & Client, const char * Name, char * & DenyReason,
				 bool FreeDenyReason);

			struct
			{
				handlerConnectRelay onConnect;
				handlerDisconnectRelay onDisconnect;
				handlerNameSetRelay onNameSet;
				handlerErrorRelay onError;
				handlerJoinChannelRelay onJoinChannel;
				handlerLeaveChannelRelay onLeaveChannel;
				handlerServerMessageRelay onServerMessage;
				handlerChannelMessageRelay onChannelMessage;
				handlerPeerMessageRelay onPeerMessage;
			} handlers;


			void onConnect(handlerConnectRelay);
			void onDisconnect(handlerDisconnectRelay);
			void onNameSet(handlerNameSetRelay);
			void onError(handlerErrorRelay);
			void onJoinChannel(handlerJoinChannelRelay);
			void onLeaveChannel(handlerLeaveChannelRelay);
			void onServerMessage(handlerServerMessageRelay);
			void onChannelMessage(handlerChannelMessageRelay);
			void onPeerMessage(handlerPeerMessageRelay);

			List<channel *> listOfChannels;
			
			server(pump &);
			~server();
		};


		relay::server::client *    ToRelay(lacewing::_lw_server_client *);
		relay::server *            ToRelay(lacewing::_lw_server *);

		/////////////////////////////////////////////////////////////////////////////////

		struct client
		{
			lacewing::pump * const msgPump;

			struct peer;
			struct channel {
				List<peer *> listOfPeers;
				peer * master;
				client * Client;
				unsigned short ID;
				char * Name;
				bool closeWhenMasterLeaves, listInPublicChannelList;

				void PeerJoin(client & Client, client::peer & Peer);
				void PeerLeave(client & Client, client::peer & Peer);
				
				channel(client & Client, const char * Name);
				~channel();
			};

			struct peer
			{
				unsigned short id; /* See spec 2.1.2 */
				List<channel *> listOfChannels;
				const char * name;

				relay::client * client;
				//lacewing::Client * container; // any original lacewing struct?
				void Write(lacewing::stream & Str, unsigned char Type, unsigned char Variant);

				peer(relay::client & Client, lacewing::client::peer & Peer);
				~peer();
			};
			
			void CloseChannel(relay::client::channel * Channel);

			typedef void (lacewingHandler * handlerConnectRelay)
				(relay::client & Client);

			typedef void (lacewingHandler * handlerDisconnectRelay)
				(relay::client & Client);

			typedef void (lacewingHandler * handlerServerMessageRelay)
				(relay::client & Client, bool Blasted, unsigned char Variant,
				 unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lacewingHandler * handlerErrorRelay)
				(relay::client & Client, error & Error);
			
			typedef bool (lacewingHandler * handlerJoinChannelRelay)
				(relay::client & Client, bool Blasted, relay::client::channel & Channel,
				 bool & CloseWhenMasterLeaves, bool & ShowInChannelList, char * & DenyReason, bool & FreeDenyReason);
			
			typedef void (lacewingHandler * handlerChannelMessageRelay)
				(relay::client & Client, relay::client::peer & Peer, bool Blasted, relay::client::channel & Channel,
				 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lacewingHandler * handlerPeerMessageRelay)
				(relay::client & Client, relay::client::peer & Peer, bool Blasted, relay::client::channel & Channel,
				 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lacewingHandler * handlerLeaveChannelRelay)
				(relay::client & Client, relay::client::peer & Peer, relay::client::channel & Channel,
				 char * & DenyReason, bool & FreeDenyReason);

			/*	In this handler, you can call Client.Name() for the current name before approving the name change.
				If you deny the change, "Custom server deny reason." will be given by default. If you wish to specify
				the reason, modify DenyReason to point to a string literal or malloc()-style memory. In the case
				of using malloc(), set FreeDenyReason to true, else it will default to false. */

			typedef void (lacewingHandler * handlerNameSetRelay)
				(relay::client & Client, const char * Name, char * & DenyReason, bool FreeDenyReason);

			struct
			{
				handlerConnectRelay onConnect;
				handlerDisconnectRelay onDisconnect;
				handlerNameSetRelay onNameSet;
				handlerErrorRelay onError;
				handlerJoinChannelRelay onJoinChannel;
				handlerLeaveChannelRelay onLeaveChannel;
				handlerServerMessageRelay onServerMessage;
				handlerChannelMessageRelay onChannelMessage;
				handlerPeerMessageRelay onPeerMessage;
			} handlers;


			void onConnect(handlerConnectRelay);
			void onDisconnect(handlerDisconnectRelay);
			void onNameSet(handlerNameSetRelay);
			void onError(handlerErrorRelay);
			void onJoinChannel(handlerJoinChannelRelay);
			void onLeaveChannel(handlerLeaveChannelRelay);
			void onServerMessage(handlerServerMessageRelay);
			void onChannelMessage(handlerChannelMessageRelay);
			void onPeerMessage(handlerPeerMessageRelay);

			List<channel *> listOfChannels;
			
			client(pump &);
			~client();
		};


		relay::server::client *    ToRelay(lacewing::_lw_server_client *);
		relay::server *            ToRelay(lacewing::_lw_server *);

	}
}

#endif //#ifndef __DarkWireSoftware_Relay_HeaderPlusPlus__
