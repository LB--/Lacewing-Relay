#ifndef __DarkWireSoftware_Relay_HeaderPlusPlus__
#define __DarkWireSoftware_Relay_HeaderPlusPlus__

#include <list>

#include "lacewing.h"

namespace lacewing
{
	namespace relay
	{
		struct _server
		{
		public:
			operator lw_server ()
			{
				return internal;
			}
		private:
			std::list<size_t> usedIDs;
			size_t lowestCleanID;
			size_t GetFreeID();
			void SetFreeID(size_t ID);
		protected:
			lw_server internal;

			const lw_pump const msgPump;
			char * welcomeMessage;
			bool enableChannelListing;

			struct _channel
			{
				std::list<relay::_client *> listOfPeers;
				lacewing::client master;
				lacewing::server server;
				unsigned short id;
				char * name;
				bool closeWhenMasterLeaves, listInPublicChannelList;

				void Join(_server & Server, _server_client & Client);
				void Leave(_server & Server, _server_client & Client);
				
				_channel(_server & Server, const char * const Name);
				~_channel();
			};

			struct _client
			{
				unsigned short id; /* See spec 2.1.2 */
				std::list<_channel *> listOfChannels;
				const char * name;

				relay::_server * server;
				lacewing::_server_client * container;
				void Write(lacewing::stream & Str, unsigned char Type, unsigned char Variant);

				_client(relay::_server & Server, lacewing::_server_client & Client);
				~_client();
			};
			
			void CloseChannel(relay::_server::_channel * Channel);

			typedef bool (lw_import * handlerConnectRelay)
				(relay::_server & Server, relay::_server::_client & Client);

			typedef void (lw_import * handlerDisconnectRelay)
				(relay::_server & Server, relay::_server::_client & Client);

			typedef void (lw_import * handlerServerMessageRelay)
				(relay::_server & Server, relay::_server::_client & Client, unsigned char Variant,
				 unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lw_import * handlerErrorRelay)
				(relay::_server &Server, _error &Error);
			
			typedef bool (lw_import * handlerJoinChannelRelay)
				(relay::_server & Server, relay::_server::_client & Client, bool Blasted, relay::_server::_channel & Channel,
				 bool &CloseWhenMasterLeaves, bool &ShowInChannelList, char * &DenyReason, bool & FreeDenyReason);
			
			typedef bool (lw_import * handlerChannelMessageRelay)
				(relay::_server & Server, relay::_server::_client & Client, bool Blasted, relay::_server::_channel & Channel,
				 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

			typedef bool (lw_import * handlerPeerMessageRelay)
				(relay::_server & Server, relay::_server::_client & Client_Send, bool Blasted, relay::_server::_channel & Channel,
				 relay::_server::_client & Client_Recv, unsigned char Variant, unsigned char Subchannel,
				 const char * Data, size_t Size);

			typedef bool (lw_import * handlerLeaveChannelRelay)
				(relay::_server & Server, relay::_server::_client & Client, relay::_server::_channel & Channel,
				 char * & DenyReason, bool & FreeDenyReason);

			/*	In this handler, you can call Client.Name() for the current name before approving the name change.
				If you deny the change, "Custom server deny reason." will be given by default. If you wish to specify
				the reason, modify DenyReason to point to a string literal or malloc()-style memory. In the case
				of using malloc(), set FreeDenyReason to true, else it will default to false. */

			typedef bool (lw_import * handlerNameSetRelay)
				(relay::_server & Server, relay::_server::_client & Client, const char * Name, char * & DenyReason,
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

			std::list<_channel *> listOfChannels;
			
			_server(_pump &);
			~_server();
		};


		relay::_server::_client *   ToRelay(lacewing::server_client);
		relay::_server *            ToRelay(lacewing::server);

		/////////////////////////////////////////////////////////////////////////////////

		struct _client
		{
			lacewing::_pump * const msgPump;

			struct _peer;
			struct _channel {
				std::list<_peer *> listOfPeers;
				_peer * master;
				_client * client;
				unsigned short id;
				char * name;
				bool closeWhenMasterLeaves, listInPublicChannelList;

				void PeerJoin(_client & Client, _client::_peer & Peer);
				void PeerLeave(_client & Client, _client::_peer & Peer);
				
				_channel(_client & Client, const char * Name);
				~_channel();
			};

			struct _peer
			{
				unsigned short id; /* See spec 2.1.2 */
				std::list<_channel *> listOfChannels;
				const char * name;

				relay::_client * client;
				//lacewing::Client * container; // any original lacewing struct?
				void Write(lacewing::stream Str, unsigned char Type, unsigned char Variant);

				_peer(relay::_client & Client, lacewing::_client::_peer & Peer);
				~_peer();
			};
			
			void CloseChannel(relay::_client::_channel * Channel);

			typedef void (lw_import * handlerConnectRelay)
				(relay::_client & Client);

			typedef void (lw_import * handlerDisconnectRelay)
				(relay::_client & Client);

			typedef void (lw_import * handlerServerMessageRelay)
				(relay::_client & Client, bool Blasted, unsigned char Variant,
				 unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lw_import * handlerErrorRelay)
				(relay::_client & Client, error & Error);
			
			typedef bool (lw_import * handlerJoinChannelRelay)
				(relay::_client & Client, bool Blasted, relay::_client::_channel & Channel,
				 bool & CloseWhenMasterLeaves, bool & ShowInChannelList, char * & DenyReason, bool & FreeDenyReason);
			
			typedef void (lw_import * handlerChannelMessageRelay)
				(relay::_client & Client, relay::_client::_peer & Peer, bool Blasted, relay::_client::_channel & Channel,
				 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lw_import * handlerPeerMessageRelay)
				(relay::_client & Client, relay::_client::_peer & Peer, bool Blasted, relay::_client::_channel & Channel,
				 unsigned char Variant, unsigned char Subchannel, const char * Data, size_t Size);

			typedef void (lw_import * handlerLeaveChannelRelay)
				(relay::_client & Client, relay::_client::_peer & Peer, relay::_client::_channel & Channel,
				 char * & DenyReason, bool & FreeDenyReason);

			/*	In this handler, you can call Client.Name() for the current name before approving the name change.
				If you deny the change, "Custom server deny reason." will be given by default. If you wish to specify
				the reason, modify DenyReason to point to a string literal or malloc()-style memory. In the case
				of using malloc(), set FreeDenyReason to true, else it will default to false. */

			typedef void (lw_import * handlerNameSetRelay)
				(relay::_client & Client, const char * Name, char * & DenyReason, bool FreeDenyReason);

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

			std::list<_channel *> listOfChannels;
			
			_client(_pump &);
			~_client();
		};


		relay::_server::_client *    ToRelay(lacewing::_server_client *);
		relay::_server *            ToRelay(lacewing::_server *);

	}
}

#endif //#ifndef __DarkWireSoftware_Relay_HeaderPlusPlus__
