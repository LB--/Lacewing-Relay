#ifndef __DarkwireSoftware_Relay_HeaderPlusPlus__
#define __DarkwireSoftware_Relay_HeaderPlusPlus__

#include "lacewing.h"

namespace LwRelay
{
	struct Server
	{
		void *Tag;

		/**
		 * Construct a new server from a pump, such as an event pump.
		 */
		Server(lacewing::pump pump);
		~Server();

		struct Client
		{
			void *Tag;

			typedef unsigned short ID_t;

			struct ChannelIterator
			{
			};

			ID_t ID() const;
			char const*Name() const;
			void Name(char const*name);
			lacewing::address Address() /*const*/;
			void Disconnect();
			void Send(bool blast, unsigned char subchannel, unsigned char variant, char const*data, size_t size);
			void Send(bool blast, unsigned char subchannel, unsigned char variant, char const*null_terminated_string);
			size_t ChannelCount() const;
			ChannelIterator ChannelIterator();
			Client *Next();

		private:
			struct Impl;
			Impl *impl;

			Client(Server &server, lacewing::server_client client);
			#if __cplusplus >= 201103L // >= C++11
			Client(Client const&) = delete;
			Client operator=(Client const&) = delete;
			#else
			Client(Client const&);
			Client operator=(Client const&);
			#endif
			~Client();

			friend struct Channel;
			friend struct Server;
		};	friend struct Client;

		struct Channel
		{
			void *Tag;

			typedef unsigned short ID_t;

			struct ClientIterator
			{
			};

			ID_t ID() const;
			char const*Name() const;
			void Name(char const*name);
			bool AutoClose() const;
			void AutoClose(bool autoclose);
			bool Visible() const;
			void Visible(bool visible);
			void Close();
			void Send(bool blast, unsigned char subchannel, unsigned char variant, char const*data, size_t size);
			void Send(bool blast, unsigned char subchannel, unsigned char variant, char const*null_terminated_string);
			Client *ChannelMaster();
			size_t ClientCount() const;
			ClientIterator ClientIterator();
			Channel *Next();

		private:
			struct Impl;
			Impl *impl;

			Channel(Server &server, char const*name);
			#if __cplusplus >= 201103L // >= C++11
			Channel(Channel const&) = delete;
			Channel operator=(Channel const&) = delete;
			#else
			Channel(Channel const&);
			Channel operator=(Channel const&);
			#endif
			~Channel();

			friend struct Client;
			friend struct Server;
		};	friend struct Channel;

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

			friend struct Server;
		};	friend struct Deny;

		//fully-qualified names are used to be copy-paste friendly (except for "Deny")
		typedef void (lw_import          ErrorHandler)(::LwRelay::Server &server, ::lacewing::error error);
		typedef Deny (lw_import        ConnectHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client);
		typedef void (lw_import     DisconnectHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client);
		typedef Deny (lw_import        NameSetHandler)(::LwRelay::Server &Server, ::LwRelay::Server::Client &client, char const*name);
		typedef Deny (lw_import    JoinChannelHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel, bool autoclose, bool visible);
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
		struct Impl;
		Impl *impl;

		#if __cplusplus >= 201103L // >= C++11
		Server(Server const&) = delete;
		Server operator=(Server const&) = delete;
		#else
		Server(Server const&);
		Server operator=(Server const&);
		#endif
	};

	/////////////////////////////////////////////////////////////////////////////////

	struct Client
	{
		void *Tag;

		struct Channel
		{
			struct Peer
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

			std::list<_peer *> listOfPeers;
			Peer * master;
			Client * client;
			unsigned short id;
			char * name;
			bool closeWhenMasterLeaves, listInPublicChannelList;

			void PeerJoin(_client & Client, _client::_peer & Peer);
			void PeerLeave(_client & Client, _client::_peer & Peer);
			
			_channel(_client & Client, const char * Name);
			~_channel();
		};

		//fully-qualified names are used to be copy-paste friendly
		typedef void (lw_import                ErrorHandler)(::LwRelay::Client &client, lacewing::error error);
		typedef void (lw_import              ConnectHandler)(::LwRelay::Client &client);
		typedef void (lw_import     ConnectionDeniedHandler)(::LwRelay::Client &client,                                                                                                  char const*reason);
		typedef void (lw_import           DisconnectHandler)(::LwRelay::Client &client);
		typedef void (lw_import  ChannelListReceivedHandler)(::LwRelay::Client &client);
		typedef void (lw_import              NameSetHandler)(::LwRelay::Client &client);
		typedef void (lw_import          NameChangedHandler)(::LwRelay::Client &client,                                                                              char const*oldname);
		typedef void (lw_import           NameDeniedHandler)(::LwRelay::Client &client,                                                                              char const*thename, char const*reason);
		typedef void (lw_import          ChannelJoinHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel);
		typedef void (lw_import    ChannelJoinDeniedHandler)(::LwRelay::Client &client,                                                                              char const*thename, char const*reason);
		typedef void (lw_import         ChannelLeaveHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel);
		typedef void (lw_import   ChannelLeaveDeniedHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel,                                                             char const*reason);
		typedef void (lw_import        ServerMessageHandler)(::LwRelay::Client &client,                                                                              bool blasted, unsigned char subchannel, unsigned char variant, char const*data, size_t size);
		typedef void (lw_import ServerChannelMessageHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel,                                         bool blasted, unsigned char subchannel, unsigned char variant, char const*data, size_t size);
		typedef void (lw_import       ChannelMessageHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer, bool blasted, unsigned char subchannel, unsigned char variant, char const*data, size_t size);
		typedef void (lw_import          PeerMessageHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer, bool blasted, unsigned char subchannel, unsigned char variant, char const*data, size_t size);
		typedef void (lw_import             PeerJoinHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer);
		typedef void (lw_import            PeerLeaveHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer);
		typedef void (lw_import       PeerChangeNameHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer, char const*oldname);

		//

		void onError               (               ErrorHandler &handler);
		void onConnect             (             ConnectHandler &handler);
		void onConnectionDenied    (    ConnectionDeniedHandler &handler);
		void onDisconnect          (          DisconnectHandler &handler);
		void onChannelListReceived ( ChannelListReceivedHandler &handler);
		void onNameSet             (             NameSetHandler &handler);
		void onNameChanged         (         NameChangedHandler &handler);
		void onNameDenied          (          NameDeniedHandler &handler);
		void onChannelJoin         (         ChannelJoinHandler &handler);
		void onChannelJoinDenied   (   ChannelJoinDeniedHandler &handler);
		void onChannelLeave        (        ChannelLeaveHandler &handler);
		void onChannelLeaveDenied  (  ChannelLeaveDeniedHandler &handler);
		void onServerMessage       (       ServerMessageHandler &handler);
		void onServerChannelMessage(ServerChannelMessageHandler &handler);
		void onChannelMessage      (      ChannelMessageHandler &handler);
		void onPeerMessage         (         PeerMessageHandler &handler);
		void onPeerJoin            (            PeerJoinHandler &handler);
		void onPeerLeave           (           PeerLeaveHandler &handler);
		void onPeerChangeName      (      PeerChangeNameHandler &handler);

	private:
		struct
		{
			ErrorHandler                *Error;
			ConnectHandler              *Connect;
			ConnectionDeniedHandler     *ConnectionDenied;
			DisconnectHandler           *Disconnect;
			ChannelListReceivedHandler  *ChannelListReceived;
			NameSetHandler              *NameSet;
			NameChangedHandler          *NameChanged;
			NameDeniedHandler           *NameDenied;
			ChannelJoinHandler          *ChannelJoin;
			ChannelJoinDeniedHandler    *ChannelJoinDenied;
			ChannelLeaveHandler         *ChannelLeave;
			ChannelLeaveDeniedHandler   *ChannelLeaveDenied;
			ServerMessageHandler        *ServerMessage;
			ServerChannelMessageHandler *ServerChannelMessage;
			ChannelMessageHandler       *ChannelMessage;
			PeerMessageHandler          *PeerMessage;
			PeerJoinHandler             *PeerJoin;
			PeerLeaveHandler            *PeerLeave;
			PeerChangeNameHandler       *PeerChangeName;
		} handlers;

		lacewing::_pump * const msgPump;
		std::list<_channel *> listOfChannels;
		void CloseChannel(relay::_client::_channel * Channel);
		
		_client(_pump &);
		~_client();
	};
}

#endif //#ifndef __DarkwireSoftware_Relay_HeaderPlusPlus__
