#ifndef __DarkwireSoftware_Relay_HeaderPlusPlus__
#define __DarkwireSoftware_Relay_HeaderPlusPlus__

#include "lacewing.h"

namespace LwRelay
{
	typedef unsigned short ID_t;
	typedef unsigned char Subchannel_t;
	typedef unsigned char Variant_t; //limited to 4 bits

	struct Server
	{
		void *Tag;

		/**
		 * Construct a new server from a pump, such as an event pump.
		 */
		Server(lacewing::pump pump);
		~Server();

		size_t ClientCount() const;
		Client *FirstClient();
		size_t ChannelCount() const;
		Channel *FirstChannel();
		void SetChannelListing(bool enabled);
		void SetWelcomeMessage(char const*message);
		void Host(unsigned short port = 6121);
		void Host(lacewing::filter filter);
		void Unhost();
		bool Hosting() const;
		unsigned short Port() const;

		struct Client
		{
			void *Tag;

			struct ChannelIterator
			{
			};

			ID_t ID() const;
			char const*Name() const;
			void Name(char const*name);
			lacewing::address Address();
			void Disconnect();
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);
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

			friend struct ::LwRelay::Server::Channel;
			friend struct ::LwRelay::Server;
		};	friend struct ::LwRelay::Server::Client;

		struct Channel
		{
			void *Tag;

			struct ClientIterator
			{
				//
			private:
				struct Impl;
				Impl *impl;

				friend struct ::LwRelay::Server::Channel;
			};	friend struct ::LwRelay::Server::Channel::ClientIterator;

			ID_t ID() const;
			char const*Name() const;
			void Name(char const*name);
			bool AutoClose() const;
			void AutoClose(bool autoclose);
			bool Visible() const;
			void Visible(bool visible);
			void Close();
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);
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

			friend struct ::LwRelay::Server::Client;
			friend struct ::LwRelay::Server;
		};	friend struct ::LwRelay::Server::Channel;

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

			friend struct ::LwRelay::Server;
		};	friend struct ::LwRelay::Server::Deny;

		//fully-qualified names are used to be copy-paste friendly (except for "Deny")
		typedef void (lw_import          ErrorHandler)(::LwRelay::Server &server, ::lacewing::error error);
		typedef Deny (lw_import        ConnectHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client);
		typedef void (lw_import     DisconnectHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client);
		typedef Deny (lw_import        NameSetHandler)(::LwRelay::Server &Server, ::LwRelay::Server::Client &client, char const*name);
		typedef Deny (lw_import    JoinChannelHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel, bool autoclose, bool visible);
		typedef Deny (lw_import   LeaveChannelHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel);
		typedef void (lw_import  ServerMessageHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client,                                                    Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
		typedef Deny (lw_import ChannelMessageHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel, bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
		typedef Deny (lw_import    PeerMessageHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &from  , ::LwRelay::Server::Channel &channel, bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size, ::LwRelay::Server::Client &to);

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

		Client(lacewing::pump pump);
		~Client();

		void Connect(char const*host, unsigned short port = 6121);
		void Connect(lacewing::address address);
		bool Connecting() const;
		bool Connected() const;
		void Disconnect();
		lacewing::address ServerAddress();
		char const*WelcomeMessage();
		ID_t ID() const;
		void ListChannels();
		size_t ChannelListingCount() const;
		ChannelListing *FirstChannelListing();
		void Name(char const*name);
		char const*Name() const;
		void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
		void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);
		void Join(char const*channel, bool autoclose = false, bool visible = true);
		size_t ChannelCount() const;
		Channel *FirstChannel();

		struct Channel
		{
			void *Tag;

			char const*Name() const;
			Channel *Next();
			size_t PeerCount() const;
			Peer *FirstPeer();
			bool IsChannelMaster() const;
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);
			void Leave();

			struct Peer
			{
				void *Tag;

				ID_t ID() const;
				Peer *Next();
				bool IsChannelMaster() const;
				void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
				void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);

			private:
				struct Impl;
				Impl *impl;
				/*
				unsigned short id;
				std::list<_channel *> listOfChannels;
				const char * name;
				Client &client;
				Channel &channel;
				void Write(lacewing::stream Str, unsigned char Type, unsigned char Variant);
				*/

				Peer(Client &client, Channel &channel);
				#if __cplusplus >= 201103L // >= C++11
				Peer(Peer const&) = delete;
				Peer operator=(Peer const&) = delete;
				#else
				Peer(Peer const&);
				Peer operator=(Peer const&);
				#endif
				~Peer();

				friend struct ::LwRelay::Client;
				friend struct ::LwRelay::Client::Channel;
			};	friend struct ::LwRelay::Client::Channel::Peer;

		private:
			struct Impl;
			Impl *impl;
			/*
			std::list<_peer *> listOfPeers;
			Peer * master;
			Client * client;
			unsigned short id;
			char * name;
			bool closeWhenMasterLeaves, listInPublicChannelList;

			void PeerJoin(_client & Client, _client::_peer & Peer);
			void PeerLeave(_client & Client, _client::_peer & Peer);
			*/
			
			Channel(Client &client, char const*name);
			#if __cplusplus >= 201103L // >= C++11
			Channel(Channel const&) = delete;
			Channel operator=(Channel const&) = delete;
			#else
			Channel(Channel const&);
			Channel operator=(Channel const&);
			#endif
			~Channel();
			friend struct ::LwRelay::Client;
		};	friend struct ::LwRelay::Client::Channel;
			friend struct ::LwRelay::Client::Channel::Peer;

		struct ChannelListing
		{
			//
		private:
			struct Impl;
			Impl *impl;

			friend struct ::LwRelay::Client;
		};	friend struct ::LwRelay::Client::ChannelListing;

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
		typedef void (lw_import        ServerMessageHandler)(::LwRelay::Client &client,                                                                              bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
		typedef void (lw_import ServerChannelMessageHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel,                                         bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
		typedef void (lw_import       ChannelMessageHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer, bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
		typedef void (lw_import          PeerMessageHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer, bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, size_t size);
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
		struct Impl;
		Impl *impl;
		/*
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
		*/
		
		#if __cplusplus >= 201103L // >= C++11
		Client(Client const&) = delete;
		Client operator=(Client const&) = delete;
		#else
		Client(Client const&);
		Client operator=(Client const&);
		#endif
	};
}

#endif //#ifndef __DarkwireSoftware_Relay_HeaderPlusPlus__
