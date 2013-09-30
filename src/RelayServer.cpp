#include <Relay.hpp>

#include <set>
#include <string>
#include <vector>
#include <map>
#include <limits>

namespace LwRelay
{
	namespace
	{
		template<typename T>
		struct ID_Manager
		{
			using ID_type = T;
			ID_type GenerateID()
			{
				ID_type ret (lowest);
				while(IDs.find(++lowest) != IDs.end())
				{
				}
				return IDs.insert(ret), ret;
			}
			void ReleaseID(ID_type ID)
			{
				IDs.erase(ID);
				lowest = (ID < lowest) ? ID : lowest;
			}
		private:
			std::set<ID_type> IDs;
			ID_type lowest = std::numeric_limits<ID_type>::min();
		};
	}

	struct Server::Impl
	{
		Server &interf;
		lacewing::pump pump;
		lacewing::server server {nullptr};
		lacewing::udp udp {nullptr};

		Impl(Server &ps, lacewing::pump p) : interf(ps), pump(p)
		{
			server = lacewing::server_new(pump);
			udp = lacewing::udp_new(pump);
		}
		Impl() = delete;
		Impl(Impl const &) = delete;
		Impl(Impl &&) noexcept(true) = default;
		Impl &operator=(Impl const &) = delete;
		Impl &operator=(Impl &&) noexcept(true) = default;
		~Impl() noexcept(true)
		{
			lacewing::udp_delete(udp), udp = nullptr;
			lacewing::server_delete(server), server = nullptr;
		}

		bool channel_listing {true};
		std::string welcome_message {lw_version()};

		ID_Manager<ID_t> client_IDs, channel_IDs;
		std::map<ID_t, Client *> clients;
		std::map<ID_t, Channel *> channels;

		struct Handlers
		{
				//Default handlers
				static void (lw_callback          ErrorDH)(Server &, lacewing::error)                                                                  {              }
				static Deny (lw_callback        ConnectDH)(Server &, Client &)                                                                         { return true; }
				static void (lw_callback     DisconnectDH)(Server &, Client &)                                                                         {              }
				static Deny (lw_callback        NameSetDH)(Server &, Client &, char const*)                                                            { return true; }
				static Deny (lw_callback    JoinChannelDH)(Server &, Client &, Channel &,       bool, bool)                                            { return true; }
				static Deny (lw_callback   LeaveChannelDH)(Server &, Client &, Channel &)                                                              { return true; }
				static void (lw_callback  ServerMessageDH)(Server &, Client &,                  Subchannel_t, Variant_t, char const*, Size_t)          {              }
				static Deny (lw_callback ChannelMessageDH)(Server &, Client &, Channel &, bool, Subchannel_t, Variant_t, char const*, Size_t)          { return true; }
				static Deny (lw_callback    PeerMessageDH)(Server &, Client &, Channel &, bool, Subchannel_t, Variant_t, char const*, Size_t, Client &){ return true; }

			         ErrorHandler *Error          {&         ErrorDH};
			       ConnectHandler *Connect        {&       ConnectDH};
			    DisconnectHandler *Disconnect     {&    DisconnectDH};
			       NameSetHandler *NameSet        {&       NameSetDH};
			   JoinChannelHandler *JoinChannel    {&   JoinChannelDH};
			  LeaveChannelHandler *LeaveChannel   {&  LeaveChannelDH};
			 ServerMessageHandler *ServerMessage  {& ServerMessageDH};
			ChannelMessageHandler *ChannelMessage {&ChannelMessageDH};
			   PeerMessageHandler *PeerMessage    {&   PeerMessageDH};
		} handlers;

		struct BinaryMessageHeader
		{
			struct Type
			{
				using Type_t = uint8_t;
				Variant_t variant : 4;
				Type_t type : 4;
			} type;
			Size_t size;
		};
	};
	Server &Server::operator=(Server &&) noexcept(true) = default;
	struct Server::Client::Impl
	{
		Server::Impl &server;
		lacewing::server_client client;
		ID_t id;

		Impl(Server::Impl &si, lacewing::server_client sc) : server(si), client(sc), id(si.client_IDs.GenerateID())
		{
		}

		std::string name;
		using Channels_t = std::vector<Channel *>;
		Channels_t channels;
	};
	struct Server::Client::ChannelIterator::Impl
	{
		std::vector<Channel *> channels;
	};
	struct Server::Channel::Impl
	{
		std::vector<Client *> clients;
		lacewing::client master;
		lacewing::server server;
		unsigned short id;
		char * name;
		bool closeWhenMasterLeaves, listInPublicChannelList;

		void Join(Client &client);
		void Leave(Client &client);
	};
	struct Server::Channel::ClientIterator::Impl
	{
		std::vector<Client *> clients;
	};
}
