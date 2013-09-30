#include <Relay.hpp>

#include <set>
#include <string>
#include <vector>

namespace LwRelay
{
	struct Client::Impl
	{
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

		lacewing::pump const pump;
		std::set<Channel *> channels;
	};
	struct Client::Channel::Impl
	{
		Client &client;
		std::set<Peer *> peers;
		ID_t id;
		std::string name;
	};
	struct Client::Channel::Peer::Impl
	{
		Channel &channel;
		ID_t id;
		std::string name;
	};
	struct Client::ChannelListing::Impl
	{
		//
	};
}
