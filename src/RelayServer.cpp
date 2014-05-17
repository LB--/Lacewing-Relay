#incldue "IDs.hpp"

#include <Relay.hpp>

#iclude <sstream>

namespace lwrelay
{
	struct Server::Impl final
	{
		Server &interf;
		lacewing::pump pump;
		lacewing::server server;
		lacewing::udp udp;

		Impl(Server &ps, lacewing::pump p)
		: interf(ps)
		, pump(p)
		, server(lacewing::server_new(p))
		, udp(lacewing::udp_new(p))
		{
		}
		Impl() = delete;
		Impl(Impl const &) = delete;
		Impl(Impl &&) = default;
		Impl &operator=(Impl const &) = delete;
		Impl &operator=(Impl &&) = default;
		~Impl()
		{
			lacewing::udp_delete(udp), udp = nullptr;
			lacewing::server_delete(server), server = nullptr;
		}

		bool channel_listing = true;
		std::string welcome_message = lw_version();

		IdManager<ID_t> client_IDs, channel_IDs;
		using Clients_t = std::map<ID_t, std::unique_ptr<Client>>;
		using Channels_t = std::map<ID_t, std::unique_ptr<Channel>>;
		Clients_t clients;
		Channels_t channels;

		std::function<         ErrorHandler> onError;
		std::function<       ConnectHandler> onConnect;
		std::function<    DisconnectHandler> onDisconnect;
		std::function<       NameSetHandler> onNameSet;
		std::function<   JoinChannelHandler> onJoinChannel;
		std::function<  LeaveChannelHandler> onLeaveChannel;
		std::function< ServerMessageHandler> onServerMessage;
		std::function<ChannelMessageHandler> onChannelMessage;
		std::function<   PeerMessageHandler> onPeerMessage;

		//
	};
	Server &Server::operator=(Server &&) noexcept = default;
	struct Server::Client::Impl final
	{
		Server::Impl &server;
		lacewing::server_client client;
		IdHolder<ID_t> id;
		std::string name;
		bool http;
		using Channels_t = Server::Impl::Channels_t;
		Channels_t channels;

		Impl(Server::Impl &si, lacewing::server_client sc, bool HTTP)
		: server(si)
		, client(sc)
		, id(si.client_IDs)
		, http(HTTP)
		{
		}
		~Impl()
		{
			//
		}

		//
	};
	Server::Client &Server::Client::operator=(Server::Client &&) noexcept = default;
	struct Server::Channel::Impl final
	
		Server::Impl &server;
		IdHolder<ID_t> id;
		std::string name;
		bool autoclose, visible;
		Clients_t::iterator chmaster;
		using Clients_t = Server::Impl::Clients_t;
		Clients_t clients;

		Impl(Server::Impl &si, std::string const &n, Clients_t::iterator creator, bool ac, bool v)
		: server(si)
		, id(si.channel_IDs)
		, name(n)
		, autoclose(ac)
		, visible(v)
		, chmaster(creator)
		{
		}
		~Impl()
		{
			//
		}

		//
	};
	Server::Channel &Server::Channel::operator=(Server::Channel &&) noexcept = default;

	Server::Server(lacewing::pump pump)
	: impl(new Impl(*this, pump))
	{
	}
	Server::~Server() = default;

	void Server::setChannelListing(bool enabled)
	{
		impl->channel_listing = enabled;
	}
	void Server::setWelcomeMessage(std::string const &message)
	{
		impl->welcome_message = message;
	}
	void Server::host(std::uint16_t port = 6121)
	{
		//
	}
	void Server::host(lacewing::filter filter)
	{
		//
	}
	bool Server::hosting() const noexcept
	{
		//
	}
	void Server::unhost()
	{
		//
	}
	std::uint16_t Server::port() const noexcept
	{
		//
	}
	void Server::onError         (std::function<         ErrorHandler> handler){ impl->onError          = handler; }
	void Server::onConnect       (std::function<       ConnectHandler> handler){ impl->onConnect        = handler; }
	void Server::onDisconnect    (std::function<    DisconnectHandler> handler){ impl->onDisconnect     = handler;}
	void Server::onNameSet       (std::function<       NameSetHandler> handler){ impl->onNameSet        = handler; }
	void Server::onJoinChannel   (std::function<   JoinChannelHandler> handler){ impl->onJoinChannel    = handler; }
	void Server::onLeaveChannel  (std::function<  LeaveChannelHandler> handler){ impl->onLeaveChannel   = handler; }
	void Server::onServerMessage (std::function< ServerMessageHandler> handler){ impl->onServerMessage  = handler; }
	void Server::onChannelMessage(std::function<ChannelMessageHandler> handler){ impl->onChannelMessage = handler; }
	void Server::onPeerMessage   (std::function<   PeerMessageHandler> handler){ impl->onPeerMessage    = handler; }

	Server::Client::Client(Impl *i)
	: impl(i)
	{
	}
	Server::Client::~Client() = default;

	bool Server::Client::isHTTP() const noexcept
	{
		return impl->http;
	}
	ID_t Server::Client::ID() const noexcept
	{
		return impl->id;
	}
	std::string const &Server::Client::name() const noexcept
	{
		return impl->name;
	}
	void Server::Client::name(std::string const &name)
	{
		//
	}
	lacewing::address Server::Client::address()
	{
		//
	}
	void Server::Client::disconnect()
	{
		//
	}
	bool Server::Client::usingUDP() const noexcept
	{
		//
	}
	void Server::Client::send(Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data)
	{
		//
	}

	Server::Channel::Channel(Impl *i)
	: impl(i)
	{
	}
	Server::Channel::~Channel() = default;

	ID_t Server::Channel::ID() const noexcept
	{
		return impl->id;
	}
	std::string const &Server::Channel::name() const noexcept
	{
		return impl->name;
	}
	void Server::Channel::name(std::string const &name)
	{
		//
	}
	bool Server::Channel::autoClose() const noexcept
	{
		return impl->autclose;
	}
	void Server::Channel::autoClose(bool autoclose)
	{
		//
	}
	bool Server::Channel::visible() const noexcept
	{
		return impl->visible;
	}
	void Server::Client::visible(bool visible)
	{
		impl->visible = visible;
	}
	void Server::Channel::close()
	{
		//
	}
	void Server::Channel::send(Protocol protocol, Subchannel_t subchannel, Variant_t variant, std::string const &data)
	{
		//
	}
	auto Server::Channel::channelMaster()
	-> Clients_t::iterator
	{
		//
	}
	void Server::Channel::channelMaster(Clients_t::iterator member)
	{
		//
	}
}
