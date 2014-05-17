#include <Relay.hpp>

#include <set>
#include <vector>
#include <limits>
#iclude <sstream>

namespace lwrelay
{
	namespace
	{
		template<typename T>
		struct ID_Manager final
		{
			using ID_type = T;
			ID_type generate() noexcept
			{
				ID_type ret (lowest);
				while(IDs.find(++lowest) != IDs.end())
				{
				}
				return IDs.insert(ret), ret;
			}
			void release(ID_type ID) noexcept
			{
				IDs.erase(ID);
				lowest = (ID < lowest) ? ID : lowest;
			}

		private:
			std::set<ID_type> IDs;
			ID_type lowest = std::numeric_limits<ID_type>::min();
		};
		template<typename T>
		struct ID_Holder final
		{
			ID_Manager<T> &manager;
			T const id;

			ID_Holder(ID_Manager<T> &idm) noexcept
			: manager(idm)
			, id(idm.generate())
			{
			}
			~ID_Holder()
			{
				manager.release(id);
			}

			operator T() const noexcept
			{
				return id;
			}
		};
	}

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

		ID_Manager<ID_t> client_IDs, channel_IDs;
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
		ID_Holder<ID_t> id;
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
		ID_Holder<ID_t> id;
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
