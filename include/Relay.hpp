#ifndef LacewingRelayClasses_HeaderPlusPlus
#define LacewingRelayClasses_HeaderPlusPlus

#include <lacewing.h>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <functional>

namespace lwrelay
{
	/**
	 * Represents the ID of a client/peer.
	 * Due to protocol restrictions, the size is limited to 16 bits.
	 */
	using ID_t = std::uint16_t;
	/**
	 * Represents the ID of a subchannel.
	 * The protocol utilizes all 8 bits.
	 */
	using Subchannel_t = std::uint8_t;
	/**
	 * Represents which of 16 varieties of data is being transmitted.
	 * Due to protocol restrictions, the size is limited to 4 bits.
	 */
	using Variant_t = std::uint8_t;
	/**
	 * Represents all other scalar data types.
	 * Due to protocol limitations, and for consistency reasons, this
	 * type is limited to 32 bits.
	 */
	using Size_t = std::uint32_t;

	/**
	 * The supported protocols.
	 */
	enum struct Protocol
	{
		TCP,
		UDP
	};

	/**
	 * Implements a Lacewing Relay Server based on the latest protocol draft.
	 * https://github.com/udp/lacewing/blob/0.2.x/relay/current_spec.txt
	 */
	struct Server final
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
		~Server();

		Server(Server &&) = default;
		Server &operator=(Server &&) noexcept;

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
		struct Client final
		{
			void *Tag;

			~Client();

			Client(Client &&) = default;
			Client &operator=(Client &&) noexcept;

			/**
			 * Returns true if this client is an HTTP client.
			 */
			bool IsHTTP() const noexcept;
			/**
			 * Returns this client's unique ID.
			 * IDs may be re-used.
			 */
			ID_t ID() const noexcept;
			/**
			 * Returns this client's name, or an empty string if not set.
			 */
			std::string const &Name() const noexcept;
			/**
			 * Sets this client's name to the given string.
			 * If set to an empty string, the client is kicked
			 * from all channels and must set their name again.
			 */
			void Name(std::string const &name);
			/**
			 * Returns the lacewing address of this client.
			 */
			lacewing::address Address();
			/**
			 * Forcefully disconnects this client.
			 */
			void Disconnect();
			/**
			 * Returns true if this client has indicated that they can use UDP/blasting.
			 */
			bool UsingUDP() const noexcept;
			/**
			 * Sends a server message to this client with the given data.
			 */
			void Send(Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data);

		private:
			struct Impl;
			std::unique_ptr<Impl> impl;

			Client(Impl *);
			Client() = delete;
			Client(Client const&) = delete;
			Client &operator=(Client const&) = delete;

			friend struct ::lwrelay::Server::Channel;
			friend struct ::lwrelay::Server;
		};	friend struct ::lwrelay::Server::Client;
		using Clients_t = std::map<ID_t, std::reference_wrapper<Client>>;

		/**
		 * Represents a channel in this server.
		 * Channels only exist if they have clients in
		 * them - as soon as all clients leave a channel,
		 * or the channel master leaves without being reassigned
		 * and the channel is set to auto close, the channel
		 * will close and cease to exist.
		 * Do not store long-term pointers to this.
		 */
		struct Channel final
		{
			void *Tag;

			~Channel();

			Channel(Channel &&) = default;
			Channel &operator=(Channel &&) noexcept;

			/**
			 * Returns this channel's unique ID.
			 * IDs may be re-used.
			 */
			ID_t ID() const noexcept;
			/**
			 * Returns the name of this channel.
			 */
			std::string const &Name() const noexcept;
			/**
			 * Sets the name of this channel without notifying existing members.
			 * Due to protocol limitations, changing a channel's name is not
			 * recognized by clients already in the channel.
			 */
			void Name(std::string const &name);
			/**
			 * Returns true if this channel is set to close when the channel master leaves.
			 */
			bool AutoClose() const noexcept;
			/**
			 * Sets whether this channel will close when the channel master leaves (true).
			 * This cannot be set to true if the channel does not have a channel master.
			 */
			void AutoClose(bool autoclose);
			/**
			 * Returns true if this channel is visible in the public channel list.
			 */
			bool Visible() const noexcept;
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
			void Send(Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data);
			/**
			 * Returns the channel master, or null if there is no channel master.
			 */
			Clients_t::iterator ChannelMaster();
			/**
			 * Sets/changes/removes the channel master unless the given client is not in the channel.
			 * Removing the channel master by passing null has no effect if
			 * the channel is set to auto close when the channel master leaves.
			 */
			void ChannelMaster(Clients_t::iterator member);

		private:
			struct Impl;
			std::unique_ptr<Impl> impl;

			Channel(Impl *);
			Channel() = delete;
			Channel(Channel const&) = delete;
			Channel &operator=(Channel const&) = delete;

			friend struct ::lwrelay::Server::Client;
			friend struct ::lwrelay::Server;
		};	friend struct ::lwrelay::Server::Channel;
		using Channels_t = std::map<ID_t, std::reference_wrapper<Channel>>;

		/**
		 * Enable or disable the ability of clients to list visible channels.
		 */
		void SetChannelListing(bool enabled);
		/**
		 * Set the 'welcome message' sent to clients on connect.
		 */
		void SetWelcomeMessage(std::string const &message);
		/**
		 * Begin hosting this server on the given port, default 6121.
		 * This function fails if the server is already hosting or if
		 * one of the internal lacewing server fails to begin hosting.
		 */
		void Host(std::uint16_t port = 6121);
		/**
		 * Begin hosting this server using the given lacewing filter.
		 * This function fails if the server is already hosting or if
		 * one of the internal lacewing server fails to begin hosting.
		 */
		void Host(lacewing::filter filter);
		/**
		 * Returns true if the server is hosting, false otherwise.
		 */
		bool Hosting() const noexcept;
		/**
		 * Stops the server from hosting and disconnects all clients.
		 */
		void Unhost();
		/**
		 * Returns the port the server is being hosted on, in case you forgot.
		 */
		std::uint16_t Port() const noexcept;

		/**
		 * Represents an indication of or reason for denying a request.
		 * Some handlers let you return this to indicate whether the
		 * action should be allowed or denied. If constructed from a
		 * bool, true means allow and false means deny. If constructed
		 * from a string, it is implicitly meant to deny, using the
		 * given string as the reason given to the client.
		 */
		struct Deny final
		{
			Deny(bool do_not_deny) noexcept;
			Deny(std::string const &deny_reason) noexcept;
			~Deny();
			Deny() = delete;
			Deny(Deny const&other) noexcept;
			Deny &operator=(Deny const&other) noexcept;
			Deny(Deny &&);
			Deny &operator=(Deny &&) noexcept;

		private:
			struct Impl;
			std::unique_ptr<Impl> impl;

			friend struct ::lwrelay::Server;
		};	friend struct ::lwrelay::Server::Deny;

		/* Handler prototypes *
		 * These are the handlers you can implement to customize
		 * the behavior of the server. For the most part you should
		 * be able to just copy & paste the prototypes, but you will
		 * need to properly scope some types.
		 */
		using          ErrorHandler = void (Server &server, lacewing::error error);
		using        ConnectHandler = Deny (Server &server, Client              &client);
		using     DisconnectHandler = void (Server &server, Client              &client);
		using        NameSetHandler = Deny (Server &server, Clients_t::iterator &client, std::string &name);
		using    JoinChannelHandler = Deny (Server &server, Clients_t::iterator &client, Channels_t::iterator &channel, bool &autoclose, bool &visible);
		using   LeaveChannelHandler = Deny (Server &server, Clients_t::iterator &client, Channels_t::iterator &channel);
		using  ServerMessageHandler = void (Server &server, Client              &client,                                Protocol  protocol, Subchannel_t  subchannel, Variant_t  variant, std::string const &data);
		using ChannelMessageHandler = Deny (Server &server, Clients_t::iterator &client, Channels_t::iterator &channel, Protocol &protocol, Subchannel_t &subchannel, Variant_t &variant, std::string       &data);
		using    PeerMessageHandler = Deny (Server &server, Clients_t::iterator &from  , Channels_t::iterator &channel, Protocol &protocol, Subchannel_t &subchannel, Variant_t &variant, std::string       &data, Clients_t::iterator &to);

		/* Handler setters *
		 * Register your handlers by passing them to
		 * these functions.
		 */
		void onError         (std::function<         ErrorHandler> handler); //Any kind of internal server error
		void onConnect       (std::function<       ConnectHandler> handler); //Client requests connection
		void onDisconnect    (std::function<    DisconnectHandler> handler); //Client disconnects
		void onNameSet       (std::function<       NameSetHandler> handler); //Client requests name set/change
		void onJoinChannel   (std::function<   JoinChannelHandler> handler); //Client requests to join a channel
		void onLeaveChannel  (std::function<  LeaveChannelHandler> handler); //Client requests to leave a channel
		void onServerMessage (std::function< ServerMessageHandler> handler); //Client sends message to server
		void onChannelMessage(std::function<ChannelMessageHandler> handler); //Client requests to send message to channel
		void onPeerMessage   (std::function<   PeerMessageHandler> handler); //Client requests to send message to channel peer

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
	struct Client final
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
		~Client();

		Client(Client &&) = default;
		Client &operator=(Client &&) noexcept;

		/**
		 * Represents a channel that this client is in.
		 * Do not store long-term pointers to this.
		 */
		struct Channel final
		{
			void *Tag;

			~Channel();

			Channel(Channel &&) = default;
			Channel &operator=(Channel &&) noexcept;

			/**
			 * Represents a peer in this channel.
			 * Do not store long-term pointers to this.
			 */
			struct Peer final
			{
				void *Tag;

				~Peer();

				Peer(Peer &&) = default;
				Peer &operator=(Peer &&) noexcept;

				/**
				 * Returns the unique ID of this peer as assigned by the server.
				 */
				ID_t ID() const noexcept;
				/**
				 * Returns true if this peer is the channel master of this channel.
				 */
				bool IsChannelMaster() const noexcept;
				/**
				 * Requests to send the given data to this peer.
				 */
				void Send(Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data);
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

				friend struct ::lwrelay::Client;
				friend struct ::lwrelay::Client::Channel;
			};	friend struct ::lwrelay::Client::Channel::Peer;
			using Peers_t = std::map<ID_t, std::reference_wrapper<Peer>>;

			/**
			 * Returns the name of this channel.
			 */
			std::string const &Name() const noexcept;
			/**
			 * Returns this channel's unique ID.
			 */
			ID_t ID() const noexcept;
			/**
			 * Returns true if this client is the channel master of this channel.
			 */
			bool IsChannelMaster() const noexcept;
			/**
			 * Requests to send the given data to this channel.
			 */
			void Send(Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data);
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

			friend struct ::lwrelay::Client;
		};	friend struct ::lwrelay::Client::Channel;
			friend struct ::lwrelay::Client::Channel::Peer;
		using Channels_t = std::map<ID_t, std::reference_wrapper<Channel>>;

		struct ListedChannelInfo final
		{
			std::size_t peers;
		};
		using ChannelListing = std::map<std::string, ListedChannelInfo>;

		/**
		 * Connects to a server at the given host address and port (default 6121).
		 */
		void Connect(std::string const &host, std::uint16_t port = 6121);
		/**
		 * Connects to a server using the given lacewing address.
		 */
		void Connect(lacewing::address address);
		/**
		 * Returns true if this client is still waiting to connect.
		 */
		bool Connecting() const noexcept;
		/**
		 * Returns true if this client is currently connected to a server.
		 */
		bool Connected() const noexcept;
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
		std::string const &WelcomeMessage();
		/**
		 * Returns the unique ID of this client assigned by the server.
		 */
		ID_t ID() const noexcept;
		/**
		 * Requests the list of public channels.
		 */
		void ListChannels();
		/**
		 * Returns the most recent channel listing.
		 */
		ChannelListing const &ListedChannels() const noexcept;
		/**
		 * Asks the server to assign the given name to this client.
		 */
		void Name(std::string const &name);
		/**
		 * Returns the name of this client, or an empty string if not set.
		 */
		std::string const &Name() const noexcept;
		/**
		 * Sends the given data to the server.
		 */
		void Send(Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data);
		/**
		 * Requests to join/create a channel with the given name.
		 */
		void Join(std::string const &channel, bool autoclose = false, bool visible = true);

		/* Handler prototypes *
		 * These are the handlers you can implement to customize
		 * the behavior of the client. For the most part you should
		 * be able to just copy & paste the prototypes, but you will
		 * need to properly scope some types.
		 */
		using                ErrorHandler = void (Client &client, lacewing::error error);
		using              ConnectHandler = void (Client &client);
		using     ConnectionDeniedHandler = void (Client &client,                                                                     std::string const &reason);
		using           DisconnectHandler = void (Client &client);
		using  ChannelListReceivedHandler = void (Client &client);
		using              NameSetHandler = void (Client &client);
		using          NameChangedHandler = void (Client &client,                                        std::string const &old_name);
		using           NameDeniedHandler = void (Client &client,                                        std::string const &the_name, std::string const &reason);
		using          ChannelJoinHandler = void (Client &client, Channel &channel);
		using    ChannelJoinDeniedHandler = void (Client &client,                                        std::string const &the_name, std::string const &reason);
		using         ChannelLeaveHandler = void (Client &client, Channel &channel);
		using   ChannelLeaveDeniedHandler = void (Client &client, Channel &channel,                                                   std::string const &reason);
		using        ServerMessageHandler = void (Client &client,                                        Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data);
		using ServerChannelMessageHandler = void (Client &client, Channel &channel,                      Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data);
		using       ChannelMessageHandler = void (Client &client, Channel &channel, Channel::Peer &peer, Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data);
		using          PeerMessageHandler = void (Client &client, Channel &channel, Channel::Peer &peer, Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data);
		using             PeerJoinHandler = void (Client &client, Channel &channel, Channel::Peer &peer);
		using            PeerLeaveHandler = void (Client &client, Channel &channel, Channel::Peer &peer);
		using       PeerChangeNameHandler = void (Client &client, Channel &channel, Channel::Peer &peer, std::string const &old_name);

		/* Handler setters *
		 * Register your handlers by passing them to
		 * these functions. You should not use & to take
		 * their address; they are passed by reference.
		 */
		void onError               (std::function<               ErrorHandler> handler); //Any kind of internal client error
		void onConnect             (std::function<             ConnectHandler> handler); //Connection to the server
		void onConnectionDenied    (std::function<    ConnectionDeniedHandler> handler); //Connection to server refused
		void onDisconnect          (std::function<          DisconnectHandler> handler); //Disconnect from server
		void onChannelListReceived (std::function< ChannelListReceivedHandler> handler); //Public channel list received
		void onNameSet             (std::function<             NameSetHandler> handler); //Name set for first time
		void onNameChanged         (std::function<         NameChangedHandler> handler); //Name changed
		void onNameDenied          (std::function<          NameDeniedHandler> handler); //Name set/change denied
		void onChannelJoin         (std::function<         ChannelJoinHandler> handler); //Joined channel
		void onChannelJoinDenied   (std::function<   ChannelJoinDeniedHandler> handler); //Channel join denied
		void onChannelLeave        (std::function<        ChannelLeaveHandler> handler); //Left channel
		void onChannelLeaveDenied  (std::function<  ChannelLeaveDeniedHandler> handler); //Channel leave denied
		void onServerMessage       (std::function<       ServerMessageHandler> handler); //Message from server
		void onServerChannelMessage(std::function<ServerChannelMessageHandler> handler); //Message from server in channel
		void onChannelMessage      (std::function<      ChannelMessageHandler> handler); //Message from channel
		void onPeerMessage         (std::function<         PeerMessageHandler> handler); //Message from peer in channel
		void onPeerJoin            (std::function<            PeerJoinHandler> handler); //Peer joined channel
		void onPeerLeave           (std::function<           PeerLeaveHandler> handler); //Peer left channel
		void onPeerChangeName      (std::function<      PeerChangeNameHandler> handler); //Peer changed name

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;

		Client() = delete;
		Client(Client const&) = delete;
		Client operator=(Client const&) = delete;
	};
}

#endif
