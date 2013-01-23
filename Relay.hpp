#ifndef __DarkwireSoftware_Relay_HeaderPlusPlus__
#define __DarkwireSoftware_Relay_HeaderPlusPlus__

#include <map>
#include <vector>
#include <string>
#include <bitset>

#include "lacewing.h"

namespace LwRelay
{
	struct Server
	{
		/**
		 * Construct a new server from a pump, such as an event pump.
		 */
		Server(lacewinng::pump pump);

		struct Client
		{
			typedef unsigned short ID_t; //See spec 2.1.2
			typedef std::string Name_t;

			struct ChannelIterator
			{
			};

			void Send(unsigned char subchannel, unsigned char variant, char const*data, size_t size);
			void Send(unsigned char subchannel, unsigned char variant, char const*null_terminated_string);
		private:
			Server &server;
			lacewing::server_client const client;
			ID_t const id;
			Name_t name;
			typedef std::vector<Channel *> Channels_t;
			Channels_t channels;

			Client(Server &server, lacewing::server_client client);

			friend struct Server;
		};	friend struct Client;

		struct Channel
		{
			std::vector<Client *> clients;
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

		size_t ClientCount() const;
		Client *FirstClient();
		size_t ChannelCount() const;
		Channel *FirstChannel();
		void SetChannelListing(bool enabled);
		void SetWelcomeMessage(char const*message);
		void Host(unsigned short port);
		void Host(lacewing::filter filter);
		void Unhost();
		bool Hosting() const;
		unsigned short Port() const;

		/**
		 * Some handlers let you return this to indicate whether the
		 * action should be allowed or denied. If constructed from a
		 * bool, true means allow and false means deny. If constructed
		 * from a c-string, it is implicitly meant to deny, using the
		 * given string as the reason given to the client.
		 */
		struct Deny
		{
			Deny(bool do_not_deny);
			Deny(char const*reason);
			Deny(Deny const&other);
			Deny operator=(Deny const&other);
			~Deny();
		private:
			struct Impl;
			Impl *impl;
		};

		//fully-qualified names are used to be copy-paste friendly (except for "Deny")
		typedef void (lw_import          ErrorHandler)(::LwRelay::Server &server, ::lacewing::error error);
		typedef Deny (lw_import        ConnectHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client);
		typedef void (lw_import     DisconnectHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client);
		typedef Deny (lw_import        NameSetHandler)(::LwRelay::Server &Server, ::LwRelay::Server::Client &client, char const*name);
		typedef Deny (lw_import    JoinChannelHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel, bool closeformaster, bool visible);
		typedef Deny (lw_import   LeaveChannelHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel);
		typedef void (lw_import  ServerMessageHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client,                                                    unsigned char subchannel, unsigned char variant, char const*data, size_t size);
		typedef Deny (lw_import ChannelMessageHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel, bool blasted, unsigned char subchannel, unsigned char variant, char const*data, size_t size);
		typedef Deny (lw_import    PeerMessageHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &from  , ::LwRelay::Server::Channel &channel, bool blasted, unsigned char subchannel, unsigned char variant, char const*data, size_t size, ::LwRelay::Server::Client &to);

		void onError         (         ErrorHandler &handler);
		void onConnect       (       ConnectHandler &handler);
		void onDisconnect    (    DisconnectHandler &handler);
		void onNameSet       (       NameSetHandler &handler);
		void onJoinChannel   (   JoinChannelHandler &handler);
		void onLeaveChannel  (  LeaveChannelHandler &handler);
		void onServerMessage ( ServerMessageHandler &handler);
		void onChannelMessage(ChannelMessageHandler &handler);
		void onPeerMessage   (   PeerMessageHandler &handler);

	private:
		struct
		{
			         ErrorHandler *Error;
			       ConnectHandler *Connect;
			    DisconnectHandler *Disconnect;
			       NameSetHandler *NameSet;
			   JoinChannelHandler *JoinChannel;
			  LeaveChannelHandler *LeaveChannel;
			 ServerMessageHandler *ServerMessage;
			ChannelMessageHandler *ChannelMessage;
			   PeerMessageHandler *PeerMessage;
		} handlers;

		lw_pump const msgPump;
		lw_server const lws;
		std::map<size_t, Client> clients;
		size_t lowestCleanID;
		size_t GetFreeID();
		char * welcomeMessage;
		bool enableChannelListing;
		
		void CloseChannel(Channel *channel);

		std::list<_channel *> listOfChannels;

		friend struct Server;
	};	friend struct Channel;


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

#endif //#ifndef __DarkwireSoftware_Relay_HeaderPlusPlus__
