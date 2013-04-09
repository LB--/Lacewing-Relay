#ifndef __DarkwireSoftware_Relay_HeaderPlusPlus__
#define __DarkwireSoftware_Relay_HeaderPlusPlus__

#include <cstdint>
#include <memory>

#include "lacewing.h"

namespace LwRelay
{
	/**
	 * Represents the ID of a client/peer.
	 * Due to protocol restrictions, the size is limited to 16 bits.
	 */
	using ID_t = uint16_t;
	/**
	 * Represents the ID of a subchannel.
	 * The protocol utilizes all 8 bits.
	 */
	using Subchannel_t = uint8_t;
	/**
	 * Represents which of 16 varieties of data is being transmitted.
	 * Due to protocol restrictions, the size is limited to 4 bits.
	 */
	using Variant_t = uint8_t;
	/**
	 * Represents all other scalar data types.
	 * Due to protocol limitations, and for consistency reasons, this
	 * type is limited to 32 bits.
	 */
	using Size_t = uint32_t;

	/**
	 * Implements a Lacewing Relay Server based on the latest protocol draft.
	 * https://github.com/udp/lacewing/blob/0.2.x/relay/current_spec.txt
	 */
	struct Server
	{
		void *Tag;

		/**
		 * Construct a new server from a pump, such as an event pump.
		 * The given pump must remain valid until after the destructor has
		 * been called.
		 */
		Server(lacewing::pump pump);
		/**
		 * Destructs this server.
		 */
		~Server() noexcept(true);

		Server(Server &&) noexcept(true) = default;
		Server &operator=(Server &&) noexcept(true);

		struct Channel;

		/**
		 * Represents a client connected to this server.
		 * Each client can be in multiple channels, and
		 * multiple clients can have the same name, but
		 * clients with the same name cannot be in the
		 * same channels at the same time. A client must
		 * set their name before being able to join any
		 * channels.
		 * Do not store long-term pointers to this.
		 */
		struct Client
		{
			void *Tag;

			~Client() noexcept(true);

			Client(Client &&) noexcept(true) = default;
			Client &operator=(Client &&) noexcept(true);

			/**
			 * Allows iteration of clients on this server.
			 * Intended for use with range-based for-loop.
			 */
			struct Iterator
			{
				~Iterator() noexcept(true);
				Iterator(Iterator &&) noexcept(true) = default;
				Iterator &operator=(Iterator &&) noexcept(true);

				Iterator begin() const;
				Iterator end() const;
				Iterator &operator++();
				bool operator!=(Iterator const&) const;
				Client &operator*() const;

			private:
				struct Impl;
				std::unique_ptr<Impl> impl;

				Iterator(Impl *);
				Iterator() = delete;
				Iterator(Iterator const&) = delete;
				Iterator &operator=(Iterator const &) = delete;

				friend struct ::LwRelay::Server;
			};

			/**
			 * Allows iteration of channels this client is connected to.
			 * Intended for use with range-based for-loop.
			 */
			struct ChannelIterator
			{
				~ChannelIterator() noexcept(true);
				ChannelIterator(ChannelIterator &&) noexcept(true) = default;
				ChannelIterator &operator=(ChannelIterator &&) noexcept(true);

				ChannelIterator begin() const;
				ChannelIterator end() const;
				ChannelIterator &operator++();
				bool operator!=(ChannelIterator const&) const;
				Channel &operator*() const;

			private:
				struct Impl;
				std::unique_ptr<Impl> impl;

				ChannelIterator(Impl *);
				ChannelIterator() = delete;
				ChannelIterator(ChannelIterator const&) = delete;
				ChannelIterator &operator=(ChannelIterator const&) = delete;

				friend struct ::LwRelay::Server::Client;
			};

			/**
			 * Returns true if this client is an HTTP client.
			 */
			bool IsHTTP() const;
			/**
			 * Returns this client's unique ID.
			 * IDs may be re-used.
			 */
			ID_t ID() const;
			/**
			 * Returns this client's name, or an empty string if not set.
			 */
			char const*Name() const;
			/**
			 * Sets this client's name to the given string.
			 * If set to an empty string, the client is kicked
			 * from all channels and must set their name again.
			 */
			void Name(char const*name);
			/**
			 * Returns the lacwing address of this client.
			 */
			lacewing::address Address();
			/**
			 * Forcefully disconnects this client.
			 */
			void Disconnect();
			/**
			 * Returns true if this client has indicated that they can use UDP/blasting.
			 */
			bool UsingUDP() const;
			/**
			 * Sends a server message to this client with the given data.
			 */
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
			/**
			 * Sends a server message to this client with the given string.
			 */
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);
			/**
			 * Returns the number of channels this client has joined.
			 */
			Size_t ChannelCount() const;
			/**
			 * Returns a ChannelIterator to iterate the channels this client is in.
			 * Intended for use with range-based for-loop.
			 */
			ChannelIterator Channels();

		private:
			struct Impl;
			std::unique_ptr<Impl> impl;

			Client(Impl *);
			Client() = delete;
			Client(Client const&) = delete;
			Client &operator=(Client const&) = delete;

			friend struct ::LwRelay::Server::Channel;
			friend struct ::LwRelay::Server;
		};	friend struct ::LwRelay::Server::Client;

		/**
		 * Represents a channel in this server.
		 * Channels only exist if they have clients in
		 * them - as soon as all clients leave a channel,
		 * or the channel master leaves without being reassigned
		 * and the channel is set to auto close, the channel
		 * will close and cease to exist.
		 * Do not store long-term pointers to this.
		 */
		struct Channel
		{
			void *Tag;

			~Channel() noexcept(true);

			Channel(Channel &&) noexcept(true) = default;
			Channel &operator=(Channel &&) noexcept(true);

			/**
			 * Allows iteration of channels on this server.
			 * Intended for use with range-based for-loop.
			 */
			struct Iterator
			{
				~Iterator() noexcept(true);
				Iterator(Iterator &&) noexcept(true) = default;
				Iterator &operator=(Iterator &&) noexcept(true);

				Iterator begin() const;
				Iterator end() const;
				Iterator &operator++();
				bool operator!=(Iterator const&) const;
				Channel &operator*() const;

			private:
				struct Impl;
				std::unique_ptr<Impl> impl;

				Iterator(Impl *);
				Iterator() = delete;
				Iterator(Iterator const&) = delete;
				Iterator &operator=(Iterator const &) = delete;

				friend struct ::LwRelay::Server;
			};

			/**
			 * Allows iteration of clients in this channel.
			 * Intended for use with range-based for-loop.
			 */
			struct ClientIterator
			{
				~ClientIterator() noexcept(true);
				ClientIterator(ClientIterator &&) noexcept(true) = default;
				ClientIterator &operator=(ClientIterator &&) noexcept(true);

				ClientIterator begin() const;
				ClientIterator end() const;
				ClientIterator &operator++();
				bool operator!=(ClientIterator const&) const;
				Client &operator*() const;

			private:
				struct Impl;
				std::unique_ptr<Impl> impl;

				ClientIterator(Impl *);
				ClientIterator() = delete;
				ClientIterator(ClientIterator const&) = delete;
				ClientIterator &operator=(ClientIterator const&) = delete;

				friend struct ::LwRelay::Server::Channel;
			};

			/**
			 * Returns this channel's unique ID.
			 * IDs may be re-used.
			 */
			ID_t ID() const;
			/**
			 * Returns the name of this channel.
			 */
			char const*Name() const;
			/**
			 * Sets the name of this channel without notifying existing members.
			 * Due to protocol limitations, changing a channel's name is not
			 * recognized by clients already in the channel.
			 */
			void Name(char const*name);
			/**
			 * Returns true if this channel is set to close when the channel master leaves.
			 */
			bool AutoClose() const;
			/**
			 * Sets whether this channel will close when the channel master leaves (true).
			 * This cannot be set to true if the channel does not have a channel master.
			 */
			void AutoClose(bool autoclose);
			/**
			 * Returns true if this channel is visible in the public channel list.
			 */
			bool Visible() const;
			/**
			 * Sets whether this channel is visible in the public channel list (true).
			 */
			void Visible(bool visible);
			/**
			 * Closes the channel, kicking out all clients from the channel.
			 */
			void Close();
			/**
			 * Sends a server channel message to this channel with the given data.
			 */
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
			/**
			 * Sends a server channel message to this channel with the given string.
			 */
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);
			/**
			 * Returns the channel master, or null if there is no channel master.
			 */
			Client *ChannelMaster();
			/**
			 * Sets/changes/removes the channel master unless the given client is not in the channel.
			 * Removing the channel master by passing null has no effect if
			 * the channel is set to auto close when the channel master leaves.
			 */
			void ChannelMaster(Client *member);
			/**
			 * Returns the number of clients in this channel.
			 */
			ID_t ClientCount() const;
			/**
			 * Returns a ClientIterator to iterate the clients in this channel.
			 * Intended for use with range-based for-loop.
			 */
			ClientIterator Clients() const;

		private:
			struct Impl;
			std::unique_ptr<Impl> impl;

			Channel(Impl *);
			Channel() = delete;
			Channel(Channel const&) = delete;
			Channel &operator=(Channel const&) = delete;

			friend struct ::LwRelay::Server::Client;
			friend struct ::LwRelay::Server;
		};	friend struct ::LwRelay::Server::Channel;

		/**
		 * Enable or disable the ability of clients to list visible channels.
		 */
		void SetChannelListing(bool enabled);
		/**
		 * Set the 'welcome message' sent to clients on connect.
		 */
		void SetWelcomeMessage(char const*message);
		/**
		 * Begin hosting this server on the given port, default 6121.
		 * This function fails if the server is already hosting or if
		 * one of the internal lacewing server fails to begin hosting.
		 */
		void Host(uint16_t port = 6121);
		/**
		 * Begin hosting this server using the given lacewing filter.
		 * This function fails if the server is already hosting or if
		 * one of the internal lacewing server fails to begin hosting.
		 */
		void Host(lacewing::filter filter);
		/**
		 * Returns true if the server is hosting, false otherwise.
		 */
		bool Hosting() const;
		/**
		 * Stops the server from hosting and disconnects all clients.
		 */
		void Unhost();
		/**
		 * Returns the number of connected clients.
		 */
		ID_t ClientCount() const;
		/**
		 * Returns an iterator over all the clients on the server.
		 * Intended for use with range-based for-loop.
		 */
		Client::Iterator Cients();
		/**
		 * Returns the number of open channels on the server.
		 */
		Size_t ChannelCount() const;
		/**
		 * Returns an iterator over all the channels on the server.
		 * Intended for use with range-based for-loop.
		 */
		Channel::Iterator Channels();
		/**
		 * Returns the port the server is being hosted on, in case you forgot.
		 */
		uint16_t Port() const;

		/**
		 * Represents an indication of or reason for denying a request.
		 * Some handlers let you return this to indicate whether the
		 * action should be allowed or denied. If constructed from a
		 * bool, true means allow and false means deny. If constructed
		 * from a c-string, it is implicitly meant to deny, using the
		 * given string as the reason given to the client.
		 */
		struct Deny
		{
			Deny(bool do_not_deny) noexcept(true);
			Deny(char const*reason) noexcept(true);
			~Deny() noexcept(true);
			Deny() = delete;
			Deny(Deny const&other) noexcept(true);
			Deny &operator=(Deny const&other) noexcept(true);
			Deny(Deny &&) noexcept(true) = default;
			Deny &operator=(Deny &&) noexcept(true);

		private:
			struct Impl;
			std::unique_ptr<Impl> impl;

			friend struct ::LwRelay::Server;
		};	friend struct ::LwRelay::Server::Deny;

		/* Handler prototypes *
		 * These are the handlers you can implement to customize
		 * the behavior of the server. For the most part you should
		 * be able to just copy & paste the prototypes, but you will
		 * need to properly scope some types.
		 */
		typedef void (lw_callback          ErrorHandler)(::LwRelay::Server &server, ::lacewing::error error);
		typedef Deny (lw_callback        ConnectHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client);
		typedef void (lw_callback     DisconnectHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client);
		typedef Deny (lw_callback        NameSetHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, char const*name);
		typedef Deny (lw_callback    JoinChannelHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel,               bool autoclose, bool visible);
		typedef Deny (lw_callback   LeaveChannelHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel);
		typedef void (lw_callback  ServerMessageHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client,                                                    Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
		typedef Deny (lw_callback ChannelMessageHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &client, ::LwRelay::Server::Channel &channel, bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
		typedef Deny (lw_callback    PeerMessageHandler)(::LwRelay::Server &server, ::LwRelay::Server::Client &from  , ::LwRelay::Server::Channel &channel, bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size, ::LwRelay::Server::Client &to);

		/* Handler setters *
		 * Register your handlers by passing them to
		 * these functions. You should not use & to take
		 * their address; they are passed by reference.
		 */
		void onError         (         ErrorHandler &handler); //Any kind of internal server error
		void onConnect       (       ConnectHandler &handler); //Client requests connection
		void onDisconnect    (    DisconnectHandler &handler); //Client diconnects
		void onNameSet       (       NameSetHandler &handler); //Client requests name set/change
		void onJoinChannel   (   JoinChannelHandler &handler); //Client requests to join a channel
		void onLeaveChannel  (  LeaveChannelHandler &handler); //Client requests to leave a channel
		void onServerMessage ( ServerMessageHandler &handler); //Client sends message to server
		void onChannelMessage(ChannelMessageHandler &handler); //Client requests to send message to channel
		void onPeerMessage   (   PeerMessageHandler &handler); //Client requests to send message to channel peer

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;

		Server() = delete;
		Server(Server const&) = delete;
		Server &operator=(Server const&) = delete;
	};

	/**
	 * Implements a Lacewing Relay Client based on the latest protocol draft.
	 * https://github.com/udp/lacewing/blob/0.2.x/relay/current_spec.txt
	 */
	struct Client
	{
		void *Tag;

		/**
		 * Construct a new client from a pump, such as an event pump.
		 * The given pump must remain valid until after the destructor has
		 * been called.
		 */
		Client(lacewing::pump pump);
		/**
		 * Destructs this client.
		 */
		~Client() noexcept(true);

		Client(Client &&) noexcept(true) = default;
		Client &operator=(Client &&) noexcept(true);

		/**
		 * Represents a channel that this client is in.
		 * Do not store long-term pointers to this.
		 */
		struct Channel
		{
			void *Tag;

			~Channel() noexcept(true);

			Channel(Channel &&) noexcept(true) = default;
			Channel &operator=(Channel &&) noexcept(true);

			/**
			 * Allows iteration of channels the client is in.
			 * Intended for use with range-based for-loop.
			 */
			struct Iterator
			{
				~Iterator() noexcept(true);
				Iterator(Iterator &&) noexcept(true) = default;
				Iterator &operator=(Iterator &&) noexcept(true);

				Iterator begin() const;
				Iterator end() const;
				Iterator &operator++();
				bool operator!=(Iterator const&) const;
				Channel &operator*() const;

			private:
				struct Impl;
				std::unique_ptr<Impl> impl;

				Iterator(Impl *);
				Iterator() = delete;
				Iterator(Iterator const&) = delete;
				Iterator &operator=(Iterator const &) = delete;

				friend struct ::LwRelay::Client;
			};

			/**
			 * Represents a peer in this channel.
			 * Do not store long-term pointers to this.
			 */
			struct Peer
			{
				void *Tag;

				~Peer() noexcept(true);

				Peer(Peer &&) noexcept(true) = default;
				Peer &operator=(Peer &&) noexcept(true);

				/**
				 * Allows iteration of peers in this channel.
				 * Intended for use with range-based for-loop.
				 */
				struct Iterator
				{
					~Iterator() noexcept(true);
					Iterator(Iterator &&) noexcept(true) = default;
					Iterator &operator=(Iterator &&) noexcept(true);

					Iterator begin() const;
					Iterator end() const;
					Iterator &operator++();
					bool operator!=(Iterator const&) const;
					Peer &operator*() const;

				private:
					struct Impl;
					std::unique_ptr<Impl> impl;

					Iterator(Impl *);
					Iterator() = delete;
					Iterator(Iterator const&) = delete;
					Iterator &operator=(Iterator const &) = delete;

					friend struct ::LwRelay::Client::Channel;
				};

				/**
				 * Returns the unique ID of this peer as assigned by the server.
				 */
				ID_t ID() const;
				/**
				 * Returns true if this peer is the channel master of this channel.
				 */
				bool IsChannelMaster() const;
				/**
				 * Requests to send the given data to this peer.
				 */
				void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
				/**
				 * Requests to send the given string to this peer.
				 */
				void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);
				/**
				 * Kicks this peer if you are the channel master.
				 * If you are not the channel master, this has
				 * no effect.
				 */
				void Kick();

			private:
				struct Impl;
				std::unique_ptr<Impl> impl;

				Peer(Impl *);
				Peer() = delete;
				Peer(Peer const&) = delete;
				Peer &operator=(Peer const&) = delete;

				friend struct ::LwRelay::Client;
				friend struct ::LwRelay::Client::Channel;
			};	friend struct ::LwRelay::Client::Channel::Peer;

			/**
			 * Returns the name of this channel.
			 */
			char const*Name() const;
			/**
			 * Returns the number of peers in this channel.
			 */
			ID_t PeerCount() const;
			/**
			 * Returns an iterator over the peers in this channel.
			 * Intended for use with range-based for-loop.
			 */
			Peer::Iterator Peers();
			/**
			 * Returns true if this client is the channel master of this channel.
			 */
			bool IsChannelMaster() const;
			/**
			 * Requests to send the given data to this channel.
			 */
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
			/**
			 * Requests to send the given string to this channel.
			 */
			void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);
			/**
			 * Requests to leave this channel.
			 */
			void Leave();

		private:
			struct Impl;
			std::unique_ptr<Impl> impl;
			
			Channel(Impl *);
			Channel() = delete;
			Channel(Channel const&) = delete;
			Channel &operator=(Channel const&) = delete;

			friend struct ::LwRelay::Client;
		};	friend struct ::LwRelay::Client::Channel;
			friend struct ::LwRelay::Client::Channel::Peer;

		struct ChannelListing
		{
			~ChannelListing() noexcept(true);

			ChannelListing(ChannelListing &&) noexcept(true) = default;
			ChannelListing &operator=(ChannelListing &&) noexcept(true);

			/**
			 * Returns the name of the listed channel.
			 */
			char const*Name() const;
			/**
			 * Returns the number of peers in the listed channel
			 */
			ID_t PeerCount() const;
			/**
			 * Returns the next listed channel, or null if this is the last.
			 */
			ChannelListing *Next();

		private:
			struct Impl;
			std::unique_ptr<Impl> impl;

			ChannelListing(Impl *);
			ChannelListing() = delete;
			ChannelListing(ChannelListing const&) = delete;
			ChannelListing &operator=(ChannelListing const&) = delete;

			friend struct ::LwRelay::Client;
		};	friend struct ::LwRelay::Client::ChannelListing;

		/**
		 * Connects to a server at the given host address and port (default 6121).
		 */
		void Connect(char const*host, uint16_t port = 6121);
		/**
		 * Connects to a server using the given lacewing address.
		 */
		void Connect(lacewing::address address);
		/**
		 * Returns true if this client is still waiting to connect.
		 */
		bool Connecting() const;
		/**
		 * Returns true if this client is currently connected to a server.
		 */
		bool Connected() const;
		/**
		 * Disconnects this client from the server.
		 */
		void Disconnect();
		/**
		 * Returns the address of the server in case you forgot it.
		 */
		lacewing::address ServerAddress();
		/**
		 * Returns the welcome message sent by the server.
		 */
		char const*WelcomeMessage();
		/**
		 * Returns the unique ID of this client assigned by the server.
		 */
		ID_t ID() const;
		/**
		 * Requests the list of public channels.
		 */
		void ListChannels();
		/**
		 * Returns the number of public listed channels received from the server.
		 */
		Size_t ChannelListingCount() const;
		/**
		 * Returns the first channel listing, or null if there were 0.
		 */
		ChannelListing *FirstChannelListing();
		/**
		 * Asks the server to assign the given name to this client.
		 */
		void Name(char const*name);
		/**
		 * Returns the name of this client, or an empty string if not set.
		 */
		char const*Name() const;
		/**
		 * Sends the given data to the server.
		 */
		void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
		/**
		 * Sends the given string to the server.
		 */
		void Send(bool blast, Subchannel_t subchannel, Variant_t variant, char const*null_terminated_string);
		/**
		 * Requests to join/create a channel with the given name.
		 */
		void Join(char const*channel, bool autoclose = false, bool visible = true);
		/**
		 * Returns the number of channels this client is in.
		 */
		Size_t ChannelCount() const;
		/**
		 * Returns an iterator over the channels this client is in.
		 * Intended for use with range-based for-loop.
		 */
		Channel::Iterator Channels();

		/* Handler prototypes *
		 * These are the handlers you can implement to customize
		 * the behavior of the client. For the most part you should
		 * be able to just copy & paste the prototypes, but you will
		 * need to properly scope some types.
		 */
		typedef void (lw_callback                ErrorHandler)(::LwRelay::Client &client, lacewing::error error);
		typedef void (lw_callback              ConnectHandler)(::LwRelay::Client &client);
		typedef void (lw_callback     ConnectionDeniedHandler)(::LwRelay::Client &client,                                                                                                  char const*reason);
		typedef void (lw_callback           DisconnectHandler)(::LwRelay::Client &client);
		typedef void (lw_callback  ChannelListReceivedHandler)(::LwRelay::Client &client);
		typedef void (lw_callback              NameSetHandler)(::LwRelay::Client &client);
		typedef void (lw_callback          NameChangedHandler)(::LwRelay::Client &client,                                                                              char const*oldname);
		typedef void (lw_callback           NameDeniedHandler)(::LwRelay::Client &client,                                                                              char const*thename, char const*reason);
		typedef void (lw_callback          ChannelJoinHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel);
		typedef void (lw_callback    ChannelJoinDeniedHandler)(::LwRelay::Client &client,                                                                              char const*thename, char const*reason);
		typedef void (lw_callback         ChannelLeaveHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel);
		typedef void (lw_callback   ChannelLeaveDeniedHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel,                                                             char const*reason);
		typedef void (lw_callback        ServerMessageHandler)(::LwRelay::Client &client,                                                                              bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
		typedef void (lw_callback ServerChannelMessageHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel,                                         bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
		typedef void (lw_callback       ChannelMessageHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer, bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
		typedef void (lw_callback          PeerMessageHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer, bool blasted, Subchannel_t subchannel, Variant_t variant, char const*data, Size_t size);
		typedef void (lw_callback             PeerJoinHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer);
		typedef void (lw_callback            PeerLeaveHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer);
		typedef void (lw_callback       PeerChangeNameHandler)(::LwRelay::Client &client, ::LwRelay::Client::Channel &channel, ::LwRelay::Client::Channel::Peer &peer, char const*oldname);

		/* Handler setters *
		 * Register your handlers by passing them to
		 * these functions. You should not use & to take
		 * their address; they are passed by reference.
		 */
		void onError               (               ErrorHandler &handler); //Any kind of internal client error
		void onConnect             (             ConnectHandler &handler); //Connection to the server
		void onConnectionDenied    (    ConnectionDeniedHandler &handler); //Connection to server refused
		void onDisconnect          (          DisconnectHandler &handler); //Disconnect from server
		void onChannelListReceived ( ChannelListReceivedHandler &handler); //Public channel list received
		void onNameSet             (             NameSetHandler &handler); //Name set for first time
		void onNameChanged         (         NameChangedHandler &handler); //Name changed
		void onNameDenied          (          NameDeniedHandler &handler); //Name set/change denied
		void onChannelJoin         (         ChannelJoinHandler &handler); //Joined channel
		void onChannelJoinDenied   (   ChannelJoinDeniedHandler &handler); //Channel join denied
		void onChannelLeave        (        ChannelLeaveHandler &handler); //Left channel
		void onChannelLeaveDenied  (  ChannelLeaveDeniedHandler &handler); //Channel leave denied
		void onServerMessage       (       ServerMessageHandler &handler); //Message from server
		void onServerChannelMessage(ServerChannelMessageHandler &handler); //Message from server in channel
		void onChannelMessage      (      ChannelMessageHandler &handler); //Message from channel
		void onPeerMessage         (         PeerMessageHandler &handler); //Message from peer in channel
		void onPeerJoin            (            PeerJoinHandler &handler); //Peer joined channel
		void onPeerLeave           (           PeerLeaveHandler &handler); //Peer left channel
		void onPeerChangeName      (      PeerChangeNameHandler &handler); //Peer changed name

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;

		Client() = delete;
		Client(Client const&) = delete;
		Client operator=(Client const&) = delete;
	};
}

#endif //#ifndef __DarkwireSoftware_Relay_HeaderPlusPlus__
