#include <Relay.hpp>

#iclude <sstream>

namespace lwrelay
{
	struct Client::Impl final
	{
		Client &interf;
		lacewing::pump pump;
		lacewing::client client;
		lacewing::udp udp;

		Impl(Client &pc, lacewing::pump p)
		: interf(pc)
		, pump(p)
		, client(lacewing::client_new(p))
		, udp(lacewing::udp_new(p))
		{
		}
		~Impl()
		{
			//
			lacewing::udp_delete(udp), udp = nullptr;
			lacewing::client_delete(client), client = nullptr;
		}

		//

		std::function<               ErrorHandler> onError;
		std::function<             ConnectHandler> onConnect;
		std::function<    ConnectionDeniedHandler> onConnectionDenied;
		std::function<          DisconnectHandler> onDisconnect;
		std::function< ChannelListReceivedHandler> onChannelListReceived;
		std::function<             NameSetHandler> onNameSet;
		std::function<         NameChangedHandler> onNameChanged;
		std::function<          NameDeniedHandler> onNameDenied;
		std::function<         ChannelJoinHandler> onChannelJoin;
		std::function<   ChannelJoinDeniedHandler> onChannelJoinDenied;
		std::function<        ChannelLeaveHandler> onChannelLeave;
		std::function<  ChannelLeaveDeniedHandler> onChannelLeaveDenied;
		std::function<       ServerMessageHandler> onServerMessage;
		std::function<ServerChannelMessageHandler> onServerChannelMessage;
		std::function<      ChannelMessageHandler> onChannelMessage;
		std::function<         PeerMessageHandler> onPeerMessage;
		std::function<            PeerJoinHandler> onPeerJoin;
		std::function<           PeerLeaveHandler> onPeerLeave;
		std::function<      PeerChangeNameHandler> onPeerChangeName;

		//
	};
	struct Client::Channel::Impl final
	{
		Client::Impl &client;
		ID_t const id;
		std::string name;
		using Peers_t = std::map<ID_t, std::unique_ptr<Peer>>;
		Peers_t peers;

		Impl(Client::Impl &ci, ID_t Id, std::string const &n)
		: client(ci)
		, id(Id)
		, name(n)
		{
		}
		~Impl()
		{
			//
		}

		//
	};
	struct Client::Channel::Peer::Impl final
	{
		Channel::Impl &channel;
		ID_t const id;
		std::string name;

		Impl(Channel::Impl &ci, ID_t Id, std::string const &n)
		: channel(ci)
		, id(Id)
		, name(n)
		{
		}
		~Impl()
		{
			//
		}

		//
	};
}
