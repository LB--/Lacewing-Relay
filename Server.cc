#include "Relay.hpp"

#include <map>
#include <list>
#include <string>

#if defined(DEBUG) || defined(_DEBUG)
#define Assert(x) assert(x)
#else
#define Assert(x) /**/
#endif

namespace LwRelay
{
	struct Server::Impl
	{
		lacewing::pump const pump;
		lacewing::server const server;

		struct
		{
			         ErrorHandler *Error;
			       ConnectHandler *Connect;
			    DisconnectHandler *Disconnect;
			       NameSetHandler *NameSet;
			   JoinChannelHandler *JoinChannel;
			  LeaveChannelHandler *LeaveChannel;
			 ServerMessageHandler *ServerMessage;
			ChannelMessageHandler *ChannelMessage;
			   PeerMessageHandler *PeerMessage;
		} handlers;

		std::map<ID_t, Client> clients;
		size_t lowestCleanID;
		size_t GetFreeID();
		std::string welcomeMessage;
		bool enableChannelListing;
		
		void CloseChannel(Channel *channel);

		std::list<_channel *> listOfChannels;

		// Server protected functions: ID management
		size_t GetFreeID()
		{
			// No abnormalies (gaps in the IDs of users) to fill up; simply return lowest available
			if (usedIDs.Size == 0)
				return lowestCleanID++;

			size_t ret = **usedIDs.First;
			usedIDs.pop_front();
			
			return ret;
		}
		void SetFreeID(size_t ID)
		{
			// If last ID of clients, we don't need to store the abnomaly
			if (ID == lowestCleanID-1)
			{
				--lowestCleanID;
				return;
			}
			
			// Otherwise, note the ID in list of abnormalies (organise numerically)
			for (List<size_t>::Element * E = usedIDs.First; ; E = E->Next)
			{
				if (**E > ID)
				{
					usedIDs.InsertBefore(E, id);
					return;
				}
			}
			
			usedIDs.Push(ID);
		}
	};
	struct Server::Client::Impl
	{
		Server &server;
		lacewing::server_client const client;
		ID_t const id;
		char const*name;
		typedef std::vector<Channel *> Channels_t;
		Channels_t channels;
	};
	struct Server::Client::ChannelIterator::Impl
	{
		//
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
		//
	};

	namespace
	{
		void WriteHeader(lacewing::stream Str, size_t Size, unsigned char Type, unsigned char Variant)
		{
			if(Size < 0xFE)
			{
				Str->writef("%c", char(Size & 0x000000FE));
			}
			else if(Size < 0xFFFF)
			{
				Str->writef("%c%hu", char(0xFE), (Size & 0x0000FFFF));
			}
			else
			{
				/* Size > 0xFFFFFFFF not permitted; size_t can exceed this in 64-bit builds */
				Assert(Size > 0xFFFFFFFF);
				Str->writef("%c%u", char(0xFF), Size);
			}

			/* These variables are `nybbles` and cannot exceed half-byte */
			Assert(Variant > 0xF || Type > 0xF);
			Str->writef("%c", char(Type << 4 | Variant));
		}
		void ReadHeader(char const*Data, size_t Size,
						unsigned char &Type, unsigned char &Variant, char const*&RealData, size_t &RealSize)
		{
			Type = Data[0] >> 4;
			Variant = Data[0] & 0x0F;

			if(Size < 2)
			{
				/* Hack attempt: malformed message header */
			}
			else if(Data[1] < 0xFE)
			{
				RealSize = size_t(Data[1]);
				RealData = &Data[2];
			}
			else if(Data[1] == 0xFE)
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
			if(Data+Size > RealData)
			{
				/*	Hack attempt: malformed Size variable; sender is probably
					trying to cause a buffer overflow/underflow.
					d/c recommended. */
			}
		}


		// Raw handlers for liblacewing pass-thru to Relay functions
		void onConnect_Relay(lacewing::_server & Server, lacewing::_server_client & Client)
		{
			Client.Tag = new _server_client(*ToRelay(&Server), Client);
			
			if(ToRelay(&Server)->handlers.onConnect &&
					!ToRelay(&Server)->handlers.onConnect(*ToRelay(&Server), *ToRelay(&Client)))
				Client.close();
		}
		void onDisconnect_Relay(lacewing::_server & Server, lacewing::_server_client & Client)
		{
			if (ToRelay(&Server)->handlers.onDisconnect)
				ToRelay(&Server)->handlers.onDisconnect(*ToRelay(&Server), *ToRelay(&Client));
			delete ToRelay(&Client);
			Client.Tag = NULL;
		}
		void onReceive_Relay(lacewing::_server & Server, lacewing::_server_client & Client, const char * Msg, size_t MsgSize)
		{
			// Read and post schematics (i.e. if channel message, call channel handler)
			unsigned char Type, Variant;
			const char * Data;
			size_t Size;
			lacewing::_fdstream Response;
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
					Response.writef("%c%c", char(0), Data[1]);

					/*	0 - Connect request
						[string version] */
					if (Data[1] == 0)
					{
						/* Version is compatible */
						if (!strcmp("revision 3", &Data[2]))
						{
							/* [... short peerID, string welcomeMessage] */
							Response.writef("%c%hu", char(true), ToRelay(&Client)->id);
							if (ToRelay(&Server)->welcomeMessage)
								Response.write(ToRelay(&Server)->welcomeMessage);
							
							ToRelay(&Client)->Write(Response, Type, Variant);
						}
						else
						{
							/*	[...]
								Client should be d/c'd after message */
							Response.writef("%c", char(false));
							
							ToRelay(&Client)->Write(Response, Type, Variant);
							Client.close();
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
							Response.writef("%c%c%c%s", char(false), char(0), char(0),
										"Channel name is blank. Sort your life out.");
						}
						/* Name is too long(max 255 due to use in nameLength */
						else if (Size > 2+255)
						{
							Response.writef("%c%c%s.255", char(false), char(255), &Data[2],
								"Name is too long: maximum of 255 characters permitted.");
						}
						/* Name contains embedded nulls */
						else if (memchr(&Data[2], '\0', Size-2))
						{
								/*	Client is attempting to set a name with embedded nulls.
								Fail it. More secure servers should d/c the client. */
							Response.writef("%c%c%s.*%s", char(false), char(Size-2), &Data[2], Size-2,
								"Name contains invalid symbols (embedded nulls).");
						}
						/* Request is correctly formatted, so now we check if the server is okay with it */
						else
						{
							/*	Is name already in use? */
							lacewing::server_client C = Server.client_first();
							while (C)
							{
								if (!memicmp(ToRelay(C)->name, &Data[2], strlen(ToRelay(C)->name)+1))
									break;
								C = C->Next();
							}

							/* Name free to use is indicated by !C, invoke user handler */
							bool AllowedByUser = true, FreeDenyReason = false;
							char * DenyReason = NULL;
							
							if (!C && ToRelay(&Server)->handlers.onNameSet)
							{
								/* Name isn't null terminated, so duplicate & null-terminate before passing to handler */
								char * nameDup = (char *)malloc(Size-2+1);
								Assert(!nameDup);
								memcpy(nameDup, &Data[2], Size-2);
								nameDup[Size-2] = '\0';

								AllowedByUser = ToRelay(&Server)->handlers.onNameSet(*ToRelay(&Server),
										*ToRelay(&Client),
										(const char *)nameDup,
										DenyReason, FreeDenyReason);
							}
							Response.writef("%c%c%s.*", char(!C && AllowedByUser), char(Size-2), &Data[2], Size-2);

							/* Write deny reason, if appropriate, then optionally free() it */
							if (C)
								Response.write("Name is already in use.");
							if (!AllowedByUser)
								Response.write(DenyReason ? DenyReason : "Custom server deny reason.");
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
							Response.writef("%c%c%c%s", char(false), char(0), char(0),
										"Channel name is blank. Sort your life out.");
						}
						/* Name is too long (max 255 due to use in nameLength */
						else if (Size > 3+255)
						{
							Response.writef("%c%c%s.255%s", char(false), char(255), &Data[3],
								"Channel name is too long: maximum of 255 characters permitted.");
						}
						/* Name contains embedded nulls */
						else if (memchr(&Data[3], '\0', Size-3))
						{
							/*	Client is attempting to set a name with embedded nulls.
								Fail it. More secure servers should d/c the client. */
							Response.writef("%c%c%s.*%s", char(false), char(Size-3), &Data[3], Size-3,
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
								if (!memicmp((**E)->Name, &Data[3], strlen((**E)->name)+1))
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
								C = new _server::_channel(*ToRelay(&Server), nameDup);
								free(nameDup);
							}
							
							if (ToRelay(&Server)->handlers.onJoinChannel)
								AllowedByUser = ToRelay(&Server)->handlers.onJoinChannel(*ToRelay(&Server),
													*ToRelay(&Client), false, *C, CloseWhenMasterLeaves, ShowInChannelList,
													DenyReason, FreeDenyReason);
								
							if (AllowedByUser)
							{
								C->listInPublicChannelList = ShowInChannelList;
								C->closeWhenMasterLeaves = CloseWhenMasterLeaves;
								C->Join(*ToRelay(&Server), *ToRelay(&Client));

								Response.writef("%c%c%c%s%hu", char(true), char(int(ShowInChannelList) | (int(CloseWhenMasterLeaves) << 1)),
										 char(Size-3), C->name, C->id);

								if (!ChannelExists)
									ToRelay(&Server)->listOfChannels.Push(C);
								else
								{
									for (List<_server::_client *>::Element * E = C->listOfPeers.First; E; E = E->Next)
									{
										if (long(**E) == long(&Client))
											continue;
										Response << (**E)->ID << char(C->master == **E ? 1 : 0) << strlen((**E)->name) << (**E)->name;
									}
								}
							}
							/* Join channel denied by user */
							else
							{
								/* Write deny reason, then optionally free() it */
								Response.writef("%c%s%s", char(Size-3), C->name, (DenyReason ? DenyReason : "Custom server deny reason."));
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
							Response.writef("%c%hu%s", char(false), (Size < 4 ? unsigned short(-1) : *(unsigned short *)(&Data[2])),
									 "Invalid channel leave message.");
						}
						/* Message format okay; check server is okay with the request */
						else
						{
							_server::_channel * C = NULL;
							for (List<_server::_channel *>::Element * E = ToRelay(&Server)->listOfChannels.First; E; E = E->Next)
							{
								if ((**E)->id == *(unsigned short *)(&Data[2]))
								{
									C = **E;
									break;
								}
							}

							/* Not connected to the given channel ID */
							if (!C)
							{
								Response.writef("%c%hu%s", char(false), *(unsigned short *)(&Data[2]),
										 "You're not connected to that channel, foo!");
							}
							/* Connected to the given channel ID; ask user if it's okay */
							else
							{
								bool AllowedByUser = true, FreeDenyReason = false;
								char * DenyReason = NULL;

								if (ToRelay(&Server)->handlers.onLeaveChannel)
									AllowedByUser = ToRelay(&Server)->handlers.onLeaveChannel(*ToRelay(&Server),
														*ToRelay(&Client), *C, DenyReason, FreeDenyReason);

								/* Write deny reason, if appropriate, then optionally free() it */
								Response.writef("%c%hu", char(AllowedByUser), C->id);
								if (!AllowedByUser)
									Response.write(DenyReason ? DenyReason : "Custom server deny reason.");
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
							Response.writef("%c%s", char(false), "Malformed channel listing request.");
						}
						/* Server denied channel listing */
						else if (!ToRelay(&server)->EnableChannelListing)
						{
							Response.writef("%c%s", char(false), "Channel listing is not enabled on this server.");
						}
						/* Allow channel listing */
						else
						{
							Response.writef("%c", char(true));
							for (List<_server::_channel *>::Element * E = ToRelay(&Server)->listOfChannels.First; E; E = E->Next)
							{
								if (!(**E)->listInPublicChannelList)
									continue;
								Response.writef("%hu%u%s", unsigned short((**E)->listOfPeers.Size), strlen((**E)->name), (**E)->name);
							}
						}

						ToRelay(&Client)->Write(Response, Type, Variant);
					}
					/* Unrecognised request type */
					else
					{
						/*	Unrecognised message; suggests invalid version string.
							More secure servers should d/c the client. */
						Response.writef("%c%s", char(false), "Unrecognised request type.");
						ToRelay(&Client)->Write(Response, Type, Variant);
					}
				}
			}
			/*	1 - BinaryServerMessage
				[byte subchannel, ... data] */
			else if (Type == 1)
			{
				unsigned char Subchannel = *(unsigned char *)(&Data[1]);
				if (ToRelay(&Server)->handlers.onServerMessage)
					ToRelay(&Server)->handlers.onServerMessage(*ToRelay(&Server), *ToRelay(&Client), Variant,
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
					for (List<_server::_channel *>::Element * E = ToRelay(&Server)->listOfChannels.First; E; E = E->Next)
					{
						if ((**E)->id == ChannelID)
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
					else if (!C->listOfPeers.find(ToRelay(&Client)))
					{
						/* No way to report an error with sending channel messages */
					}
					/* Sender is on channel */
					else
					{
						bool AllowedByUser = true;
						if (ToRelay(&Server)->handlers.onChannelMessage)
							AllowedByUser = ToRelay(&Server)->handlers.onChannelMessage(*ToRelay(&Server), *ToRelay(&Client), false, *C, Variant,
															*(unsigned char *)(&Data[1]), (const char *)(&Data[4]), Size-5);
						if (!AllowedByUser)
						{
							/* No way to report an error with sending channel messages */
						}
						else
						{
							/*	We need to insert [short peer] before [binary message] */
							Response.writef("%s.3%hu%s.*", &Data[1], ToRelay(&Client)->id, &Data[4], Size-5);

							for (List<_server::_client *>::Element * E = C->listOfPeers.First; E; E = E->Next)
							{
								if ((**E)->container == &Client)
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
				for (List<_server::_channel *>::Element * E = ToRelay(&Server)->listOfChannels.First; E; E = E->Next)
				{
					if ((**E)->id == ChannelID)
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
						if ((**E)->id == PeerID)
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
						if (ToRelay(&Server)->handlers.onPeerMessage)
							AllowedByUser = ToRelay(&Server)->handlers.onPeerMessage(*ToRelay(&Server), *ToRelay(&Client), false, *C,
															*Recv, Variant, *(unsigned char *)(&Data[1]), (const char *)&Data[6], Size-7);
						
						if (!AllowedByUser)
						{
							/* No way to report an error with sending peer messages */
						}
						else
						{
							/*	We need to insert [short peer] before [binary message] */
							Response.writef("%s.5%hu%s.*", &Data[1], ToRelay(&Client)->id, &Data[6], Size-7);

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
				Response.writef("%c", char(10));
				ToRelay(&Client)->Write(Response, Type, Variant);
			}
			/* Unrecognised message type */
			else
			{
				/* Unrecognised message type; suggests invalid version string */
			}

			/* TODO: Test all messages. Implement UDP. */
		}


		void onError_Relay(lacewing::_server & Server, _error & Error)
		{
			if (ToRelay(&Server)->handlers.onError)
				ToRelay(&Server)->handlers.onError(*ToRelay(&Server), Error);
		}
	}

	  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Server::Server(lacewing::pump pump) : Tag(0), impl(new Impl)
	{
		//
	}


	// Channel functions
	void _server::_channel::Join(_server & Server, _server::_client & Client)
	{
		if (!listOfPeers.Size)
			master = &Client;
		
		listOfPeers.Push(&Client);
	}
	void _server::_channel::Leave(_server & Server, _server::_client & Client)
	{
		if (master == &Client)
		{
			master = NULL;
			if (closeWhenMasterLeaves)
			{
				Server.CloseChannel(this);
				// Don't send message here; closeChannel --> ~Channel --> Leave() for this user
				return;
			}
		}

		// No peers left; run deconstructor via Server struct
		if (listOfPeers.Size == 1)
			Server.CloseChannel(this);
		else
		{
			listOfPeers.Remove(&client);

			// Tell client leaving channel was successful, or that it was closed anyway.
			client.container->write("Disconnection message; refer to autoclose?");
		}
	}
	_server::_channel::_channel(_server & Server, const char * Name)
	{
		this->server = &Server;
		this->Name = strdup(Name);
		master = NULL;
		id = Server.listOfChannels.Size;
		closeWhenMasterLeaves = false;
		listInPublicChannelList = false;
	}
	_server::_channel::~_channel()
	{
		for (List<_server::_client *>::Element * E = listOfPeers.First; E; E->Next)
			Leave(*server, ***E);

		free((void *)name);
		name = NULL;
		
		listOfPeers.Clear();
	}



	/* Client functions */
	void _server::_client::Write(lacewing::_stream &Str, unsigned char Type, unsigned char Variant)
	{
		lacewing::_fdstream WithHeader;
		WriteHeader(WithHeader, Str.queued(), Type, Variant);
	}
	_server::_client::_client(_server & Server, lacewing::_server_client & Client)
	{
		this->server = &Server;
		name = NULL;
		container = &Client;
		id = Server.GetFreeID();
	}


	_server::_client::~_client()
	{
		/* Send channel-left messages for all clients */
		for (List<_channel *>::Element * E = listOfChannels.First; E; E->Next)
			(**E)->Leave(*server, *this);
		
		listOfChannels.Clear();

		container->Tag = NULL;
		delete container;
		container = NULL;
	}

	// Server public functions
	void _server::CloseChannel(_channel * Channel)
	{
		delete Channel;
		listOfChannels.Remove(Channel);
	}

	#define SetFuncPoint2(name,pointtype)			\
	void _client::name(pointtype functionPointer)	\
	{												\
		handlers.name = functionPointer;			\
	}
	SetFuncPoint2(onConnect, handlerConnectRelay)
	SetFuncPoint2(onDisconnect, handlerDisconnectRelay)
	SetFuncPoint2(onNameSet, handlerNameSetRelay)
	SetFuncPoint2(onError, handlerErrorRelay)
	SetFuncPoint2(onJoinChannel, handlerJoinChannelRelay)
	SetFuncPoint2(onLeaveChannel, handlerLeaveChannelRelay)
	SetFuncPoint2(onServerMessage, handlerServerMessageRelay)
	SetFuncPoint2(onChannelMessage, handlerChannelMessageRelay)
	SetFuncPoint2(onPeerMessage, handlerPeerMessageRelay)

	_server::_server(lacewing::_pump &Pump) : lacewing::_server(Pump), msgPump(&Pump)
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
		lacewing::_server * This = dynamic_cast<lacewing::_server *>(this);
		Assert(This); 

		This->onConnect(onConnect_Relay);
		This->onDisconnect(onDisconnect_Relay);
		This->onReceive(onReceive_Relay);
		This->onError(onError_Relay);

		// Set relay handlers to disabled
		handlers.onConnect = NULL;
		handlers.onDisconnect = NULL;
		handlers.onError = NULL;
		handlers.onJoinChannel = NULL;
		handlers.onLeaveChannel = NULL;
		handlers.onServerMessage = NULL;
		handlers.onChannelMessage = NULL;
		handlers.onPeerMessage = NULL;
		
		// First available peer ID is 0
		lowestCleanID = 0;
		
		// No welcome message
		welcomeMessage = NULL;
	}
	_server::~_server()
	{
		lacewing::server_client A = first_client(), * B;
		while (A)
		{
			B = A->Next();
			delete ToRelay(A);
			A = B;
		}
		
		free(welcomeMessage);
		usedIDs.Clear();
	}
}
