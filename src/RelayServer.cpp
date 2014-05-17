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
		Client &interf;
		lacewing::server_client client;
		IdHolder<ID_t> id;
		std::string name;
		using Channels_t = Server::Impl::Channels_t;
		Channels_t channels;

		Impl(Server::Impl &si, Client &pc, lacewing::server_client sc)
		: server(si)
		, interf(pc)
		, client(sc)
		, id(si.client_IDs)
		{
		}
		~Impl()
		{
			//
		}

		//
	};
	struct Server::Channel::Impl final
	
		Server::Impl &server;
		Channel &interf;
		IdHolder<ID_t> id;
		std::string name;
		bool autoclose, visible;
		Clients_t::iterator chmaster;
		using Clients_t = Server::Impl::Clients_t;
		Clients_t clients;

		Impl(Server::Impl &si, Channel &pc, std::string const &n, Clients_t::iterator creator, bool ac, bool v)
		: server(si)
		, interf(pc)
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

	//
}
