
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011 James McLaughlin.  All rights reserved.
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

struct RelayClient::Internal
{
    RelayClient       &Client;

    Lacewing::Client  Socket;
    Lacewing::UDP     UDP;
    Lacewing::Timer   Timer;

    struct
    {
        HandlerConnect               Connect;
        HandlerConnectionDenied      ConnectionDenied;
        HandlerDisconnect            Disconnect;
        HandlerServerMessage         ServerMessage;
        HandlerChannelMessage        ChannelMessage;
        HandlerPeerMessage           PeerMessage;
        HandlerServerChannelMessage  ServerChannelMessage;
        HandlerError                 Error;
        HandlerJoin                  Join;
        HandlerJoinDenied            JoinDenied;
        HandlerLeave                 Leave;
        HandlerLeaveDenied           LeaveDenied;
        HandlerNameSet               NameSet;
        HandlerNameChanged           NameChanged;
        HandlerNameDenied            NameDenied;
        HandlerPeerConnect           PeerConnect;
        HandlerPeerDisconnect        PeerDisconnect;
        HandlerPeerChangeName        PeerChangeName;
        HandlerChannelListReceived   ChannelListReceived;

    } Handlers;

    Internal (RelayClient &_Client, Pump &_EventPump);

    FrameReader Reader;

    static void MessageHandler (void * Tag, unsigned char Type, char * Message, int Size);
    void        MessageHandler (unsigned char Type, char * Message, int Size, bool Blasted);

    static void ClientTick (Lacewing::Timer &);
    void        ClientTick ();

    RelayClient::Channel::Internal * ReadChannel (MessageReader &Reader);

    FrameBuilder Message;

    List <RelayClient::ChannelListing *> ChannelList;

    void ClearChannelList()
    {
        for (List <RelayClient::ChannelListing *>::Element * E = ChannelList.First; E; E = E->Next)
            delete ** E;
        
        ChannelList.Clear();
    }

    void Clear();

    List <RelayClient::Channel::Internal *> Channels;
    String Name;

    int ID;
    String WelcomeMessage;

    bool Connected;
};

struct RelayClient::ChannelListing::Internal
{
};

struct RelayClient::Channel::Peer::Internal
{
    RelayClient::Channel::Peer Public;
    List <RelayClient::Channel::Peer::Internal *>::Element * Element;

    RelayClient::Channel::Internal &Channel;

    Internal (RelayClient::Channel::Internal &_Channel) : Channel (_Channel)
    {
        Public.internal = this;
        Public.Tag = 0;
    }

    int ID;
    String Name;
   
    bool IsChannelMaster;
};

struct RelayClient::Channel::Internal
{
    RelayClient::Channel Public;
    List <RelayClient::Channel::Internal *>::Element * Element;

    RelayClient::Internal &Client;

    Internal (RelayClient::Internal &_Client) : Client (_Client)
    {
        Public.internal = this;
        Public.Tag = 0;
    }

    ~ Internal ()
    {
    }

    int ID;
    String Name;
    bool IsChannelMaster;

    List <RelayClient::Channel::Peer::Internal *> Peers;

    RelayClient::Channel::Peer::Internal * ReadPeer (MessageReader &Reader)
    {
        int PeerID = Reader.Get<unsigned short>();

        if(Reader.Failed)
            return 0;

        for (List <RelayClient::Channel::Peer::Internal *>::Element * E = Peers.First; E; E = E->Next)
        {
            RelayClient::Channel::Peer::Internal * Peer = ** E;

            if(Peer->ID == PeerID)
                return Peer;
        }

        Reader.Failed = true;
        return 0;
    }

    RelayClient::Channel::Peer::Internal * AddNewPeer (int PeerID, int Flags, const char * Name)
    {
        RelayClient::Channel::Peer::Internal * Peer = new RelayClient::Channel::Peer::Internal (*this);

        Peer->ID               = PeerID;
        Peer->IsChannelMaster  = (Flags & 1) != 0;
        Peer->Name             = Name;

        Peer->Element = Peers.Push (Peer);

        return Peer;
    }
};

void RelayClient::Internal::Clear()
{
    ClearChannelList();

    for (List <RelayClient::Channel::Internal *>::Element * E = Channels.First; E; E = E->Next)
        delete ** E;
    
    Channels.Clear();

    Name = "";
    ID   = -1;

    Connected = false;
}

RelayClient::Channel::Internal * RelayClient::Internal::ReadChannel (MessageReader &Reader)
{
    int ChannelID = Reader.Get <unsigned short> ();

    if(Reader.Failed)
        return 0;

    for(List <RelayClient::Channel::Internal *>::Element * E = Channels.First; E; E = E->Next)
        if((** E)->ID == ChannelID)
            return ** E;
     
    Reader.Failed = true;
    return 0;
}

static void HandlerConnect (Client &Socket)
{
    RelayClient::Internal * internal = (RelayClient::Internal *) Socket.Tag;

    /* Opening 0 byte */
    Socket.Send ("", 1);

    internal->UDP.Host (Socket.ServerAddress ());

    FrameBuilder &Message = internal->Message;

    Message.AddHeader(0, 0); /* Request */

    Message.Add <unsigned char> (0); /* Connect */
    Message.Add ("revision 3", -1);

    Message.Send(internal->Socket);
}

static void HandlerDisconnect (Client &Socket)
{
    RelayClient::Internal * internal = (RelayClient::Internal *) Socket.Tag;

    internal->Timer.Stop();

    internal->Connected = false;

    if (internal->Handlers.Disconnect)
        internal->Handlers.Disconnect (internal->Client);

    internal->Clear ();
}

static void HandlerReceive (Client &Socket, char * Data, int Size)
{
    RelayClient::Internal * internal = (RelayClient::Internal *) Socket.Tag;

    internal->Reader.Process (Data, Size);
}

static void HandlerError (Client &Socket, Error &Error)
{
    RelayClient::Internal * internal = (RelayClient::Internal *) Socket.Tag;

    Error.Add("Socket error");
    
    if (internal->Handlers.Error)
        internal->Handlers.Error (internal->Client, Error);
}

void HandlerClientUDPReceive (UDP &UDP, Address &Address, char * Data, int Size)
{
    RelayClient::Internal * internal = (RelayClient::Internal *) UDP.Tag;

    if(!Size)
        return;

    internal->MessageHandler (*Data, Data + 1, Size - 1, true);
}

void HandlerClientUDPError (UDP &UDP, Error &Error)
{
    RelayClient::Internal * internal = (RelayClient::Internal *) UDP.Tag;

    Error.Add("Socket error");
    
    if (internal->Handlers.Error)
        internal->Handlers.Error (internal->Client, Error);
}

RelayClient::Internal::Internal (RelayClient &_Client, Pump &_EventPump)
    : Client (_Client), Message (true), Socket (_EventPump), Timer (_EventPump), UDP (_EventPump)
{
    memset (&Handlers, 0, sizeof (Handlers));

    Reader.MessageHandler = MessageHandler;
    Reader.Tag = this;

    Timer.onTick (ClientTick);

    Socket.Tag = UDP.Tag = Timer.Tag = this;

    Socket.DisableNagling ();

    Socket.onConnect     (::HandlerConnect);
    Socket.onDisconnect  (::HandlerDisconnect);
    Socket.onReceive     (::HandlerReceive);
    Socket.onError       (::HandlerError);

    UDP.onReceive (::HandlerClientUDPReceive);
    UDP.onError   (::HandlerClientUDPError);

    Clear ();
}

RelayClient::RelayClient (Pump &EventPump)
{
    LacewingInitialise ();

    internal = new RelayClient::Internal (*this, EventPump);

    Tag = 0;
}

RelayClient::~RelayClient ()
{
    Disconnect ();

    delete internal;
}

void RelayClient::Connect (const char * Host, int Port)
{
    if (!Port)
        Port = 6121;

    internal->Socket.Connect (Host, Port);
}

void RelayClient::Connect (Address &Address)
{
    internal->Socket.Connect (Address);
}

void RelayClient::Disconnect ()
{
    internal->Socket.Disconnect ();
}

bool RelayClient::Connected ()
{
    return internal->Connected;
}

bool RelayClient::Connecting ()
{
    return (!Connected ()) &&
        (internal->Socket.Connected () || internal->Socket.Connecting ());
}

const char * RelayClient::Name ()
{
    return internal->Name;
}

void RelayClient::ListChannels ()
{
    FrameBuilder   &Message  = internal->Message;

    Message.AddHeader        (0, 0);  /* Request */
    Message.Add <unsigned char> (4);  /* ChannelList */
    
    Message.Send (internal->Socket);
}

void RelayClient::Name (const char * Name)
{
    FrameBuilder   &Message  = internal->Message;

    if(!*Name)
    {
        Lacewing::Error Error;
        Error.Add("Can't set a blank name");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error(*this, Error);

        return;
    }

    Message.AddHeader        (0, 0);  /* Request */
    Message.Add <unsigned char> (1);  /* SetName */
    Message.Add (Name, -1);

    Message.Send (internal->Socket);
}

void RelayClient::Join (const char * Channel, bool Hidden, bool AutoClose)
{
    FrameBuilder &Message = internal->Message;

    if(!*Name ())
    {
        Lacewing::Error Error;
        Error.Add("Set a name before joining a channel");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    if(!*Channel)
    {
        Lacewing::Error Error;
        Error.Add("Can't join a channel with a blank name");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    Message.AddHeader        (0, 0);  /* Request */
    Message.Add <unsigned char> (2);  /* JoinChannel */
    Message.Add <unsigned char> ((Hidden ? 1 : 0) | (AutoClose ? 2 : 0));
    Message.Add (Channel, -1);

    Message.Send (internal->Socket);
}

void RelayClient::SendServer (int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    FrameBuilder &Message = internal->Message;

    Message.AddHeader(1, Variant); /* BinaryServerMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add (Data, Size);

    Message.Send (internal->Socket);
}

void RelayClient::BlastServer (int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    FrameBuilder &Message = internal->Message;

    Message.AddHeader (1, Variant, 1, internal->ID); /* BinaryServerMessage */
    Message.Add <unsigned char> (Subchannel);
    Message.Add (Data, Size);

    Message.Send (internal->UDP, internal->Socket.ServerAddress ());
}

void RelayClient::Channel::Send (int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    RelayClient::Internal &Client = internal->Client;
    FrameBuilder &Message = Client.Message;

    Message.AddHeader (2, Variant); /* BinaryChannelMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add <unsigned short> (internal->ID);
    Message.Add (Data, Size);

    Message.Send (Client.Socket);
}

void RelayClient::Channel::Blast (int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    RelayClient::Internal &Client = internal->Client;
    FrameBuilder &Message = Client.Message;

    Message.AddHeader (2, Variant, 1, Client.ID); /* BinaryChannelMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add <unsigned short> (internal->ID);
    Message.Add (Data, Size);

    Message.Send (Client.UDP, Client.Socket.ServerAddress ());
}

void RelayClient::Channel::Peer::Send (int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    RelayClient::Channel::Internal &Channel = internal->Channel;
    RelayClient::Internal &Client = Channel.Client;
    FrameBuilder &Message = Client.Message;

    Message.AddHeader (3, Variant); /* BinaryPeerMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add <unsigned short> (Channel.ID);
    Message.Add <unsigned short> (internal->ID);
    Message.Add (Data, Size);

    Message.Send (Client.Socket);
}

void RelayClient::Channel::Peer::Blast (int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    RelayClient::Channel::Peer::Internal &Peer = *((RelayClient::Channel::Peer::Internal *) internal);
    RelayClient::Channel::Internal &Channel = Peer.Channel;
    RelayClient::Internal &Client = Channel.Client;
    FrameBuilder &Message = Client.Message;

    Message.AddHeader (3, Variant, 1, Client.ID); /* BinaryPeerMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add <unsigned short> (Channel.ID);
    Message.Add <unsigned short> (internal->ID);
    Message.Add (Data, Size);

    Message.Send (Client.UDP, Client.Socket.ServerAddress ());
}

void RelayClient::Channel::Leave ()
{
    RelayClient::Channel::Internal &Channel  = *((RelayClient::Channel::Internal *) internal);
    RelayClient::Internal &Client = Channel.Client;
    FrameBuilder &Message = Client.Message;

    Message.AddHeader (0, 0); /* Request */

    Message.Add <unsigned char>  (3);  /* LeaveChannel */
    Message.Add <unsigned short> (Channel.ID);

    Message.Send (Client.Socket);
}

const char * RelayClient::Channel::Name ()
{
    return ((RelayClient::Channel::Internal *) internal)->Name;
}

int RelayClient::Channel::PeerCount ()
{
    return ((RelayClient::Channel::Internal *) internal)->Peers.Size;
}

const char * RelayClient::Channel::Peer::Name ()
{
    return ((RelayClient::Channel::Peer::Internal *) internal)->Name;
}

int RelayClient::ChannelCount ()
{
     return internal->Channels.Size;
}

int RelayClient::ID ()
{
    return internal->ID;
}

Address &RelayClient::ServerAddress ()
{
    return internal->Socket.ServerAddress ();
}

const char * RelayClient::WelcomeMessage ()
{
    return internal->WelcomeMessage;
}

void RelayClient::Internal::MessageHandler (unsigned char Type, char * Message, int Size, bool Blasted)
{
    unsigned char MessageTypeID  = (Type >> 4);
    unsigned char Variant        = (Type << 4);

    Variant >>= 4;

    MessageReader Reader (Message, Size);

    switch(MessageTypeID)
    {
        case 0: /* Response */
        {
            unsigned char  ResponseType = Reader.Get <unsigned char> ();
            bool           Succeeded    = Reader.Get <unsigned char> () != 0;

            if(Reader.Failed)
                break;

            switch (ResponseType)
            {
                case 0: /* Connect */
                {
                    if(Succeeded)
                    {
                        ID = Reader.Get <unsigned short> ();
                        WelcomeMessage = Reader.GetRemaining();

                        if (Reader.Failed)
                            break;

                        ClientTick();
                        Timer.Start(500);

                        /* The connect handler will be called on UDP acknowledged */
                    }
                    else
                    {
                        if (Handlers.ConnectionDenied)
                            Handlers.ConnectionDenied(Client, Reader.GetRemaining ());
                    }

                    break;
                }

                case 1: /* SetName */
                {
                    unsigned char NameLength = Reader.Get <unsigned char> ();
                    char *        Name       = Reader.Get (NameLength);

                    if(Reader.Failed)
                        break;

                    if(Succeeded)
                    {
                        if(!this->Name.Length)
                        {
                            this->Name = Name;

                            if (Handlers.NameSet)
                                Handlers.NameSet (Client);
                        }
                        else
                        {
                            String OldName = this->Name;
                            this->Name = Name;

                            if (Handlers.NameChanged)
                                Handlers.NameChanged (Client, OldName);
                        }
                    }
                    else
                    {
                        char * DenyReason = Reader.GetRemaining();

                        if (Handlers.NameDenied)
                            Handlers.NameDenied (Client, Name, DenyReason);
                    }

                    break;
                }

                case 2: /* JoinChannel */
                {
                    unsigned char Flags       = Succeeded ? Reader.Get <unsigned char> () : 0;
                    unsigned char NameLength  = Reader.Get <unsigned char> ();
                    char *        Name        = Reader.Get (NameLength);

                    if(Reader.Failed)
                        break;

                    if(Succeeded)
                    {
                        int ChannelID = Reader.Get <unsigned short> ();

                        if(Reader.Failed)
                            break;

                        RelayClient::Channel::Internal * Channel = new RelayClient::Channel::Internal (*this);

                        Channel->ID              = ChannelID;
                        Channel->Name            = Name;
                        Channel->IsChannelMaster = (Flags & 1) != 0;

                        for (;;)
                        {
                            int PeerID     = Reader.Get <unsigned short> ();
                            int Flags      = Reader.Get <unsigned char>  ();
                            int NameLength = Reader.Get <unsigned char>  ();

                            if(Reader.Failed)
                                break;
                            
                            char * Name = Reader.Get(NameLength);

                            if(Reader.Failed)
                                break;

                            Channel->AddNewPeer(PeerID, Flags, Name);
                        }

                        Channel->Element = Channels.Push (Channel);

                        if (Handlers.Join)
                            Handlers.Join (Client, Channel->Public);
                    }
                    else
                    {
                        char * DenyReason = Reader.GetRemaining();

                        if (Handlers.JoinDenied)
                            Handlers.JoinDenied (Client, Name, DenyReason);
                    }

                    break;
                }
                
                case 3: /* LeaveChannel */
                {
                    RelayClient::Channel::Internal * Channel = ReadChannel (Reader);

                    if(Reader.Failed)
                        break;

                    if(Succeeded)
                    {
                        Channels.Erase (Channel->Element);

                        if (Handlers.Leave)
                            Handlers.Leave (Client, Channel->Public);

                        delete Channel;
                    }
                    else
                    {
                        char * DenyReason = Reader.GetRemaining();

                        if (Handlers.LeaveDenied)
                            Handlers.LeaveDenied (Client, Channel->Public, DenyReason);
                    }

                    break;
                }

                case 4: /* ChannelList */
                {
                    ClearChannelList();

                    for(;;)
                    {
                        int PeerCount  = Reader.Get <unsigned short> ();
                        int NameLength = Reader.Get <unsigned char>  ();

                        if(Reader.Failed)
                            break;
                        
                        char * Name = Reader.Get(NameLength);

                        if(Reader.Failed)
                            break;

                        RelayClient::ChannelListing * Listing = new RelayClient::ChannelListing;

                        Listing->Name         = strdup(Name);
                        Listing->PeerCount    = PeerCount;

                        Listing->internal =
                            (RelayClient::ChannelListing::Internal *) ChannelList.Push (Listing);
                    }

                    if (Handlers.ChannelListReceived)
                        Handlers.ChannelListReceived (Client);

                    break;
                }

                default:
                {
                    LacewingAssert(false);

                    break;
                }
            }

            break;
        }

        case 1: /* BinaryServerMessage */
        {
            int Subchannel = Reader.Get <unsigned char> ();

            char * Message;
            unsigned int Size;

            Reader.GetRemaining(Message, Size);

            if(Reader.Failed)
                break;

            if (Handlers.ServerMessage)
                Handlers.ServerMessage (Client, Blasted, Subchannel, Message, Size, Variant);

            break;
        }

        case 2: /* BinaryChannelMessage */
        {
            int Subchannel = Reader.Get <unsigned char> ();
            RelayClient::Channel::Internal * Channel = ReadChannel (Reader);
            RelayClient::Channel::Peer::Internal * Peer = Channel->ReadPeer (Reader);

            char * Message;
            unsigned int Size;

            Reader.GetRemaining(Message, Size);

            if(Reader.Failed)
                break;

            if (Handlers.ChannelMessage)
                Handlers.ChannelMessage (Client, Channel->Public, Peer->Public, Blasted,
                                        Subchannel, Message, Size, Variant);

            break;
        }
        
        case 3: /* BinaryPeerMessage */
        {
            int Subchannel = Reader.Get <unsigned char> ();
            RelayClient::Channel::Internal * Channel = ReadChannel (Reader);
            RelayClient::Channel::Peer::Internal * Peer = Channel->ReadPeer (Reader);

            char * Message;
            unsigned int Size;

            Reader.GetRemaining(Message, Size);

            if(Reader.Failed)
                break;

            if (Handlers.PeerMessage)
                Handlers.PeerMessage (Client, Channel->Public, Peer->Public, Blasted,
                                         Subchannel, Message, Size, Variant);

            break;
        }

        case 4: /* BinaryServerChannelMessage */
        {
            int Subchannel = Reader.Get <unsigned char> ();
            RelayClient::Channel::Internal * Channel = ReadChannel (Reader);

            char * Message;
            unsigned int Size;

            Reader.GetRemaining(Message, Size);

            if(Reader.Failed)
                break;

            if (Handlers.ServerChannelMessage)
                Handlers.ServerChannelMessage (Client, Channel->Public, Blasted,
                                Subchannel, Message, Size, Variant);

            break;
        }

        case 5: /* ObjectServerMessage */
        case 6: /* ObjectChannelMessage */
        case 7: /* ObjectPeerMessage */
        case 8: /* ObjectServerChannelMessage */
            
            break;

        case 9: /* Peer */
        {
            RelayClient::Channel::Internal * Channel = ReadChannel (Reader);
            
            if (Reader.Failed)
                break;

            int PeerID = Reader.Get <unsigned short> ();
            RelayClient::Channel::Peer::Internal * Peer = 0;

            for (List <RelayClient::Channel::Peer::Internal *>::Element * E = Channel->Peers.First; E; E = E->Next)
            {
                if((** E)->ID == PeerID)
                {
                    Peer = ** E;
                    break;
                }
            }

            unsigned char Flags = Reader.Get <unsigned char> ();
            const char *  Name  = Reader.GetRemaining();
            
            if(Reader.Failed)
            {
                /* No flags/name - the peer must have left the channel */
        
                if(!Peer)
                    break;
                
                Channel->Peers.Erase (Peer->Element);

                if (Handlers.PeerDisconnect)
                    Handlers.PeerDisconnect (Client, Channel->Public, Peer->Public);
                
                delete Peer;
                break;
            }

            if(!Peer)
            {
                /* New peer */

                Peer = Channel->AddNewPeer (PeerID, Flags, Name);

                if (Handlers.PeerConnect)
                    Handlers.PeerConnect (Client, Channel->Public, Peer->Public);

                break;
            }

            /* Existing peer */

            if (strcmp(Name, Peer->Name))
            {
                /* Peer is changing their name */

                String OldName (Peer->Name);
                Peer->Name = Name;

                if (Handlers.PeerChangeName)
                    Handlers.PeerChangeName (Client, Channel->Public, Peer->Public, OldName);
            }

            Peer->IsChannelMaster = (Flags & 1) != 0;
            
            break;
        }

        case 10: /* UDPWelcome */
            
            if (!Blasted)
                break;

            Timer.Stop();
            Connected = true;

            if (Handlers.Connect)
                Handlers.Connect (Client);

            break;

        case 11: /* Ping */

            this->Message.AddHeader(9, 0); /* Pong */
            this->Message.Send(Socket);

            break;

    };
}


void RelayClient::Internal::MessageHandler (void * Tag, unsigned char Type, char * Message, int Size)
{
    ((RelayClient::Internal *) Tag)->MessageHandler (Type, Message, Size, false);
}

void RelayClient::Internal::ClientTick (Lacewing::Timer &Timer)
{
    ((RelayClient::Internal *) Timer.Tag)->ClientTick ();
}

void RelayClient::Internal::ClientTick ()
{
    Message.AddHeader (7, 0, 1, ID); /* UDPHello */
    Message.Send      (UDP, Socket.ServerAddress());
}

bool RelayClient::Channel::Peer::IsChannelMaster ()
{
    return internal->IsChannelMaster;
}

int RelayClient::Channel::Peer::ID ()
{
    return internal->ID;
}

bool RelayClient::Channel::IsChannelMaster ()
{
    return internal->IsChannelMaster;
}

RelayClient::Channel * RelayClient::FirstChannel ()
{
    return internal->Channels.First ? 
        &(** internal->Channels.First)->Public : 0;
}

RelayClient::Channel * RelayClient::Channel::Next ()
{
    return ((RelayClient::Channel::Internal *) internal)->Element->Next ?
        &(** ((RelayClient::Channel::Internal *) internal)->Element->Next)->Public : 0;
}

RelayClient::Channel::Peer * RelayClient::Channel::FirstPeer ()
{
    return ((RelayClient::Channel::Internal *) internal)->Peers.First ?
            &(** ((RelayClient::Channel::Internal *) internal)->Peers.First)->Public : 0;
}

RelayClient::Channel::Peer * RelayClient::Channel::Peer::Next ()
{
    return ((RelayClient::Channel::Peer::Internal *) internal)->Element->Next ?
        &(** ((RelayClient::Channel::Peer::Internal *) internal)->Element->Next)->Public : 0;
}

int RelayClient::ChannelListingCount ()
{
    return internal->ChannelList.Size;
}

RelayClient::ChannelListing * RelayClient::FirstChannelListing ()
{
    return internal->ChannelList.First ?
        ** internal->ChannelList.First : 0;
}

RelayClient::ChannelListing * RelayClient::ChannelListing::Next ()
{
    return ((List <RelayClient::ChannelListing *>::Element *) internal)->Next ?
        ** ((List <RelayClient::ChannelListing *>::Element *) internal)->Next : 0; 
}

AutoHandlerFunctions (RelayClient, Connect)
AutoHandlerFunctions (RelayClient, ConnectionDenied)
AutoHandlerFunctions (RelayClient, Disconnect) 
AutoHandlerFunctions (RelayClient, ServerMessage)
AutoHandlerFunctions (RelayClient, ChannelMessage)
AutoHandlerFunctions (RelayClient, PeerMessage)
AutoHandlerFunctions (RelayClient, ServerChannelMessage)
AutoHandlerFunctions (RelayClient, Error)
AutoHandlerFunctions (RelayClient, Join)
AutoHandlerFunctions (RelayClient, JoinDenied)
AutoHandlerFunctions (RelayClient, Leave)
AutoHandlerFunctions (RelayClient, LeaveDenied)
AutoHandlerFunctions (RelayClient, NameSet)
AutoHandlerFunctions (RelayClient, NameChanged)
AutoHandlerFunctions (RelayClient, NameDenied)
AutoHandlerFunctions (RelayClient, PeerConnect)
AutoHandlerFunctions (RelayClient, PeerDisconnect)
AutoHandlerFunctions (RelayClient, PeerChangeName)
AutoHandlerFunctions (RelayClient, ChannelListReceived)

