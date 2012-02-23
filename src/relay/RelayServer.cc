
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../Common.h"

#include "FrameReader.h"
#include "FrameBuilder.h"
#include "IDPool.h"

#include "../webserver/Common.h"

void ServerMessageHandler (void * Tag, unsigned char Type, char * Message, int Size);
static void TimerTick (Timer &Timer);

struct RelayServer::Internal
{
    RelayServer &Server;

    Lacewing::Server  Socket;
    Lacewing::UDP     UDP;
    Lacewing::Timer   Timer;

    struct
    {
        HandlerConnect           Connect;
        HandlerDisconnect        Disconnect;
        HandlerError             Error;
        HandlerServerMessage     ServerMessage;
        HandlerChannelMessage    ChannelMessage;
        HandlerPeerMessage       PeerMessage;
        HandlerJoinChannel       JoinChannel;
        HandlerLeaveChannel      LeaveChannel;
        HandlerSetName           SetName;

    } Handlers;

    Internal (RelayServer &_Server, Pump &Pump);

    ~ Internal ()
    {
        free (WelcomeMessage);
    }

    IDPool ClientIDs;
    IDPool ChannelIDs;
    
    Backlog <Client::Internal>   ClientBacklog;
    Backlog <Channel::Internal>  ChannelBacklog;

    FrameBuilder Builder;

    char * WelcomeMessage;

    List <RelayServer::Channel::Internal *> Channels;

    bool ChannelListingEnabled;
};

struct RelayServer::Client::Internal
{
    RelayServer::Client Public;

    Lacewing::Server::Client &Socket;
    RelayServer::Internal &Server;
    
    Internal (Lacewing::Server::Client &_Socket)
            : Server (*(RelayServer::Internal *) Socket.Tag), Socket (_Socket),
                UDPAddress (Socket.GetAddress ())
    {
        Public.internal = this;
        Public.Tag = 0;

        Reader.Tag            = this;
        Reader.MessageHandler = ServerMessageHandler;

        ID = Server.ClientIDs.Borrow ();

        Handshook      = false;
        Ponged         = true;
        GotFirstByte   = false;

        Name = strdup ("");
    }

    ~ Internal ()
    {
        Server.ClientIDs.Return (ID);  

        free (Name);
    }

    FrameReader Reader;
    
    void MessageHandler (unsigned char Type, char * Message, int Size, bool Blasted);

    List <RelayServer::Channel::Internal *> Channels;

    char * Name;
    bool NameAltered;

    bool CheckName (const char * Name);

    unsigned short ID;

    RelayServer::Channel::Internal * ReadChannel (MessageReader &Reader);

    bool Handshook;
    bool GotFirstByte;
    bool Ponged;

    Address UDPAddress;
};

struct RelayServer::Channel::Internal
{
    RelayServer::Channel Public;
    
    RelayServer::Internal &Server;

    List <RelayServer::Channel::Internal *>::Element * Element;

    Internal (RelayServer::Internal &_Server) : Server (_Server)
    {
        Public.internal    = this;
        Public.Tag            = 0;

        ID = Server.ChannelIDs.Borrow ();

        Name = strdup ("");
    }

    ~ Internal ()
    {
        Server.ChannelIDs.Return (ID);

        free (Name);
    }

    List <RelayServer::Client::Internal *> Clients;

    char * Name;

    unsigned short ID;

    bool Hidden;
    bool AutoClose;

    RelayServer::Client::Internal * ChannelMaster;
    RelayServer::Client::Internal * ReadPeer (MessageReader &Reader);
    
    void RemoveClient (RelayServer::Client::Internal &);
    void Close ();
};

void ServerMessageHandler (void * Tag, unsigned char Type, char * Message, int Size)
{   ((RelayServer::Client::Internal *) Tag)->MessageHandler (Type, Message, Size, false);
}

static void TimerTick (Lacewing::Timer &Timer)
{
    RelayServer::Internal * internal = (RelayServer::Internal *) Timer.Tag;

    Lacewing::Server &Socket = internal->Socket;
    List <RelayServer::Client::Internal *> ToDisconnect;

    internal->Builder.AddHeader (11, 0); /* Ping */
    
    for (Server::Client * ClientSocket = Socket.FirstClient (); ClientSocket; ClientSocket = ClientSocket->Next ())
    {
        RelayServer::Client::Internal &Client = *(RelayServer::Client::Internal *) ClientSocket->Tag;
        
        if (!Client.Ponged)
        {
            ToDisconnect.Push (&Client);
            continue;
        }

        Client.Ponged = false;
        internal->Builder.Send (Client.Socket, false);
    }

    internal->Builder.Reset ();

    for (List <RelayServer::Client::Internal *>::Element * E = ToDisconnect.First; E; E = E->Next)
        (** E)->Socket.Disconnect ();
}

RelayServer::Channel::Internal * RelayServer::Client::Internal::ReadChannel (MessageReader &Reader)
{
    int ChannelID = Reader.Get <unsigned short> ();

    if(Reader.Failed)
        return 0;

    for (List <RelayServer::Channel::Internal *>::Element * E = Channels.First; E; E = E->Next)
        if((** E)->ID == ChannelID)
            return ** E;
     
    Reader.Failed = true;
    return 0;
}

RelayServer::Client::Internal * RelayServer::Channel::Internal::ReadPeer (MessageReader &Reader)
{
    int PeerID = Reader.Get <unsigned short> ();

    if(Reader.Failed)
        return 0;

    for (List <RelayServer::Client::Internal *>::Element * E = Clients.First; E; E = E->Next)
        if((** E)->ID == PeerID)
            return ** E;
     
    Reader.Failed = true;
    return 0;
}

static void HandlerConnect (Server &Server, Server::Client &ClientSocket)
{
    RelayServer::Internal * internal = (RelayServer::Internal *) Server.Tag;

    ClientSocket.Tag = internal;
    ClientSocket.Tag = &internal->ClientBacklog.Borrow (ClientSocket);   
}

static void HandlerDisconnect (Server &Server, Server::Client &ClientSocket)
{
    RelayServer::Internal * internal = (RelayServer::Internal *) Server.Tag;
    RelayServer::Client::Internal &Client = *(RelayServer::Client::Internal *) ClientSocket.Tag;

    for (List <RelayServer::Channel::Internal *>::Element * E = Client.Channels.First; E; E = E->Next)
        (** E)->RemoveClient (Client);
    
    if (Client.Handshook && internal->Handlers.Disconnect)
        internal->Handlers.Disconnect (internal->Server, Client.Public);

    internal->ClientBacklog.Return (Client);
}

static void HandlerReceive (Server &Server, Server::Client &ClientSocket, char * Data, int Size)
{
    RelayServer::Client::Internal &Client = *(RelayServer::Client::Internal *) ClientSocket.Tag;
    
    if (!Client.GotFirstByte)
    {
        Client.GotFirstByte = true;

        ++ Data;

        if (!-- Size)
            return;
    }

    Client.Reader.Process (Data, Size);
}

static void HandlerError (Server &Server, Error &Error)
{
    RelayServer::Internal * internal = (RelayServer::Internal *) Server.Tag;

    Error.Add("Socket error");

    if (internal->Handlers.Error)
        internal->Handlers.Error (internal->Server, Error);
}

static void HandlerUDPReceive (UDP &UDP, Address &Address, char * Data, int Size)
{
    RelayServer::Internal * internal = (RelayServer::Internal *) UDP.Tag;

    if(Size < (sizeof(unsigned short) + 1))
        return;

    unsigned char Type = *(unsigned char  *) Data;
    unsigned short ID  = *(unsigned short *) (Data + 1);

    Data += sizeof(unsigned short) + 1;
    Size -= sizeof(unsigned short) + 1;

    Server &Socket = internal->Socket;

    for (Server::Client * ClientSocket = Socket.FirstClient (); ClientSocket; ClientSocket = ClientSocket->Next ())
    {
        RelayServer::Client::Internal &Client = *(RelayServer::Client::Internal *) ClientSocket->Tag;
        
        if(Client.ID == ID)
        {
            if (Client.Socket.GetAddress () != Address)
                return;

            Client.UDPAddress.Port(Address.Port ());
            Client.MessageHandler(Type, Data, Size, true);

            break;
        }
    }
}

static void HandlerUDPError (UDP &UDP, Error &Error)
{
    RelayServer::Internal * internal = (RelayServer::Internal *) UDP.Tag;

    Error.Add("UDP socket error");

    if(internal->Handlers.Error)
        internal->Handlers.Error(internal->Server, Error);
}

RelayServer::Internal::Internal (RelayServer &_Server, Pump &Pump)
        : Server (_Server), Builder (false), Socket (Pump), UDP (Pump), Timer (Pump)
{
    memset (&Handlers, 0, sizeof (Handlers));

    WelcomeMessage = strdup (Lacewing::Version ());

    Timer.Tag = Socket.Tag = UDP.Tag = this;

    Timer.onTick (TimerTick);

    Socket.DisableNagling ();

    Socket.onConnect     (::HandlerConnect);
    Socket.onDisconnect  (::HandlerDisconnect);
    Socket.onReceive     (::HandlerReceive);
    Socket.onError       (::HandlerError);

    UDP.onReceive  (::HandlerUDPReceive);
    UDP.onError    (::HandlerUDPError);

    ChannelListingEnabled = true;
}

RelayServer::RelayServer (Pump &Pump)
{
    LacewingInitialise ();

    internal = new RelayServer::Internal (*this, Pump);
    Tag = 0;
}

RelayServer::~RelayServer ()
{
    Unhost();

    delete internal;
}

void RelayServer::Host (int Port)
{
    Filter Filter;
    Filter.LocalPort(Port);

    Host(Filter);
}

void RelayServer::Host (Filter &_Filter)
{
    Filter Filter (_Filter);

    if (!Filter.LocalPort())
        Filter.LocalPort(6121);

    internal->Socket.Host (Filter, true);
    internal->UDP.Host (Filter);

    internal->Timer.Start (5000);
}

void RelayServer::Unhost ()
{
    internal->Socket.Unhost ();
    internal->UDP.Unhost ();

    internal->Timer.Stop ();
}

bool RelayServer::Hosting ()
{
    return internal->Socket.Hosting ();
}

int RelayServer::Port ()
{
    return internal->Socket.Port ();
}

void RelayServer::Channel::Internal::Close ()
{
    FrameBuilder &Builder = Server.Builder;

    /* Tell all the clients that they've left, and remove this channel from their channel lists. */

    Builder.AddHeader   (0, 0, false); /* Response */
    Builder.Add <unsigned char>   (3); /* LeaveChannel */
    Builder.Add <unsigned char>   (1); /* Success */
    Builder.Add <unsigned short> (ID);

    for (List <RelayServer::Client::Internal *>::Element * E = Clients.First;
            E; E = E->Next)
    {
        RelayServer::Client::Internal &Client = *** E;
        Builder.Send (Client.Socket, false);

        for (List <RelayServer::Channel::Internal *>::Element * E2 = Client.Channels.First;
                E2; E2 = E2->Next)
        {
            if(** E2 == this)
            {
                Client.Channels.Erase (E2);
                break;
            }
        }
    }

    Builder.Reset();

    
    /* Remove this channel from the channel list and return it to the backlog. */

    for (List <RelayServer::Channel::Internal *>::Element * E = Server.Channels.First;
            E; E = E->Next)
    {
        if(** E == this)
        {
            Server.Channels.Erase(E);
            break;
        }
    }
    
    Server.ChannelBacklog.Return(*this);
}

void RelayServer::Channel::Internal::RemoveClient (RelayServer::Client::Internal &Client)
{
    for (List <RelayServer::Client::Internal *>::Element * E = Clients.First;
            E; E = E->Next)
    {
        if(** E == &Client)
        {
            Clients.Erase (E);
            break;
        }
    }

    if((!Clients.Size) || (ChannelMaster == &Client && AutoClose))
    {   
        Close ();
        return;
    }

    if(ChannelMaster == &Client)
        ChannelMaster = 0;


    FrameBuilder &Builder = Server.Builder;

    /* Notify all the other peers that this client has left the channel */

    Builder.AddHeader (9, 0); /* Peer */
    
    Builder.Add <unsigned short> (ID);
    Builder.Add <unsigned short> (Client.ID);

    for(List <RelayServer::Client::Internal *>::Element * E = Clients.First;
            E; E = E->Next)
    {
        Builder.Send((** E)->Socket, false);
    }

    Builder.Reset();
}

bool RelayServer::Client::Internal::CheckName (const char * Name)
{
    for (List <RelayServer::Channel::Internal *>::Element * E = Channels.First;
            E; E = E->Next)
    {
        RelayServer::Channel::Internal * Channel = ** E;

        for (List <RelayServer::Client::Internal *>::Element * E2 = Channel->Clients.First;
                E2; E2 = E2->Next)
        {
            if ((** E2) == this)
                continue;

            if(!strcasecmp((** E2)->Name, Name))
            {
                FrameBuilder &Builder = Server.Builder;

                Builder.AddHeader        (0, 0);  /* Response */
                Builder.Add <unsigned char> (1);  /* SetName */
                Builder.Add <unsigned char> (0);  /* Failed */

                Builder.Add <unsigned char> (strlen(Name));
                Builder.Add (Name, -1);

                Builder.Add ("Name already taken", -1);

                Builder.Send(Socket);

                return false;
            }
        }
    }

    return true;
}

void RelayServer::Client::Internal::MessageHandler (unsigned char Type, char * Message, int Size, bool Blasted)
{
    unsigned char MessageTypeID  = (Type >> 4);
    unsigned char Variant        = (Type << 4);

    Variant >>= 4;

    MessageReader Reader (Message, Size);
    FrameBuilder &Builder = Server.Builder;

    if(MessageTypeID != 0 && !Handshook)
    {
        Socket.Disconnect();
        return;
    }

    switch(MessageTypeID)
    {
        case 0: /* Request */
        {
            unsigned char RequestType = Reader.Get <unsigned char> ();

            if(Reader.Failed)
                break;

            if(RequestType != 0 && !Handshook)
            {
                Reader.Failed = true;
                break;
            }

            switch(RequestType)
            {
                case 0: /* Connect */
                {
                    const char * Version = Reader.GetRemaining ();

                    if(Reader.Failed)
                        break;

                    if(Handshook)
                    {
                        Reader.Failed = true;
                        break;
                    }

                    if(strcmp(Version, "revision 3"))
                    {
                        Builder.AddHeader        (0, 0);  /* Response */
                        Builder.Add <unsigned char> (0);  /* Connect */
                        Builder.Add <unsigned char> (0);  /* Failed */
                        Builder.Add ("Version mismatch", -1);

                        Builder.Send(Socket);

                        Reader.Failed = true;
                        break;
                    }

                    if (Server.Handlers.Connect && !Server.Handlers.Connect (Server.Server, Public))
                    {
                        Builder.AddHeader        (0, 0);  /* Response */
                        Builder.Add <unsigned char> (0);  /* Connect */
                        Builder.Add <unsigned char> (0);  /* Failed */
                        Builder.Add ("Connection refused by server", -1);

                        Builder.Send(Socket);

                        Reader.Failed = true;
                        break;
                    }

                    Handshook = true;

                    Builder.AddHeader          (0, 0);  /* Response */
                    Builder.Add <unsigned char>   (0);  /* Connect */
                    Builder.Add <unsigned char>   (1);  /* Success */
                    
                    Builder.Add <unsigned short> (ID);
                    Builder.Add (Server.WelcomeMessage);

                    Builder.Send(Socket);

                    break;
                }

                case 1: /* SetName */
                {
                    const char * Name = Reader.GetRemaining (false);

                    if (Reader.Failed)
                        break;

                    if (!CheckName (Name))
                        break;

                    char * OldName = strdup (this->Name);

                    /* The .Name() setter will also set NameAltered to true.  This means that if the
                       handler sets the name explicitly, the default behaviour of setting the name to
                       the requested one will be skipped. */

                    NameAltered = false;

                    if (Server.Handlers.SetName && !Server.Handlers.SetName (Server.Server, Public, Name))
                    {
                        Builder.AddHeader        (0, 0);  /* Response */
                        Builder.Add <unsigned char> (1);  /* SetName */
                        Builder.Add <unsigned char> (0);  /* Failed */
                        
                        Builder.Add <unsigned char> (strlen(Name));
                        Builder.Add (Name, -1);

                        Builder.Add ("Name refused by server", -1);

                        Builder.Send(Socket);

                        free (OldName);
                        break;
                    }

                    if (!NameAltered)
                    {
                        free (this->Name);
                        this->Name = strdup (Name);
                    }
                    else
                    {
                        /* Check the new name provided by the handler */

                        if (!CheckName (this->Name))
                        {
                            free (this->Name);
                            this->Name = OldName;

                            break;
                        }
                    }

                    free (OldName);

                    Builder.AddHeader        (0, 0);  /* Response */
                    Builder.Add <unsigned char> (1);  /* SetName */
                    Builder.Add <unsigned char> (1);  /* Success */
                
                    Builder.Add <unsigned char> (strlen (this->Name));
                    Builder.Add (this->Name);

                    Builder.Send (Socket);

                    for (List <RelayServer::Channel::Internal *>::Element * E = Channels.First;
                            E; E = E->Next)
                    {
                        RelayServer::Channel::Internal * Channel = ** E;

                        Builder.AddHeader (9, 0); /* Peer */
                        
                        Builder.Add <unsigned short> (Channel->ID);
                        Builder.Add <unsigned short> (ID);
                        Builder.Add <unsigned char>  (this == Channel->ChannelMaster ? 1 : 0);
                        Builder.Add (this->Name);

                        for (List <RelayServer::Client::Internal *>::Element * E2 = Channel->Clients.First;
                                E2; E2 = E2->Next)
                        {
                            if(** E2 == this)
                                continue;

                            Builder.Send((** E2)->Socket, false);
                        }

                        Builder.Reset ();
                    }

                    break;
                }

                case 2: /* JoinChannel */
                {            
                    if(!*this->Name)
                        Reader.Failed = true;

                    unsigned char Flags = Reader.Get <unsigned char> ();
                    char *        Name  = Reader.GetRemaining(false);
                    
                    if(Reader.Failed)
                        break;

                    RelayServer::Channel::Internal * Channel = 0;

                    for (List <RelayServer::Channel::Internal *>::Element * E = Server.Channels.First;
                            E; E = E->Next)
                    {
                        if(!strcasecmp ((** E)->Name, Name))
                        {
                            Channel = ** E;
                            break;
                        }
                    }
                    
                    if(Channel)
                    {
                        /* Joining an existing channel */

                        bool NameTaken = false;

                        for (List <RelayServer::Client::Internal *>::Element * E = Channel->Clients.First;
                                E; E = E->Next)
                        {
                            RelayServer::Client::Internal * Client = ** E;
    
                            if(!strcasecmp (Client->Name, this->Name))
                            {
                                NameTaken = true;
                                break;
                            }
                        }

                        if(NameTaken)
                        {
                            Builder.AddHeader        (0, 0);  /* Response */
                            Builder.Add <unsigned char> (2);  /* JoinChannel */
                            Builder.Add <unsigned char> (0);  /* Failed */

                            Builder.Add <unsigned char> (strlen(Name));
                            Builder.Add (Name, -1);

                            Builder.Add ("Name already taken", -1);

                            Builder.Send(Socket);

                            break;
                        }

                        if (Server.Handlers.JoinChannel && !Server.Handlers.JoinChannel (Server.Server, Public, Channel->Public))
                        {
                            Builder.AddHeader        (0, 0);  /* Response */
                            Builder.Add <unsigned char> (2);  /* JoinChannel */
                            Builder.Add <unsigned char> (0);  /* Failed */

                            Builder.Add <unsigned char> (strlen (Channel->Name));
                            Builder.Add (Channel->Name);

                            Builder.Add ("Join refused by server", -1);

                            Builder.Send(Socket);
                            
                            break;
                        }
                    
                        Builder.AddHeader        (0, 0);  /* Response */
                        Builder.Add <unsigned char> (2);  /* JoinChannel */
                        Builder.Add <unsigned char> (1);  /* Success */
                        Builder.Add <unsigned char> (0);  /* Not the channel master */

                        Builder.Add <unsigned char> (strlen (Channel->Name));
                        Builder.Add (Channel->Name);

                        Builder.Add <unsigned short> (Channel->ID);
                        
                        for (List <RelayServer::Client::Internal *>::Element * E = Channel->Clients.First;
                                E; E = E->Next)
                        {
                            RelayServer::Client::Internal * Client = ** E;

                            Builder.Add <unsigned short> (Client->ID);
                            Builder.Add <unsigned char>  (Channel->ChannelMaster == Client ? 1 : 0);
                            Builder.Add <unsigned char>  (strlen (Client->Name));
                            Builder.Add (Client->Name);
                        }

                        Builder.Send(Socket);


                        Builder.AddHeader (9, 0); /* Peer */
                        
                        Builder.Add <unsigned short> (Channel->ID);
                        Builder.Add <unsigned short> (ID);
                        Builder.Add <unsigned char>  (0);
                        Builder.Add (this->Name);

                        /* Notify the other clients on the channel that this client has joined */

                        for (List <RelayServer::Client::Internal *>::Element * E = Channel->Clients.First;
                                E; E = E->Next)
                        {
                            Builder.Send((** E)->Socket, false);
                        }

                        Builder.Reset();


                        /* Add this client to the channel */

                        Channels.Push (Channel);
                        Channel->Clients.Push (this);

                        break;
                    }

                    /* Creating a new channel */

                    Channel = &Server.ChannelBacklog.Borrow(Server);

                    Channel->Name          =  Name;
                    Channel->ChannelMaster =  this;
                    Channel->Hidden        =  (Flags & 1) != 0;
                    Channel->AutoClose     =  (Flags & 2) != 0;

                    if (Server.Handlers.JoinChannel && !Server.Handlers.JoinChannel (Server.Server, Public, Channel->Public))
                    {
                        Builder.AddHeader        (0, 0);  /* Response */
                        Builder.Add <unsigned char> (2);  /* JoinChannel */
                        Builder.Add <unsigned char> (0);  /* Failed */

                        Builder.Add <unsigned char> (strlen (Channel->Name));
                        Builder.Add (Channel->Name);

                        Builder.Add ("Join refused by server", -1);

                        Builder.Send(Socket);
                        
                        Server.ChannelBacklog.Return(*Channel);
                        break;
                    }

                    Channel->Element = Server.Channels.Push (Channel);

                    Channel->Clients.Push (this);
                    Channels.Push (Channel);

                    Builder.AddHeader        (0, 0);  /* Response */
                    Builder.Add <unsigned char> (2);  /* JoinChannel */
                    Builder.Add <unsigned char> (1);  /* Success */
                    Builder.Add <unsigned char> (1);  /* Channel master */

                    Builder.Add <unsigned char> (strlen (Channel->Name));
                    Builder.Add (Channel->Name);

                    Builder.Add <unsigned short> (Channel->ID);

                    Builder.Send(Socket);

                    break;
                }

                case 3: /* LeaveChannel */
                {
                    RelayServer::Channel::Internal * Channel = ReadChannel (Reader);

                    if(Reader.Failed)
                        break;

                    if (Server.Handlers.LeaveChannel && !Server.Handlers.LeaveChannel (Server.Server, Public, Channel->Public))
                    {
                        Builder.AddHeader         (0, 0);  /* Response */
                        Builder.Add <unsigned char>  (3);  /* LeaveChannel */
                        Builder.Add <unsigned char>  (0);  /* Failed */
                        Builder.Add <unsigned short> (Channel->ID);

                        Builder.Add ("Leave refused by server", -1);

                        Builder.Send(Socket);

                        break;
                    }

                    for (List <RelayServer::Channel::Internal *>::Element * E = Channels.First; E; E = E->Next)
                    {
                        if(** E == Channel)
                        {
                            Channels.Erase(E);
                            break;
                        }
                    } 

                    Builder.AddHeader         (0, 0);  /* Response */
                    Builder.Add <unsigned char>  (3);  /* LeaveChannel */
                    Builder.Add <unsigned char>  (1);  /* Success */
                    Builder.Add <unsigned short> (Channel->ID);

                    Builder.Send(Socket);

                    /* Do this last, because it might delete the channel */

                    Channel->RemoveClient(*this);

                    break;
                }

                case 4: /* ChannelList */

                    if (!Server.ChannelListingEnabled)
                    {
                        Builder.AddHeader        (0, 0);  /* Response */
                        Builder.Add <unsigned char> (4);  /* ChannelList */
                        Builder.Add <unsigned char> (0);  /* Failed */
                        
                        Builder.Add ("Channel listing is not enabled on this server");

                        Builder.Send (Socket);

                        break;
                    }

                    Builder.AddHeader        (0, 0);  /* Response */
                    Builder.Add <unsigned char> (4);  /* ChannelList */
                    Builder.Add <unsigned char> (1);  /* Success */

                    for (List <RelayServer::Channel::Internal *>::Element * E = Server.Channels.First;
                            E; E = E->Next)
                    {
                        RelayServer::Channel::Internal * Channel = ** E;

                        if(Channel->Hidden)
                            continue;

                        Builder.Add <unsigned short> (Channel->Clients.Size);
                        Builder.Add <unsigned char>  (strlen (Channel->Name));
                        Builder.Add (Channel->Name);
                    }

                    Builder.Send(Socket);

                    break;

                default:
                    
                    Reader.Failed = true;
                    break;
            }

            break;
        }

        case 1: /* BinaryServerMessage */
        {
            unsigned char Subchannel = Reader.Get <unsigned char> ();
            
            char * Message;
            unsigned int Size;

            Reader.GetRemaining(Message, Size);
            
            if(Reader.Failed)
                break;

            if (Server.Handlers.ServerMessage)
                Server.Handlers.ServerMessage (Server.Server, Public, Blasted, Subchannel, Message, Size, Variant);

            break;
        }

        case 2: /* BinaryChannelMessage */
        {
            unsigned char Subchannel = Reader.Get <unsigned char> ();
            RelayServer::Channel::Internal * Channel = ReadChannel (Reader);
            
            char * Message;
            unsigned int Size;

            Reader.GetRemaining(Message, Size);
            
            if(Reader.Failed)
                break;

            if (Server.Handlers.ChannelMessage && !Server.Handlers.ChannelMessage
                    (Server.Server, Public, Channel->Public, Blasted, Subchannel, Message, Size, Variant))
            {
                break;
            }

            Builder.AddHeader (2, Variant, Blasted); /* BinaryChannelMessage */
            
            Builder.Add <unsigned char>  (Subchannel);
            Builder.Add <unsigned short> (Channel->ID);
            Builder.Add <unsigned short> (ID);
            Builder.Add (Message, Size);

            for (List <RelayServer::Client::Internal *>::Element * E = Channel->Clients.First; E; E = E->Next)
            {
                if(** E == this)
                    continue;

                if (Blasted)
                    Builder.Send (Server.UDP, (** E)->UDPAddress, false);
                else
                    Builder.Send((** E)->Socket, false);
            }

            Builder.Reset ();

            break;
        }

        case 3: /* BinaryPeerMessage */
        {
            unsigned char Subchannel = Reader.Get <unsigned char> ();
            RelayServer::Channel::Internal * Channel = ReadChannel      (Reader);
            RelayServer::Client::Internal  * Peer = Channel->ReadPeer (Reader);

            if(Peer == this)
            {
                Reader.Failed = true;
                break;
            }

            char * Message;
            unsigned int Size;

            Reader.GetRemaining(Message, Size);
            
            if(Reader.Failed)
                break;

            if (Server.Handlers.PeerMessage && !Server.Handlers.PeerMessage
                (Server.Server, Public, Channel->Public, Peer->Public, Blasted, Subchannel, Message, Size, Variant))
            {
                break;
            }

            Builder.AddHeader (3, Variant, Blasted); /* BinaryPeerMessage */
            
            Builder.Add <unsigned char>  (Subchannel);
            Builder.Add <unsigned short> (Channel->ID);
            Builder.Add <unsigned short> (ID);
            Builder.Add (Message, Size);

            if (Blasted)
                Builder.Send (Server.UDP, Peer->UDPAddress);
            else
                Builder.Send (Peer->Socket);

            break;
        }
            
        case 4: /* ObjectServerMessage */

            break;
            
        case 5: /* ObjectChannelMessage */

            break;
            
        case 6: /* ObjectPeerMessage */

            break;
            
        case 7: /* UDPHello */

            if(!Blasted)
            {
                Reader.Failed = true;
                break;
            }

            Builder.AddHeader (10, 0); /* UDPWelcome */
            Builder.Send      (Server.UDP, UDPAddress);

            break;
            
        case 8: /* ChannelMaster */

            break;

        case 9: /* Ping */

            Ponged = true;
            break;

        default:

            Reader.Failed = true;
            break;
    };

    if(Reader.Failed)
    {
        /* Socket.Disconnect(); */
    }
}

void RelayServer::Client::Send (int Subchannel, const char * Message, int Size, int Variant)
{
    FrameBuilder &Builder = internal->Server.Builder;

    Builder.AddHeader (1, Variant); /* BinaryServerMessage */
    
    Builder.Add <unsigned char> (Subchannel);
    Builder.Add (Message, Size);

    Builder.Send (internal->Socket);
}

void RelayServer::Client::Blast (int Subchannel, const char * Message, int Size, int Variant)
{
    FrameBuilder &Builder = internal->Server.Builder;

    Builder.AddHeader (1, Variant, true); /* BinaryServerMessage */
    
    Builder.Add <unsigned char> (Subchannel);
    Builder.Add (Message, Size);

    Builder.Send (internal->Server.UDP, internal->UDPAddress);
}

void RelayServer::Channel::Send (int Subchannel, const char * Message, int Size, int Variant)
{
    FrameBuilder &Builder = internal->Server.Builder;

    Builder.AddHeader (4, Variant); /* BinaryServerChannelMessage */
    
    Builder.Add <unsigned char> (Subchannel);
    Builder.Add <unsigned short> (internal->ID);
    Builder.Add (Message, Size);

    for (List <RelayServer::Client::Internal *>::Element *
                E = internal->Clients.First; E; E = E->Next)
    {
        Builder.Send ((** E)->Socket, false);
    }

    Builder.Reset ();
}

void RelayServer::Channel::Blast (int Subchannel, const char * Message, int Size, int Variant)
{
    FrameBuilder &Builder = internal->Server.Builder;

    Builder.AddHeader (4, Variant, true); /* BinaryServerChannelMessage */
    
    Builder.Add <unsigned char> (Subchannel);
    Builder.Add <unsigned short> (internal->ID);
    Builder.Add (Message, Size);

    for (List <RelayServer::Client::Internal *>::Element *
                E = internal->Clients.First; E; E = E->Next)
    {
        Builder.Send (internal->Server.UDP, (** E)->UDPAddress, false);
    }

    Builder.Reset ();
}

int RelayServer::Client::ID ()
{
    return ((RelayServer::Client::Internal *) internal)->ID;
}

const char * RelayServer::Channel::Name ()
{
    return ((RelayServer::Channel::Internal *) internal)->Name;
}

void RelayServer::Channel::Name (const char * Name)
{
    free (internal->Name);
    internal->Name = strdup (Name);
}

bool RelayServer::Channel::Hidden ()
{
    return ((RelayServer::Channel::Internal *) internal)->Hidden;
}

bool RelayServer::Channel::AutoCloseEnabled ()
{
    return ((RelayServer::Channel::Internal *) internal)->AutoClose;
}

void RelayServer::SetWelcomeMessage (const char * Message)
{
    free (internal->WelcomeMessage);
    internal->WelcomeMessage = strdup (Message);
}

void RelayServer::SetChannelListing (bool Enabled)
{
    internal->ChannelListingEnabled = Enabled;
}

RelayServer::Client * RelayServer::Channel::ChannelMaster ()
{
    RelayServer::Client::Internal * Client = ((RelayServer::Channel::Internal *) internal)->ChannelMaster;

    return Client ? &Client->Public : 0;
}

void RelayServer::Channel::Close ()
{
    ((RelayServer::Channel::Internal *) internal)->Close ();
}

void RelayServer::Client::Disconnect ()
{
    ((RelayServer::Client::Internal *) internal)->Socket.Disconnect ();
}

Address &RelayServer::Client::GetAddress ()
{
    return ((RelayServer::Client::Internal *) internal)->Socket.GetAddress ();
}

const char * RelayServer::Client::Name ()
{
    return ((RelayServer::Client::Internal *) internal)->Name;
}

void RelayServer::Client::Name (const char * Name)
{
    free (internal->Name);
    internal->Name = strdup (Name);

    internal->NameAltered = true;
}

int RelayServer::ChannelCount ()
{
    return internal->Channels.Size;
}

int RelayServer::Channel::ClientCount ()
{
    return ((RelayServer::Channel::Internal *) internal)->Clients.Size;
}

int RelayServer::Client::ChannelCount ()
{
    return ((RelayServer::Client::Internal *) internal)->Channels.Size;
}

int RelayServer::ClientCount ()
{
    return internal->Socket.ClientCount ();
}

RelayServer::Client * RelayServer::FirstClient ()
{
    return internal->Socket.FirstClient () ?
        (RelayServer::Client *) internal->Socket.FirstClient ()->Tag : 0;
}

RelayServer::Client * RelayServer::Client::Next ()
{
    Server::Client * NextSocket = internal->Socket.Next ();

    return NextSocket ? (RelayServer::Client *) NextSocket->Tag : 0;
}

RelayServer::Channel * RelayServer::Channel::Next ()
{
    return internal->Element->Next ? &(** internal->Element->Next)->Public : 0;
}

RelayServer::Channel * RelayServer::FirstChannel ()
{
    return internal->Channels.First ?
            &(** internal->Channels.First)->Public : 0;
}

RelayServer::Channel::ClientIterator::ClientIterator
    (RelayServer::Channel &Channel)
{
    internal = Channel.internal->Clients.First;
}

RelayServer::Client * RelayServer::Channel::ClientIterator::Next ()
{
    List <RelayServer::Client *>::Element * E =
        (List <RelayServer::Client *>::Element *) internal;

    if (!E)
        return 0;

    internal = E->Next;
    return ** E;
}

RelayServer::Client::ChannelIterator::ChannelIterator
    (RelayServer::Client &Client)
{
    internal = ((RelayServer::Client::Internal *) Client.internal)->Channels.First;
}

RelayServer::Channel * RelayServer::Client::ChannelIterator::Next ()
{
    List <RelayServer::Channel *>::Element * E =
        (List <RelayServer::Channel *>::Element *) internal;

    if (!E)
        return 0;

    internal = E->Next;
    return ** E;
}

AutoHandlerFunctions (RelayServer, Connect)
AutoHandlerFunctions (RelayServer, Disconnect)
AutoHandlerFunctions (RelayServer, Error)
AutoHandlerFunctions (RelayServer, ServerMessage)
AutoHandlerFunctions (RelayServer, ChannelMessage)
AutoHandlerFunctions (RelayServer, PeerMessage)
AutoHandlerFunctions (RelayServer, JoinChannel)
AutoHandlerFunctions (RelayServer, LeaveChannel)
AutoHandlerFunctions (RelayServer, SetName)

