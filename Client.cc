#include "Relay.hpp"

#if defined(DEBUG) || defined(_DEBUG)
#define Assert(x) assert(x)
#else
#define Assert(x) /**/
#endif

namespace lacewing {
namespace relay {
// Raw handlers for liblacewing pass-thru to Relay functions
void onConnect_RelayC(lacewing::_client & Client)
{
	if (ToRelay(&Client)->handlers.onConnect)
		ToRelay(&Client)->handlers.onConnect(*ToRelay(&Client));
}
void onDisconnect_RelayC(lacewing::_client & Client)
{
	if (ToRelay(&Client)->handlers.onDisconnect)
		ToRelay(&Client)->handlers.onDisconnect(*ToRelay(&Client), *ToRelay(&Client));
	delete ToRelay(&Client);
	client.Tag = NULL;
}
void onReceive_RelayC(lacewing::_client & Client, const char * Msg, size_t MsgSize)
{
	// Read and post schematics (i.e. if channel message, call channel handler)
	unsigned char Type, Variant;
	const char * Data;
	size_t Size;
	lacewing::fdstream Response;
	ReadHeader(Msg, MsgSize, Type, Variant, Data, Size);
	
	/*	0 - Request
		[byte type, ... data] */
	if (Type == 0)
	{
		/*	The response should be a match with type of request:
			[byte type, bool success, ...]
			type is written to the stream here; success later. */

		if (Size == 0)
		{
			/* Malformed message header; type is missing */
			return;
		}
		else // Message header is good for request
		{
			Response->writef("%c%c", char(0), Data[1]);

			/*	0 - Connect request
				[string version] */
			if (Data[1] == 0)
			{
				/* Version is compatible */
				if (!strcmp("revision 3", &Data[2]))
				{
					/* [... bool success, short peerID, string welcomeMessage] */
					Response->writef("%c%hu", char(true), ToRelay(&Client)->id);
					if (ToRelay(&Client)->welcomeMessage)
						Response->writef("%s", ToRelay(&Client)->welcomeMessage);
					
					ToRelay(&Client)->Write(Response, Type, Variant);
				}
				else
				{
					/*	[... bool success]
						Client should be d/c'd after message */
					Response->writef("%c", char(false));
					
					ToRelay(&Client)->Write(Response, Type, Variant);
					client.Close();
				}
			}
			/*	1 - Set name request
				[string name] */
			else if (Data[1] == 1)
			{
				/*	Response: [... bool success, byte nameLength, string Name,
							   string DenyReason]
					DenyReason is blank, but nameLength and Name are always
					the same as at the request*/
				
				/* Name is blank */
				if (Data[2] == '\0')
				{
					/*	Client is attempting to set an empty name.
						Probably not a hack attempt, just the user being dumb */
					Response->writef("%c%c%c%s", char(false), char(0), char(0),
								"Channel name is blank. Sort your life out.");
				}
				/* Name is too long(max 255 due to use in nameLength */
				else if (Size > 2+255)
				{
					Response->writef("%c%c%s.255%s", char(false), char(255), &Data[2],
						"Name is too long: maximum of 255 characters permitted.");
				}
				/* Name contains embedded nulls */
				else if (memchr(&Data[2], '\0', Size-2))
				{
					/*	Client is attempting to set a name with embedded nulls.
						Fail it. More secure servers should d/c the client. */
					Response->writef("%c%c%s.*%s", char(false), char(Size-2), &Data[2], Size-2,
						"Name contains invalid symbols (embedded nulls).");
				}
				/* Request is correctly formatted, so now we check if the server is okay with it */
				else
				{
					/*	Is name already in use? */
					lw_server_client C = server->FirstClient();
					while (C)
					{
						if (!memicmp(ToRelay(*C)->name, &Data[2], strlen(ToRelay(*C)->name)+1))
							break;
						C = C->Next();
					}

					/* Name free to use is indicated by !C, invoke user handler */
					bool AllowedByUser = true, FreeDenyReason = false;
					char * DenyReason = NULL;
					
					if (!C && ToRelay(&server)->Handlers.onNameSet)
					{
						/* Name isn't null terminated, so duplicate & null-terminate before passing to handler */
						char * NameDup = (char *)malloc(Size-2+1);
						Assert(!NameDup);
						memcpy(NameDup, &Data[2], Size-2);
						NameDup[Size-2] = '\0';

						AllowedByUser = ToRelay(server)->Handlers.onNameSet(*ToRelay(&server),
								*ToRelay(&Client),
								(const char *)NameDup,
								DenyReason, FreeDenyReason);
					}
					Response->writef("%c%c%s.*", char(!C && AllowedByUser), char(Size-2),
						&Data[2], Size-2);

					/* Write deny reason, if appropriate, then optionally free() it */
					if (C)
						Response->write("Name is already in use.");
					if (!AllowedByUser)
						Response->write(DenyReason ? DenyReason : "Custom server deny reason.");
					if (FreeDenyReason)
						free(DenyReason);
				}
				ToRelay(&Client)->Write(Response, Type, Variant);
			}
			/*	2 - Join channel request
				[byte flags, string name] */
			else if (Data[1] == 2)
			{
				/*	Response: 
					On success:
					[byte flags, byte nameLength, string name,
					short channel, ...]
					Then for each peer in the channel:
					[short id, byte flags, byte nameLength, string name]
					
					On failure:
					[byte NameLength, string name, string DenyReason] */
				bool ShowInChannelList = (Data[2] & 0x1) == 0,
					 CloseWhenMasterLeaves = (Data[2] & 0x2) != 0;
				
				/* Name is blank */
				if (Data[3] == '\0')
				{
					/*	Client is attempting to join a channel with an empty name.
						Probably not a hack attempt, just the user being dumb */
					Response->writef("%c%c%c%s", char(false), char(0), char(0),
								"Channel name is blank. Sort your life out.");
				}
				/* Name is too long (max 255 due to use in nameLength */
				else if (Size > 3+255)
				{
					Response->writef("%c%c%s.255%s", char(false), char(255), &Data[3],
						"Channel name is too long: maximum of 255 characters permitted.");
				}
				/* Name contains embedded nulls */
				else if (memchr(&Data[3], '\0', Size-3))
				{
					/*	Client is attempting to set a name with embedded nulls.
						Fail it. More secure servers should d/c the client. */
					Response->writef("%c%c%s.*%s", char(false), char(255), &Data[3], Size-3,
						"Name contains invalid symbols (embedded nulls).");
				}
				/* Request is correctly formatted, so now we check if the server is okay with it */
				else
				{
					/* Is channel with this name already exists already in use? */
					bool AllowedByUser = true, FreeDenyReason = false, ChannelExists = false;
					char * DenyReason = NULL;

					_server::_channel * C = NULL;
					for (List<_server::_channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
					{
						if (!memicmp((**E)->Name, &Data[3], strlen((**E)->Name)+1))
						{
							C = **E;
							ChannelExists = true;
							break;
						}
					}

					/* If channel isn't made, make it temporarily */
					if (!ChannelExists)
					{
						char * nameDup = (char *)malloc(Size-3+1);
						Assert(!nameDup);
						memcpy(nameDup, &Data[3], Size-3);
						nameDup[Size-3] = '\0';
						C = new _server::_channel(*ToRelay(&server), nameDup);
						free(nameDup);
					}
					
					if (ToRelay(&server)->handlers.onJoinChannel)
						AllowedByUser = ToRelay(&server)->handlers.onJoinChannel(*ToRelay(&server),
											*ToRelay(&Client), false, *C, CloseWhenMasterLeaves, ShowInChannelList,
											DenyReason, FreeDenyReason);
						
					if (AllowedByUser)
					{
						C->listInPublicChannelList = ShowInChannelList;
						C->closeWhenMasterLeaves = CloseWhenMasterLeaves;
						C->Join(*ToRelay(&server), *ToRelay(&Client));

						Response->writef("%c%c%c%s%hu", char(true), char(int(ShowInChannelList) | (int(CloseWhenMasterLeaves) << 1)),
								 char(Size-3), C->name, C->id);

						if (!ChannelExists)
							ToRelay(&server)->listOfChannels.Push(C);
						else
						{
							for (List<_server::_client *>::Element * E = C->listOfPeers.First; E; E = E->Next)
							{
								if (long(**E) == long(&Client))
									continue;

								Response->writef("%hu%c%u%s", (**E)->ID, char(C->master == **E ? 1 : 0), strlen((**E)->Name), (**E)->Name);
							}
						}
					}
					/* Join channel denied by user */
					else
					{
						/* Write deny reason, then optionally free() it */
						Response->writef("%c%s%c", char(Size-3), C->name, 
							(DenyReason ? DenyReason : "Custom server deny reason."));

						if (FreeDenyReason)
							free(DenyReason);

						delete C;
					}
				}
				ToRelay(&Client)->Write(Response, Type, Variant);
			}
			/*	3 - Leave channel request
				[short ID] */
			else if (Data[1] == 3)
			{
				if (Size != 4)
				{
					/* Malformed message; hack attempt? */
					Response->writef("%c%hu%s", char(false), (Size < 4 ? unsigned short(-1) : *(unsigned short *)(&Data[2])),
							 "Invalid channel leave message.");
				}
				/* Message format okay; check server is okay with the request */
				else
				{
					_server::_channel * C = NULL;
					for (List<_server::_channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
					{
						if ((**E)->ID == *(unsigned short *)(&Data[2]))
						{
							C = **E;
							break;
						}
					}

					/* Not connected to the given channel ID */
					if (!C)
					{
						Response->writef("%c%hu%s", char(false), *(unsigned short *)(&Data[2]),
								 "You're not connected to that channel, foo!");
					}
					/* Connected to the given channel ID; ask user if it's okay */
					else
					{
						bool AllowedByUser = true, FreeDenyReason = false;
						char * DenyReason = NULL;

						if (ToRelay(&server)->Handlers.onLeaveChannel)
							AllowedByUser = ToRelay(&server)->handlers.onLeaveChannel(*ToRelay(&server),
												*ToRelay(&Client), *C, DenyReason, FreeDenyReason);

						/* Write deny reason, if appropriate, then optionally free() it */
						Response->writef("%c%hu", char(AllowedByUser), C->id);
						if (!AllowedByUser)
							Response->write(DenyReason ? DenyReason : "Custom server deny reason.");
						if (FreeDenyReason)
							free(DenyReason);
					}
				}

				ToRelay(&Client)->Write(Response, Type, Variant);
			}
			/*	4 - ChannelList
				[no data] */
			else if (Data[1] == 4)
			{
				/*	Response:
					Success, for each channel:
					[short NumOfClients, byte NameLength, string ChannelName]

					Failure:
					[string DenyReason] */

				/* Size is invalid */
				if (Size > 2)
				{
					/* Malformed message; hack attempt? */
					Response->writef("%c%s", char(false), "Malformed channel listing request.");
				}
				/* Server denied channel listing */
				else if (!ToRelay(&server)->enableChannelListing)
				{
					Response->writef("%c%s", char(false), "Channel listing is not enabled on this server.");
				}
				/* Allow channel listing */
				else
				{
					Response->writef("%c", char(true));
					for (List<_server::_channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
					{
						if (!(**E)->listInPublicChannelList)
							continue;

						Response << unsigned short((**E)->listOfPeers.Size) << strlen((**E)->Name) << (**E)->Name;
					}
				}

				ToRelay(&Client)->Write(Response, Type, Variant);
			}
			/* Unrecognised request type */
			else
			{
				/*	Unrecognised message; suggests invalid version string.
					More secure servers should d/c the client. */
				Response->writef("%c%s", char(false), "Unrecognised request type.");
				ToRelay(&Client)->Write(Response, Type, Variant);
			}
		}
	}
	/*	1 - BinaryServerMessage
		[byte subchannel, ... data] */
	else if (Type == 1)
	{
		unsigned char Subchannel = *(unsigned char *)(&Data[1]);
		if (ToRelay(&server)->handlers.onServerMessage)
			ToRelay(&server)->handlers.onServerMessage(*ToRelay(&server), *ToRelay(&Client), Variant,
										*(unsigned char *)(&Data[1]), (const char *)&Data[2], Size-2);
	}
	/*	2 - BinaryChannelMessage
		[byte Subchannel, short ChannelID, binary Message] */
	else if (Type == 2)
	{
		/*	Response:
			[byte Subchannel, short ChannelID, short Peer, binary Message]

			No failure message accounted for in revision 6. */
		
		/* Size is invalid */
		if (Size < 3)
		{
			/* Malformed message; hack attempt? */
			/* No way to report an error with sending channel messages */
		}
		else
		{
			unsigned char Subchannel = *(unsigned char *)(&Data[1]);
			unsigned short ChannelID = *(unsigned short *)(&Data[2]);

			_server::_channel * C = NULL;
			for (List<_server::_channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
			{
				if ((**E)->ID == ChannelID)
				{
					C = **E;
					break;
				}
			}

			/* Channel ID non-existent */
			if (!C)
			{
				/* No way to report an error with sending channel messages */
			}
			/* Channel ID exists; sender not on channel */
			else if (!C->listOfPeers.Find(ToRelay(&Client)))
			{
				/* No way to report an error with sending channel messages */
			}
			/* Sender is on channel */
			else
			{
				bool AllowedByUser = true;
				if (ToRelay(&server)->Handlers.onChannelMessage)
					AllowedByUser = ToRelay(&server)->Handlers.onChannelMessage(*ToRelay(&server), *ToRelay(&Client), false, *C, Variant,
													*(unsigned char *)(&Data[1]), (const char *)(&Data[4]), Size-5);
				if (!AllowedByUser)
				{
					/* No way to report an error with sending channel messages */
				}
				else
				{
					/*	We need to insert [short peer] before [binary message] */
					Response->writef("%s.3%hu%s.*", &Data[1], ToRelay(&Client)->id, &Data[4], Size-5);

					for (List<_server::_client *>::Element * E = C->listOfPeers.First; E; E = E->Next)
					{
						if ((**E)->container == & Client)
							continue;

						(**E)->Write(Response, Type, Variant);
					}
				}
			}
		}
	}
	/*	3 - BinaryPeerMessage
		[byte subchannel, short channel, short peer, binary message] */
	else if (Type == 3)
	{
		/*	Response:
			[byte subchannel, short channel, short peer, binary message]

			No failure message accounted for in revision 6. */

		unsigned char Subchannel = *(unsigned char *)(&Data[1]);
		unsigned short ChannelID = *(unsigned short *)(&Data[2]),
						  PeerID = *(unsigned short *)(&Data[4]);

		_server::_channel * C = NULL;
		for (List<_server::_channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
		{
			if ((**E)->ID == ChannelID)
			{
				C = **E;
				break;
			}
		}

		/* Channel ID non-existent */
		if (!C)
		{
			/* No way to report an error with sending peer messages */
		}
		/* Channel ID exists; sender not on channel */
		else if (!C->listOfPeers.Find(ToRelay(&Client)))
		{
			/* No way to report an error with sending peer messages */
		}
		/* Sender is on channel; is the receiver? */
		else
		{
			_server::_client * Recv = NULL;
			for (List<_server::_client *>::Element * E = C->listOfPeers.First; E; E = E->Next)
			{
				if ((**E)->ID == PeerID)
				{
					Recv = **E;
					break;
				}
			}

			/* Receiver not found on channel */
			if (!Recv)
			{
				/* No way to report an error with sending peer messages */
			}
			else
			{
				bool AllowedByUser = true;
				if (ToRelay(&server)->handlers.onPeerMessage)
					AllowedByUser = ToRelay(&server)->handlers.onPeerMessage(*ToRelay(&server), *ToRelay(&Client), false, *C,
													*Recv, Variant, *(unsigned char *)(&Data[1]), (const char *)&Data[6], Size-7);
				
				if (!AllowedByUser)
				{
					/* No way to report an error with sending peer messages */
				}
				else
				{
					/*	We need to insert [short peer] before [binary message] */
					Response->writef("%s.5%hu%s.*", &Data[1], ToRelay(&Client)->id,
							&Data[6], Size-7);

					Recv->Write(Response, Type, Variant);
				}
			}
		}
	}
	/* 4 - ObjectServerMessage */
	else if (Type == 4)
	{
		/* Mainly used with web requests */
		Assert(false); /* Not coded */
		/* Server message: pass Data as a JSON-format string, &Data[2] */
	}
	/* 5 - ObjectChannelMessage */
	else if (Type == 5)
	{
		unsigned char Subchannel = *(unsigned char *)(&Data[1]);
		unsigned short ChannelID = *(unsigned short *)(&Data[2]);

		/* Channel message: pass Data as a JSON-format string, &Data[4] */
	}
	/* 6 - ObjectPeerMessage */
	else if (Type == 6)
	{
		unsigned char Subchannel = *(unsigned char *)(&Data[1]);
		unsigned short ChannelID = *(unsigned short *)(&Data[2]),
						  PeerID = *(unsigned short *)(&Data[4]);

		/* Peer message: pass Data as a JSON-format string, &Data[6] */
	}
	/*	7 - UDPHello
		[No format given in specs!] */
	else if (Type == 7)
	{
		/*	Reply with UDPWelcome
			[No format given in specs!] */
		ToRelay(&Client)->Write(Response, Type, Variant);
	}
	/* 8 - ChannelMaster */
	else if (Type == 8)
	{
		if (Size < 5)
		{
			/* Bla */
		}
		unsigned short ChannelID = *(unsigned short *)(&Data[2]);
		unsigned char Action = *(unsigned char *)(&Data[4]);
		unsigned short PeerID = *(unsigned short *)(&Data[5]);

		if (Action == 0)
		{
			/* Boot peer PeerID */
		}
		else
		{
			/* Unrecognised channel master command; suggests invalid version string */
		}
	}
	/* 9 - Ping */
	else if (Type == 9)
	{
		/* Reply with Pong */
		Response->writef("%c", char(10));
		ToRelay(&Client)->Write(Response, Type, Variant);
	}
	/* Unrecognised message type */
	else
	{
		/* Unrecognised message type; suggests invalid version string */
	}

	/* TODO: Test all messages. Implement UDP. */
}
void onError_RelayC(lacewing::_client & Client, error &error)
{
	if (ToRelay(&Client)->handlers.onError)
		ToRelay(&Client)->handlers.onError(*ToRelay(&Client), error);
}

// Raw liblacewing -> relay management
_client * ToRelay(lacewing::_client * Client)
{
	return (_client *)Client->Tag;
}

// Channel functions
void _client::_channel::PeerJoin(_server & Server, _server::_client & Client)
{
	if (!listOfPeers.Size)
		master = &Client;
	
	listOfPeers.Push(&Client);
}
void _client::_channel::PeerLeave(_server & Server, _server::_client & Client)
{
	if (master == &Client)
	{
		master = NULL;
		if (closeWhenMasterLeaves)
		{
			Client.CloseChannel(this);
			// Don't send message here; closeChannel --> ~Channel --> Leave() for this user
			return;
		}
	}

	// No peers left; run deconstructor via Server struct
	if (listOfPeers.Size == 1)
		Client.CloseChannel(this);
	else
	{
		listOfPeers.Remove(&Client);

		// Tell client leaving channel was successful, or that it was closed anyway.
		client.container->write("Disconnection message; refer to autoclose?");
	}
}
_client::_channel::_channel(_client & Client, const char * Name)
{
	this->client = &Client;
	this->name = strdup(Name);
	master = NULL;
	id = client.listOfChannels.Size;
	closeWhenMasterLeaves = false;
	listInPublicChannelList = false;
}
_client::_channel::~_channel()
{
	for (List<_client::_peer *>::Element * E = listOfPeers.First; E; E->Next)
		Leave(*Client, ***E);

	free((void *)name);
	name = NULL;
	
	listOfPeers.Clear();
}



/* Peer functions */
void _client::_peer::Write(lacewing::_stream & Str, unsigned char Type, unsigned char Variant)
{
	lacewing::_fdstream WithHeader(*this->server->MsgPump);
	WriteHeader(WithHeader, Str.queued(), Type, Variant);
}
_client::_peer::_peer(_server & Server, lacewing::_server_client & Client)
{
	this->server = &Server;
	name = NULL;
	container = &Client;
	id = Server.GetFreeID();
}


_client::_peer::~_peer()
{
	/* Send channel-left messages for all clients */
	for (List<_channel *>::Element * E = listOfChannels.First; E; E->Next)
		(**E)->Leave(*server, *this);
	
	listOfChannels.Clear();

	container->Tag = NULL;
	delete container;
	container = NULL;
}

// Client public functions
void _client::CloseChannel(_channel * Channel)
{
	delete Channel;
	listOfChannels.remove(Channel);
}

#define SetFuncPoint(name,pointtype)			\
void server::name(pointtype functionPointer)	\
{												\
	handlers.name = functionPointer;			\
}
SetFuncPoint(onConnect, handlerConnectRelay)
SetFuncPoint(onDisconnect, handlerDisconnectRelay)
SetFuncPoint(onNameSet, handlerNameSetRelay)
SetFuncPoint(onError, handlerErrorRelay)
SetFuncPoint(onJoinChannel, handlerJoinChannelRelay)
SetFuncPoint(onLeaveChannel, handlerLeaveChannelRelay)
SetFuncPoint(onServerMessage, handlerServerMessageRelay)
SetFuncPoint(onChannelMessage, handlerChannelMessageRelay)
SetFuncPoint(onPeerMessage, handlerPeerMessageRelay)

relay::client::client(lacewing::_pump & Pump) : lacewing::server(Pump), msgPump(&Pump)
{
	/*  For simplicity, all of lacewing::Server's handler setting functions
		names are overriden in Server, so Relay variables can be used as
		handler parameters. So we need to utilise dynamic_cast to access Server's
		original set-handler parameters briefly, so we can set them to the proxy
		functions (which have _Relay at the end, seen above)
		The only handler that doesn't have a Server duplicate set-handler name
		is onReceive, since Relay doesn't support raw On Receive, it automatically
		calls onChannelMessage, onServerMessage, etc.
	*/
	
	// Set raw handlers to relay pass-thrus
	_lw_client This = dynamic_cast<_lw_client>(this);
	Assert(This); 

	This->onConnect(onConnect_RelayC);
	This->onDisconnect(onDisconnect_RelayC);
	This->onReceive(onReceive_RelayC);
	This->onError(onError_RelayC);

	// Set relay handlers to disabled
	handlers.onConnect = NULL;
	handlers.onDisconnect = NULL;
	handlers.onError = NULL;
	handlers.onJoinChannel = NULL;
	handlers.onLeaveChannel = NULL;
	handlers.onServerMessage = NULL;
	handlers.onChannelMessage = NULL;
	handlers.onPeerMessage = NULL;
	
	// No welcome message
	welcomeMessage = NULL;
}
relay::_client::~_client()
{
}

} // namespace relay
} // namespace lacewing

