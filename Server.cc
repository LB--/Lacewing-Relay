#include "relay\Server.h"

namespace Lacewing {
namespace Relay {
/* Header */
void WriteHeader(Lacewing::Stream &Str, size_t Size, unsigned char Type, unsigned char Variant)
{
	if (Size < 0xFE)
		Str << char(Size & 0x000000FE);
	else if (Size < 0xFFFF)
		Str << char(0xFE) << (Size & 0x0000FFFF);
	else
	{
		/* Size > 0xFFFFFFFF not permitted; size_t can exceed this in 64-bit builds */
		assert(Size > 0xFFFFFFFF); 
		Str << char(0xFF) << Size;
	}

	/* These variables are `nybbles` and cannot exceed half-byte */
	assert(Variant > 0xF || Type > 0xF);
	Str << char(Type << 4 | Variant);
}
void ReadHeader(const char * Data, size_t Size, unsigned char &Type, unsigned char &Variant, const char * &RealData, size_t &RealSize)
{
	Type = Data[0] >> 4;
	Variant = Data[0] & 0x0F;

	if (Size < 2)
	{
		/* Hack attempt: malformed message header */
	}
	else if (Data[1] < 0xFE)
	{
		RealSize = size_t(Data[1]);
		RealData = &Data[2];
	}
	else if (Data[1] == 0xFE)
	{
		RealSize = (size_t)*(unsigned short *)(&Data[2]);
		RealData = &Data[4];
	}
	else // Data[1] == 0xFF
	{
		RealSize = *(size_t *)(&Data[2]);
		RealData = &Data[6];
	}
	
	// Size in Relay header doesn't match up with WinAPI size of message
	if (Data+Size > RealData)
	{
		/*	Hack attempt: malformed Size variable; sender is probably
			trying to cause a buffer overflow/underflow.
			d/c recommended. */
	}
}


// Raw handlers for liblacewing pass-thru to Relay functions
void onConnect_Relay(Lacewing::Server &server, Lacewing::Server::Client &client)
{
	client.Tag = new Server::Client(*ToRelay(&server), client);
	
	if (ToRelay(&server)->Handlers.onConnect &&
			!ToRelay(&server)->Handlers.onConnect(*ToRelay(&server), *ToRelay(&client)))
		client.Close();
}
void onDisconnect_Relay(Lacewing::Server &server, Lacewing::Server::Client &client)
{
	if (ToRelay(&server)->Handlers.onDisconnect)
		ToRelay(&server)->Handlers.onDisconnect(*ToRelay(&server), *ToRelay(&client));
	delete ToRelay(&client);
	client.Tag = NULL;
}
void onReceive_Relay(Lacewing::Server &server, Lacewing::Server::Client &client, const char * Msg, size_t MsgSize)
{
	// Read and post schematics (i.e. if channel message, call channel handler)
	unsigned char Type, Variant;
	const char * Data;
	size_t Size;
	Lacewing::FDStream Response(*ToRelay(&server)->MsgPump);
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
			Response << char(0) << Data[1];

			/*	0 - Connect request
				[string version] */
			if (Data[1] == 0)
			{
				/* Version is compatible */
				if (!strcmp("revision 3", &Data[2]))
				{
					/* [... short peerID, string welcomeMessage] */
					Response << char(true) << ToRelay(&client)->ID;
					if (ToRelay(&server)->WelcomeMessage)
						Response << ToRelay(&server)->WelcomeMessage;
					
					ToRelay(&client)->Write(Response, Type, Variant);
				}
				else
				{
					/*	[...]
						Client should be d/c'd after message */
					Response << char(false);
					
					ToRelay(&client)->Write(Response, Type, Variant);
					client.Close();
				}
			}
			/*	1 - Set name request
				[string name] */
			else if (Data[1] == 1)
			{
				/*	Response: [... byte nameLength, string Name, string DenyReason]
					DenyReason is blank, but nameLength and Name are always
					the same as at the request*/
				/* Name is blank */
				if (Data[2] == '\0')
				{
					/*	Client is attempting to set an empty name.
						Probably not a hack attempt, just the user being dumb */
					Response << char(false) << char(0) << char(0) << 
								"Channel name is blank. Sort your life out.";
				}
				/* Name is too long(max 255 due to use in nameLength */
				else if (Size > 2+255)
				{
					Response << char(false) << char(255);
					Response.Write(&Data[2], 255);
					Response << "Name is too long: maximum of 255 characters permitted.";
				}
				/* Name contains embedded nulls */
				else if (memchr(&Data[2], '\0', Size-2))
				{
						/*	Client is attempting to set a name with embedded nulls.
						Fail it. More secure servers should d/c the client. */
					Response << char(false) << char(Size-2);
					Response.Write(&Data[2], Size-2);
					Response << "Name contains invalid symbols (embedded nulls).";
				}
				/* Request is correctly formatted, so now we check if the server is okay with it */
				else
				{
					/*	Is name already in use? */
					Lacewing::Server::Client * C = server.FirstClient();
					while (C)
					{
						if (!memicmp(ToRelay(C)->Name, &Data[2], strlen(ToRelay(C)->Name)+1))
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
						assert(!NameDup);
						memcpy(NameDup, &Data[2], Size-2);
						NameDup[Size-2] = '\0';

						AllowedByUser = ToRelay(&server)->Handlers.onNameSet(*ToRelay(&server),
								*ToRelay(&client),
								(const char *)NameDup,
								DenyReason, FreeDenyReason);
					}
					Response << char(!C && AllowedByUser) << char(Size-2);
					Response.Write(&Data[2], Size-2);

					/* Write deny reason, if appropriate, then optionally free() it */
					if (C)
						Response << "Name is already in use.";
					if (!AllowedByUser)
						Response << (DenyReason ? DenyReason : "Custom server deny reason.");
					if (FreeDenyReason)
						free(DenyReason);
				}
				ToRelay(&client)->Write(Response, Type, Variant);
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
					Response << char(false) << char(0) << char(0) << 
								"Channel name is blank. Sort your life out.";
				}
				/* Name is too long (max 255 due to use in nameLength */
				else if (Size > 3+255)
				{
					Response << char(false) << char(255);
					Response.Write(&Data[3], 255);
					Response << "Channel name is too long: maximum of 255 characters permitted.";
				}
				/* Name contains embedded nulls */
				else if (memchr(&Data[3], '\0', Size-3))
				{
					/*	Client is attempting to set a name with embedded nulls.
						Fail it. More secure servers should d/c the client. */
					Response << char(false) << char(Size-3);
					Response.Write(&Data[3], Size-3);
					Response << "Name contains invalid symbols (embedded nulls).";
				}
				/* Request is correctly formatted, so now we check if the server is okay with it */
				else
				{
					/* Is channel with this name already exists already in use? */
					bool AllowedByUser = true, FreeDenyReason = false, ChannelExists = false;
					char * DenyReason = NULL;

					Server::Channel * C = NULL;
					for (List<Server::Channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
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
						char * NameDup = (char *)malloc(Size-3+1);
						assert(!NameDup);
						memcpy(NameDup, &Data[3], Size-3);
						NameDup[Size-3] = '\0';
						C = new Server::Channel(*ToRelay(&server), NameDup);
						free(NameDup);
					}
					
					if (ToRelay(&server)->Handlers.onJoinChannel)
						AllowedByUser = ToRelay(&server)->Handlers.onJoinChannel(*ToRelay(&server),
											*ToRelay(&client), false, *C, CloseWhenMasterLeaves, ShowInChannelList,
											DenyReason, FreeDenyReason);
						
					if (AllowedByUser)
					{
						C->listInPublicChannelList = ShowInChannelList;
						C->closeWhenMasterLeaves = CloseWhenMasterLeaves;
						C->Join(*ToRelay(&server), *ToRelay(&client));

						Response << char(true) << char(int(ShowInChannelList) | (int(CloseWhenMasterLeaves) << 1))
								 << char(Size-3) << C->Name << C->ID;

						if (!ChannelExists)
							ToRelay(&server)->listOfChannels.Push(C);
						else
						{
							for (List<Server::Client *>::Element * E = C->listOfPeers.First; E; E = E->Next)
							{
								if (long(**E) == long(&client))
									continue;
								Response << (**E)->ID << char(C->master == **E ? 1 : 0) << strlen((**E)->Name) << (**E)->Name;
							}
						}
					}
					/* Join channel denied by user */
					else
					{
						/* Write deny reason, then optionally free() it */
						Response << char(Size-3) << C->Name << (DenyReason ? DenyReason : "Custom server deny reason.");
						if (FreeDenyReason)
							free(DenyReason);
						delete C;
					}
				}
				ToRelay(&client)->Write(Response, Type, Variant);
			}
			/*	3 - Leave channel request
				[short ID] */
			else if (Data[1] == 3)
			{
				if (Size != 4)
				{
					/* Malformed message; hack attempt? */
					Response << char(false) << (Size < 4 ? unsigned short(-1) : *(unsigned short *)(&Data[2]))
							 << "Invalid channel leave message.";
				}
				/* Message format okay; check server is okay with the request */
				else
				{
					Server::Channel * C = NULL;
					for (List<Server::Channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
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
						Response << char(false) << *(unsigned short *)(&Data[2])
								 << "You're not connected to that channel, foo!";
					}
					/* Connected to the given channel ID; ask user if it's okay */
					else
					{
						bool AllowedByUser = true, FreeDenyReason = false;
						char * DenyReason = NULL;

						if (ToRelay(&server)->Handlers.onLeaveChannel)
							AllowedByUser = ToRelay(&server)->Handlers.onLeaveChannel(*ToRelay(&server),
												*ToRelay(&client), *C, DenyReason, FreeDenyReason);

						/* Write deny reason, if appropriate, then optionally free() it */
						Response << char(AllowedByUser) << C->ID;
						if (!AllowedByUser)
							Response << (DenyReason ? DenyReason : "Custom server deny reason.");
						if (FreeDenyReason)
							free(DenyReason);
					}
				}
				ToRelay(&client)->Write(Response, Type, Variant);
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
					Response << char(false) << "Malformed channel listing request.";
				}
				/* Server denied channel listing */
				else if (!ToRelay(&server)->EnableChannelListing)
				{
					Response << char(false) << "Channel listing is not enabled on this server.";
				}
				/* Allow channel listing */
				else
				{
					Response << char(true);
					for (List<Server::Channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
					{
						if (!(**E)->listInPublicChannelList)
							continue;
						Response << unsigned short((**E)->listOfPeers.Size) << strlen((**E)->Name) << (**E)->Name;
					}
				}
				ToRelay(&client)->Write(Response, Type, Variant);
			}
			/* Unrecognised request type */
			else
			{
				/*	Unrecognised message; suggests invalid version string.
					More secure servers should d/c the client. */
				Response << char(false) << "Unrecognised request type.";
				ToRelay(&client)->Write(Response, Type, Variant);
			}
		}
	}
	/*	1 - BinaryServerMessage
		[byte subchannel, ... data] */
	else if (Type == 1)
	{
		unsigned char Subchannel = *(unsigned char *)(&Data[1]);
		if (ToRelay(&server)->Handlers.onServerMessage)
			ToRelay(&server)->Handlers.onServerMessage(*ToRelay(&server), *ToRelay(&client), Variant,
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

			Server::Channel * C = NULL;
			for (List<Server::Channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
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
			else if (!C->listOfPeers.Find(ToRelay(&client)))
			{
				/* No way to report an error with sending channel messages */
			}
			/* Sender is on channel */
			else
			{
				bool AllowedByUser = true;
				if (ToRelay(&server)->Handlers.onChannelMessage)
					AllowedByUser = ToRelay(&server)->Handlers.onChannelMessage(*ToRelay(&server), *ToRelay(&client), false, *C, Variant,
													*(unsigned char *)(&Data[1]), (const char *)(&Data[4]), Size-5);
				if (!AllowedByUser)
				{
					/* No way to report an error with sending channel messages */
				}
				else
				{
					/*	We need to insert [short peer] before [binary message] */
					Response.Write(&Data[1], 3);
					Response << ToRelay(&client)->ID;
					Response.Write(&Data[4], Size-5);

					for (List<Server::Client *>::Element * E = C->listOfPeers.First; E; E = E->Next)
					{
						if ((**E)->container == &client)
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

		Server::Channel * C = NULL;
		for (List<Server::Channel *>::Element * E = ToRelay(&server)->listOfChannels.First; E; E = E->Next)
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
		else if (!C->listOfPeers.Find(ToRelay(&client)))
		{
			/* No way to report an error with sending peer messages */
		}
		/* Sender is on channel; is the receiver? */
		else
		{
			Server::Client * Recv = NULL;
			for (List<Server::Client *>::Element * E = C->listOfPeers.First; E; E = E->Next)
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
				if (ToRelay(&server)->Handlers.onPeerMessage)
					AllowedByUser = ToRelay(&server)->Handlers.onPeerMessage(*ToRelay(&server), *ToRelay(&client), false, *C,
													*Recv, Variant, *(unsigned char *)(&Data[1]), (const char *)&Data[6], Size-7);
				
				if (!AllowedByUser)
				{
					/* No way to report an error with sending peer messages */
				}
				else
				{
					/*	We need to insert [short peer] before [binary message] */
					Response.Write(&Data[1], 5);
					Response << ToRelay(&client)->ID;
					Response.Write(&Data[6], Size-7);

					Recv->Write(Response, Type, Variant);
				}
			}
		}
	}
	/* 4 - ObjectServerMessage */
	else if (Type == 4)
	{
		/* Mainly used with web requests */
		assert(false); /* Not coded */
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
		ToRelay(&client)->Write(Response, Type, Variant);
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
		Response << char(10);
		ToRelay(&client)->Write(Response, Type, Variant);
	}
	/* Unrecognised message type */
	else
	{
		/* Unrecognised message type; suggests invalid version string */
	}

	/* TODO: Test all messages. Implement UDP. */
}


void onError_Relay(Lacewing::Server &server, Error &error)
{
	if (ToRelay(&server)->Handlers.onError)
		ToRelay(&server)->Handlers.onError(*ToRelay(&server), error);
}




// Server protected functions: ID management
size_t Server::GetFreeID()
{
	// No abnormalies (gaps in the IDs of users) to fill up; simply return lowest available
	if (usedIDs.Size == 0)
		return lowestCleanID++;

	size_t ret = **usedIDs.First;
	usedIDs.PopFront();
	
	return ret;
}
void Server::SetFreeID(size_t ID)
{
	// If last ID of clients, we don't need to store the abnomaly
	if (ID == lowestCleanID-1)
	{
		--lowestCleanID;
		return;
	}
	
	// Otherwise, note the ID in list of abnormalies (organise numerically)
	for (List<size_t>::Element * E = usedIDs.First; E; E = E->Next)
	{
		if (**E > ID)
		{
			usedIDs.InsertBefore(E, ID);
			return;
		}
	}
	
	usedIDs.Push(ID);
}



// Raw liblacewing -> relay management
Server::Client * ToRelay(Lacewing::Server::Client * client)
{
	return (Server::Client *)client->Tag;
}
Server * ToRelay(Lacewing::Server * server)
{
	return (Server *)server->Tag;
}


// Channel functions
void Server::Channel::Join(Server &server, Server::Client &client)
{
	if (!listOfPeers.Size)
		master = &client;
	
	listOfPeers.Push(&client);
}
void Server::Channel::Leave(Server &server, Server::Client &client)
{
	if (master == &client)
	{
		master = NULL;
		if (closeWhenMasterLeaves)
		{
			server.CloseChannel(this);
			// Don't send message here; closeChannel --> ~Channel --> Leave() for this user
			return;
		}
	}

	// No peers left; run deconstructor via Server struct
	if (listOfPeers.Size == 1)
		server.CloseChannel(this);
	else
	{
		listOfPeers.Remove(&client);

		// Tell client leaving channel was successful, or that it was closed anyway.
		client.container->Write("Disconnection message; refer to autoclose?");
	}
}
Server::Channel::Channel(Server &server, const char * Name)
{
	this->server = &server;
	this->Name = strdup(Name);
	master = NULL;
	ID = server.listOfChannels.Size;
	closeWhenMasterLeaves = false;
	listInPublicChannelList = false;
}
Server::Channel::~Channel()
{
	for (List<Server::Client *>::Element * E = listOfPeers.First; E; E->Next)
		Leave(*server, ***E);

	free((void *)Name);
	Name = NULL;
	
	listOfPeers.Clear();
}



/* Client functions */
void Server::Client::Write(Lacewing::Stream &Str, unsigned char Type, unsigned char Variant)
{
	Lacewing::FDStream WithHeader(*this->server->MsgPump);
	WriteHeader(WithHeader, Str.Queued(), Type, Variant);
}
Server::Client::Client(Server &server, Lacewing::Server::Client &client)
{
	this->server = &server;
	Name = NULL;
	container = &client;
	ID = server.GetFreeID();
}


Server::Client::~Client()
{
	/* Send channel-left messages for all clients */
	for (List<Channel *>::Element * E = listOfChannels.First; E; E->Next)
		(**E)->Leave(*server, *this);
	
	listOfChannels.Clear();

	container->Tag = NULL;
	delete container;
	container = NULL;
}

// Server public functions
void Server::CloseChannel(Channel * channel)
{
	delete channel;
	listOfChannels.Remove(channel);
}

#define SetFuncPoint(name,pointtype)			\
void Server::name(pointtype functionPointer)	\
{												\
	Handlers.name = functionPointer;			\
}
SetFuncPoint(onConnect, HandlerConnectRelay)
SetFuncPoint(onDisconnect, HandlerDisconnectRelay)
SetFuncPoint(onNameSet, HandlerNameSetRelay)
SetFuncPoint(onError, HandlerErrorRelay)
SetFuncPoint(onJoinChannel, HandlerJoinChannelRelay)
SetFuncPoint(onLeaveChannel, HandlerLeaveChannelRelay)
SetFuncPoint(onServerMessage, HandlerServerMessageRelay)
SetFuncPoint(onChannelMessage, HandlerChannelMessageRelay)
SetFuncPoint(onPeerMessage, HandlerPeerMessageRelay)

Server::Server(Lacewing::Pump &Pump) : Lacewing::Server(Pump), MsgPump(&Pump)
{
	/*  For simplicity, all of Lacewing::Server's handler setting functions
		names are overriden in Server, so Relay variables can be used as
		handler parameters. So we need to utilise dynamic_cast to access Server's
		original set-handler parameters briefly, so we can set them to the proxy
		functions (which have _Relay at the end, seen above)
		The only handler that doesn't have a Server duplicate set-handler name
		is onReceive, since Relay doesn't support raw On Receive, it automatically
		calls onChannelMessage, onServerMessage, etc.
	*/
	
	// Set raw handlers to relay pass-thrus
	Lacewing::Server * This = dynamic_cast<Lacewing::Server *>(this);
	assert(This); 

	This->onConnect(onConnect_Relay);
	This->onDisconnect(onDisconnect_Relay);
	This->onReceive(onReceive_Relay);
	This->onError(onError_Relay);

	// Set relay handlers to disabled
	Handlers.onConnect = NULL;
	Handlers.onDisconnect = NULL;
	Handlers.onError = NULL;
	Handlers.onJoinChannel = NULL;
	Handlers.onLeaveChannel = NULL;
	Handlers.onServerMessage = NULL;
	Handlers.onChannelMessage = NULL;
	Handlers.onPeerMessage = NULL;
	
	// First available peer ID is 0
	lowestCleanID = 0;
	
	// No welcome message
	WelcomeMessage = NULL;
}
Server::~Server()
{
	Lacewing::Server::Client * A = FirstClient(), * B;
	while (A)
	{
		B = A->Next();
		delete ToRelay(A);
		A = B;
	}
	
	free(WelcomeMessage);
	usedIDs.Clear();
}

}
}