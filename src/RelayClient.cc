
/*
    Copyright (C) 2011 James McLaughlin

    This file is part of Lacewing.

    Lacewing is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lacewing is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Lacewing.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "Common.h"

#include "FrameReader.h"
#include "FrameBuilder.h"

struct ChannelInternal;

struct RelayClientInternal
{
    ThreadTracker           Threads;

    Lacewing::RelayClient   &Client;
    Lacewing::Client        &Socket;
    Lacewing::UDP           &UDP;

    Lacewing::Timer         Timer;

    Lacewing::RelayClient::HandlerConnect              HandlerConnect;
    Lacewing::RelayClient::HandlerConnectionDenied     HandlerConnectionDenied;
    Lacewing::RelayClient::HandlerDisconnect           HandlerDisconnect;
    Lacewing::RelayClient::HandlerServerMessage        HandlerServerMessage;
    Lacewing::RelayClient::HandlerChannelMessage       HandlerChannelMessage;
    Lacewing::RelayClient::HandlerPeerMessage          HandlerPeerMessage;
    Lacewing::RelayClient::HandlerError                HandlerError;
    Lacewing::RelayClient::HandlerJoin                 HandlerJoin;
    Lacewing::RelayClient::HandlerJoinDenied           HandlerJoinDenied;
    Lacewing::RelayClient::HandlerLeave                HandlerLeave;
    Lacewing::RelayClient::HandlerLeaveDenied          HandlerLeaveDenied;
    Lacewing::RelayClient::HandlerNameSet              HandlerNameSet;
    Lacewing::RelayClient::HandlerNameChanged          HandlerNameChanged;
    Lacewing::RelayClient::HandlerNameDenied           HandlerNameDenied;
    Lacewing::RelayClient::HandlerPeerConnect          HandlerPeerConnect;
    Lacewing::RelayClient::HandlerPeerDisconnect       HandlerPeerDisconnect;
    Lacewing::RelayClient::HandlerPeerChangeName       HandlerPeerChangeName;
    Lacewing::RelayClient::HandlerChannelListReceived  HandlerChannelListReceived;

    RelayClientInternal(Lacewing::RelayClient &_Client, Lacewing::Client &_Socket,
                Lacewing::UDP &_UDP, Lacewing::Pump &_EventPump) : Client(_Client),
                    Socket(_Socket), UDP(_UDP), Message (true), Timer(_EventPump)
    {
        HandlerConnect              = 0;
        HandlerConnectionDenied     = 0;
        HandlerDisconnect           = 0; 
        HandlerServerMessage        = 0;
        HandlerChannelMessage       = 0;
        HandlerPeerMessage          = 0;
        HandlerError                = 0;
        HandlerJoin                 = 0;
        HandlerJoinDenied           = 0;
        HandlerLeave                = 0;
        HandlerLeaveDenied          = 0;
        HandlerNameSet              = 0;
        HandlerNameChanged          = 0;
        HandlerNameDenied           = 0;
        HandlerPeerConnect          = 0;
        HandlerPeerDisconnect       = 0;
        HandlerPeerChangeName       = 0;
        HandlerChannelListReceived  = 0;

        Reader.MessageHandler = MessageHandler;
        Reader.Tag = this;

        Timer.onTick(ClientTick);
        Timer.Tag = this;

        Clear();
    }

    FrameReader Reader;

    static void MessageHandler (void * Tag, unsigned char Type, char * Message, int Size);
    void        MessageHandler (unsigned char Type, char * Message, int Size, bool Blasted);

    static void ClientTick (Lacewing::Timer &Timer);
    void        ClientTick ();

    ChannelInternal * ReadChannel(MessageReader &Reader);

    FrameBuilder Message;

    list <Lacewing::RelayClient::ChannelListing *> ChannelList;

    void ClearChannelList()
    {
        for(list<Lacewing::RelayClient::ChannelListing *>::iterator it = ChannelList.begin(); it != ChannelList.end(); ++ it)
            delete *it;
        
        ChannelList.clear();
    }

    void Clear();

    list <ChannelInternal *> Channels;
    string Name;

    int ID;
    string WelcomeMessage;

    bool UDPAcknowledged;
};

struct PeerInternal
{
    Lacewing::RelayClient::Channel::Peer Public;

    ChannelInternal &Channel;

    PeerInternal(ChannelInternal &_Channel) : Channel(_Channel)
    {
        Public.InternalTag = this;
        Public.Tag = 0;
    }

    int     ID;
    string  Name;
   
    bool IsChannelMaster;
};

struct ChannelInternal
{
    Lacewing::RelayClient::Channel Public;

    RelayClientInternal &Client;

    ChannelInternal(RelayClientInternal &_Client) : Client(_Client)
    {
        Public.InternalTag = this;
        Public.Tag = 0;
    }

    ~ChannelInternal()
    {

    }

    int     ID;
    string  Name;
    bool    IsChannelMaster;

    list<PeerInternal *> Peers;

    PeerInternal * ReadPeer(MessageReader &Reader)
    {
        int PeerID = Reader.Get<unsigned short>();

        if(Reader.Failed)
            return 0;

        for(list<PeerInternal *>::iterator it = Peers.begin(); it != Peers.end(); ++ it)
            if((*it)->ID == PeerID)
                return *it;
         
        Reader.Failed = true;
        return 0;
    }

    PeerInternal * AddNewPeer(int PeerID, int Flags, const char * Name)
    {
        PeerInternal * Peer = new PeerInternal(*this);

        Peer->ID               = PeerID;
        Peer->IsChannelMaster  = (Flags & 1) != 0;
        Peer->Name            += Name;

        Peers.push_back(Peer);

        return Peer;
    }
};

void RelayClientInternal::Clear()
{
    ClearChannelList();

    for(list<ChannelInternal *>::iterator it = Channels.begin(); it != Channels.end(); ++ it)
        delete *it;
    
    Channels.clear();

    Name = "";
    ID   = -1;

    UDPAcknowledged = false;
}

ChannelInternal * RelayClientInternal::ReadChannel(MessageReader &Reader)
{
    int ChannelID = Reader.Get <unsigned short> ();

    if(Reader.Failed)
        return 0;

    for(list<ChannelInternal *>::iterator it = Channels.begin(); it != Channels.end(); ++ it)
        if((*it)->ID == ChannelID)
            return *it;
     
    Reader.Failed = true;
    return 0;
}

void HandlerConnect(Lacewing::Client &Socket)
{
    RelayClientInternal &Internal = *(RelayClientInternal *) Socket.Tag;
    
    Internal.UDP.Host(Socket.ServerAddress());

    FrameBuilder &Message = Internal.Message;

    Message.AddHeader(0, 0); /* Request */

    Message.Add <unsigned char> (0); /* Connect */
    Message.Add ("revision 2", -1);

    Message.Send(Internal.Socket);
}

void HandlerDisconnect(Lacewing::Client &Socket)
{
    RelayClientInternal &Internal = *(RelayClientInternal *) Socket.Tag;

    Internal.Timer.Stop();

    if(Internal.HandlerDisconnect)
        Internal.HandlerDisconnect(Internal.Client);

    Internal.Clear();
}

void HandlerReceive(Lacewing::Client &Socket, char * Data, int Size)
{
    RelayClientInternal &Internal = *(RelayClientInternal *) Socket.Tag;

    Internal.Reader.Process(Data, Size);
}

void HandlerError(Lacewing::Client &Socket, Lacewing::Error &Error)
{
    RelayClientInternal &Internal = *(RelayClientInternal *) Socket.Tag;

    Error.Add("Socket error");
    
    if(Internal.HandlerError)
        Internal.HandlerError(Internal.Client, Error);
}

void HandlerClientUDPReceive(Lacewing::UDP &UDP, Lacewing::Address &Address, char * Data, int Size)
{
    RelayClientInternal &Internal = *(RelayClientInternal *) UDP.Tag;

    if(!Size)
        return;

    Internal.MessageHandler(*Data, Data + 1, Size - 1, true);
}

void HandlerClientUDPError(Lacewing::UDP &UDP, Lacewing::Error &Error)
{
    RelayClientInternal &Internal = *(RelayClientInternal *) UDP.Tag;

    Error.Add("Socket error");
    
    if(Internal.HandlerError)
        Internal.HandlerError(Internal.Client, Error);
}

Lacewing::RelayClient::RelayClient(Lacewing::Pump &EventPump) : Socket(EventPump), UDP(EventPump)
{
    LacewingInitialise();

    Socket.onConnect     (::HandlerConnect);
    Socket.onDisconnect  (::HandlerDisconnect);
    Socket.onReceive     (::HandlerReceive);
    Socket.onError       (::HandlerError);

    UDP.onReceive (::HandlerClientUDPReceive);
    UDP.onError   (::HandlerClientUDPError);
 
    Socket.DisableNagling ();

    InternalTag = Socket.Tag = UDP.Tag = new RelayClientInternal(*this, Socket, UDP, EventPump);
    Tag         = 0;
}

Lacewing::RelayClient::~RelayClient()
{
    Disconnect();

    delete ((RelayClientInternal *) InternalTag);
}

void Lacewing::RelayClient::Connect(const char * Host, int Port)
{
    Socket.Connect(Host, Port);
}

void Lacewing::RelayClient::Connect(Lacewing::Address &Address)
{
    if(!Address.Port())
        Address.Port(6121);

    Socket.Connect(Address);
}

void Lacewing::RelayClient::Disconnect()
{
    Socket.Disconnect();
}

bool Lacewing::RelayClient::Connected()
{
    return Socket.Connected();
}

bool Lacewing::RelayClient::Connecting()
{
    return Socket.Connecting();
}

const char * Lacewing::RelayClient::Name()
{
    return ((RelayClientInternal *) InternalTag)->Name.c_str();
}

void Lacewing::RelayClient::ListChannels()
{
    RelayClientInternal &Internal = *((RelayClientInternal *) InternalTag);
    FrameBuilder   &Message  = Internal.Message;

    Message.AddHeader        (0, 0);  /* Request */
    Message.Add <unsigned char> (4);  /* ChannelList */
    
    Message.Send(Internal.Socket);
}

void Lacewing::RelayClient::Name(const char * Name)
{
    RelayClientInternal &Internal = *((RelayClientInternal *) InternalTag);
    FrameBuilder   &Message  = Internal.Message;

    if(!*Name)
    {
        Lacewing::Error Error;
        Error.Add("Can't set a blank name");
        
        if(Internal.HandlerError)
            Internal.HandlerError(*this, Error);

        return;
    }

    Message.AddHeader        (0, 0);  /* Request */
    Message.Add <unsigned char> (1);  /* SetName */
    Message.Add (Name, -1);

    Message.Send(Internal.Socket);
}

void Lacewing::RelayClient::Join(const char * Channel, bool Hidden, bool AutoClose)
{
    RelayClientInternal &Internal = *((RelayClientInternal *) InternalTag);
    FrameBuilder   &Message  = Internal.Message;

    if(!*Name())
    {
        Lacewing::Error Error;
        Error.Add("Set a name before joining a channel");
        
        if(Internal.HandlerError)
            Internal.HandlerError(*this, Error);

        return;
    }

    if(!*Channel)
    {
        Lacewing::Error Error;
        Error.Add("Can't join a channel with a blank name");
        
        if(Internal.HandlerError)
            Internal.HandlerError(*this, Error);

        return;
    }

    Message.AddHeader        (0, 0);  /* Request */
    Message.Add <unsigned char> (2);  /* JoinChannel */
    Message.Add <unsigned char> ((Hidden ? 1 : 0) | (AutoClose ? 2 : 0));
    Message.Add (Channel, -1);

    Message.Send(Internal.Socket);
}

void Lacewing::RelayClient::SendServer(int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    RelayClientInternal &Internal = *((RelayClientInternal *) InternalTag);
    FrameBuilder   &Message  = Internal.Message;

    Message.AddHeader(1, Variant); /* BinaryServerMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add (Data, Size);

    Message.Send(Internal.Socket);
}

void Lacewing::RelayClient::BlastServer(int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    RelayClientInternal &Internal = *((RelayClientInternal *) InternalTag);
    FrameBuilder   &Message  = Internal.Message;

    Message.AddHeader (1, Variant, 1, Internal.ID); /* BinaryServerMessage */
    Message.Add <unsigned char> (Subchannel);
    Message.Add (Data, Size);

    Message.Send(Internal.UDP, Internal.Socket.ServerAddress());
}

void Lacewing::RelayClient::Channel::Send(int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    ChannelInternal &Channel   = *((ChannelInternal *) InternalTag);
    RelayClientInternal  &Internal  = Channel.Client;
    FrameBuilder    &Message   = Internal.Message;

    Message.AddHeader (2, Variant); /* BinaryChannelMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add <unsigned short> (Channel.ID);
    Message.Add (Data, Size);

    Message.Send(Internal.Socket);
}

void Lacewing::RelayClient::Channel::Blast(int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    ChannelInternal &Channel   = *((ChannelInternal *) InternalTag);
    RelayClientInternal  &Internal  = Channel.Client;
    FrameBuilder    &Message   = Internal.Message;

    Message.AddHeader(2, Variant, 1, Internal.ID); /* BinaryChannelMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add <unsigned short> (Channel.ID);
    Message.Add (Data, Size);

    Message.Send(Internal.UDP, Internal.Socket.ServerAddress());
}

void Lacewing::RelayClient::Channel::Peer::Send(int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    PeerInternal    &Peer      = *((PeerInternal *) InternalTag);
    ChannelInternal &Channel   = Peer.Channel;
    RelayClientInternal  &Internal  = Channel.Client;
    FrameBuilder    &Message   = Internal.Message;

    Message.AddHeader(3, Variant); /* BinaryPeerMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add <unsigned short> (Channel.ID);
    Message.Add <unsigned short> (Peer.ID);
    Message.Add (Data, Size);

    Message.Send(Internal.Socket);
}

void Lacewing::RelayClient::Channel::Peer::Blast(int Subchannel, const char * Data, int Size, int Variant)
{
    if(Size == -1)
        Size = strlen(Data);

    PeerInternal    &Peer      = *((PeerInternal *) InternalTag);
    ChannelInternal &Channel   = Peer.Channel;
    RelayClientInternal  &Internal  = Channel.Client;
    FrameBuilder    &Message   = Internal.Message;

    Message.AddHeader(3, Variant, 1, Internal.ID); /* BinaryPeerMessage */
    Message.Add <unsigned char>  (Subchannel);
    Message.Add <unsigned short> (Channel.ID);
    Message.Add <unsigned short> (Peer.ID);
    Message.Add (Data, Size);

    Message.Send(Internal.UDP, Internal.Socket.ServerAddress());
}

void Lacewing::RelayClient::Channel::Leave()
{
    ChannelInternal &Channel  = *((ChannelInternal *) InternalTag);
    RelayClientInternal  &Internal = Channel.Client;
    FrameBuilder    &Message  = Internal.Message;

    Internal.Message.AddHeader(0, 0); /* Request */

    Message.Add <unsigned char>  (3);  /* LeaveChannel */
    Message.Add <unsigned short> (Channel.ID);

    Message.Send(Internal.Socket);
}

const char * Lacewing::RelayClient::Channel::Name()
{
    return ((ChannelInternal *) InternalTag)->Name.c_str();
}

int Lacewing::RelayClient::Channel::PeerCount()
{
    return ((ChannelInternal *) InternalTag)->Peers.size();
}

const char * Lacewing::RelayClient::Channel::Peer::Name()
{
    return ((PeerInternal *) InternalTag)->Name.c_str();
}

int Lacewing::RelayClient::ChannelCount()
{
     return ((RelayClientInternal *) InternalTag)->Channels.size();
}

int Lacewing::RelayClient::ID()
{
    return ((RelayClientInternal *) InternalTag)->ID;
}

Lacewing::Address &Lacewing::RelayClient::ServerAddress()
{
    return Socket.ServerAddress();
}

void RelayClientInternal::MessageHandler(unsigned char Type, char * Message, int Size, bool Blasted)
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

            switch(ResponseType)
            {
                case 0: /* Connect */
                {
                    if(Succeeded)
                    {
                        ID = Reader.Get <unsigned short> ();

                        WelcomeMessage  = "";
                        WelcomeMessage += Reader.GetRemaining();

                        if(Reader.Failed)
                            break;

                        ClientTick();
                        Timer.Start(500);

                        /* The connect handler will be called on UDP acknowledged */
                    }
                    else
                    {
                        if(HandlerConnectionDenied)
                            HandlerConnectionDenied(Client, Reader.GetRemaining());
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
                        if(!this->Name.length())
                        {
                            this->Name += Name;

                            if(HandlerNameSet)
                                HandlerNameSet(Client);
                        }
                        else
                        {
                            string OldName = "";
                            OldName += this->Name;

                            this->Name  = "";
                            this->Name += Name;

                            if(HandlerNameChanged)
                                HandlerNameChanged(Client, OldName.c_str());
                        }
                    }
                    else
                    {
                        char * DenyReason = Reader.GetRemaining();

                        if(HandlerNameDenied)
                            HandlerNameDenied(Client, Name, DenyReason);
                    }

                    break;
                }

                case 2: /* JoinChannel */
                {
                    unsigned char Flags       = Reader.Get <unsigned char> ();
                    unsigned char NameLength  = Reader.Get <unsigned char> ();
                    char *        Name        = Reader.Get (NameLength);

                    if(Reader.Failed)
                        break;

                    if(Succeeded)
                    {
                        int ChannelID = Reader.Get <unsigned short> ();

                        if(Reader.Failed)
                            break;

                        ChannelInternal * Channel = new ChannelInternal(*this);

                        Channel->ID              = ChannelID;
                        Channel->Name           += Name;
                        Channel->IsChannelMaster = (Flags & 1) != 0;

                        for(;;)
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

                        Channels.push_back(Channel);

                        if(HandlerJoin)
                            HandlerJoin(Client, Channel->Public);
                    }
                    else
                    {
                        char * DenyReason = Reader.GetRemaining();

                        if(HandlerJoinDenied)
                            HandlerJoinDenied(Client, Name, DenyReason);
                    }

                    break;
                }
                
                case 3: /* LeaveChannel */
                {
                    ChannelInternal * Channel = ReadChannel(Reader);

                    if(Reader.Failed)
                        break;

                    if(Succeeded)
                    {
                        for(list<ChannelInternal *>::iterator it = Channels.begin(); it != Channels.end(); ++ it)
                        {
                            if(*it == Channel)
                            {
                                Channels.erase(it);
                                break;
                            }
                        }

                        if(HandlerLeave)
                            HandlerLeave(Client, Channel->Public);

                        delete Channel;
                    }
                    else
                    {
                        char * DenyReason = Reader.GetRemaining();

                        if(HandlerLeaveDenied)
                            HandlerLeaveDenied(Client, Channel->Public, DenyReason);
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

                        Lacewing::RelayClient::ChannelListing * Listing = new Lacewing::RelayClient::ChannelListing;

                        Listing->Name       = strdup(Name);
                        Listing->PeerCount  = PeerCount;

                        ChannelList.push_back(Listing);
                    }

                    if(HandlerChannelListReceived)
                        HandlerChannelListReceived(Client);

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

            if(HandlerServerMessage)
                HandlerServerMessage(Client, Blasted, Subchannel, Message, Size, Variant);

            break;
        }

        case 2: /* BinaryChannelMessage */
        {
            int Subchannel            = Reader.Get <unsigned char> ();
            ChannelInternal * Channel = ReadChannel          (Reader);
            PeerInternal    * Peer    = Channel->ReadPeer    (Reader);

            char * Message;
            unsigned int Size;

            Reader.GetRemaining(Message, Size);

            if(Reader.Failed)
                break;

            if(HandlerChannelMessage)
                HandlerChannelMessage(Client, Channel->Public, Peer->Public, Blasted,
                                        Subchannel, Message, Size, Variant);

            break;
        }
        
        case 3: /* BinaryPeerMessage */
        {
            int Subchannel            = Reader.Get <unsigned char> ();
            ChannelInternal * Channel = ReadChannel          (Reader);
            PeerInternal    * Peer    = Channel->ReadPeer    (Reader);

            char * Message;
            unsigned int Size;

            Reader.GetRemaining(Message, Size);

            if(Reader.Failed)
                break;

            if(HandlerPeerMessage)
                HandlerPeerMessage(Client, Channel->Public, Peer->Public, Blasted,
                                         Subchannel, Message, Size, Variant);

            break;
        }

        case 4: /* ObjectServerMessage */
        case 5: /* ObjectChannelMessage */
        case 6: /* ObjectPeerMessage */
            
            break;

        case 7: /* Peer */
        {
            ChannelInternal * Channel = ReadChannel (Reader);
            
            if(Reader.Failed)
                break;

            int PeerID = Reader.Get <unsigned short> ();
            PeerInternal * Peer = 0;

            for(list<PeerInternal *>::iterator it = Channel->Peers.begin(); it != Channel->Peers.end(); ++ it)
            {
                if((*it)->ID == PeerID)
                {
                    Peer = *it;
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

                for(list<PeerInternal *>::iterator it = Channel->Peers.begin(); it != Channel->Peers.end(); ++ it)
                {
                    if(*it == Peer)
                    {
                        Channel->Peers.erase(it);
                        break;
                    }
                }

                if(HandlerPeerDisconnect)
                    HandlerPeerDisconnect(Client, Channel->Public, Peer->Public);
                
                delete Peer;
                break;
            }

            if(!Peer)
            {
                /* New peer */

                Peer = Channel->AddNewPeer (PeerID, Flags, Name);

                if(HandlerPeerConnect)
                    HandlerPeerConnect(Client, Channel->Public, Peer->Public);

                break;
            }

            /* Existing peer */

            if(strcmp(Name, Peer->Name.c_str()))
            {
                /* Peer is changing their name */

                string OldName = "";
                OldName += Peer->Name;

                Peer->Name =  "";
                Peer->Name += Name;

                if(HandlerPeerChangeName)
                    HandlerPeerChangeName(Client, Channel->Public, Peer->Public, OldName.c_str());
            }

            Peer->IsChannelMaster = (Flags & 1) != 0;
            
            break;
        }

        case 8: /* UDPWelcome */
            
            Timer.Stop();
            UDPAcknowledged = true;

            if(HandlerConnect)
                HandlerConnect(Client, WelcomeMessage.c_str());

            break;

        case 9: /* Ping */

            this->Message.AddHeader(9, 0); /* Pong */
            this->Message.Send(Socket);

            break;

    };
}


void RelayClientInternal::MessageHandler(void * Tag, unsigned char Type, char * Message, int Size)
{
    ((RelayClientInternal *) Tag)->MessageHandler(Type, Message, Size, false);
}

void RelayClientInternal::ClientTick(Lacewing::Timer &Timer)
{
    ((RelayClientInternal *) Timer.Tag)->ClientTick();
}

void RelayClientInternal::ClientTick()
{
    Message.AddHeader (7, 0, 1, ID); /* UDPHello */
    Message.Send      (UDP, Socket.ServerAddress());
}

bool Lacewing::RelayClient::Channel::Peer::IsChannelMaster()
{
    PeerInternal &Internal = *(PeerInternal *) InternalTag;

    return Internal.IsChannelMaster;
}

int Lacewing::RelayClient::Channel::Peer::ID()
{
    PeerInternal &Internal = *(PeerInternal *) InternalTag;

    return Internal.ID;
}

bool Lacewing::RelayClient::Channel::IsChannelMaster()
{
    ChannelInternal &Internal = *(ChannelInternal *) InternalTag;

    return Internal.IsChannelMaster;
}

Looper(A, RelayClient, Channel, RelayClientInternal, Channels, ;, ;, list<ChannelInternal *>::iterator, Lacewing::RelayClient::Channel &, ->Public);
Looper(A, RelayClient::Channel, Peer, ChannelInternal, Peers, ;, ;, list<PeerInternal *>::iterator, Lacewing::RelayClient::Channel::Peer &, ->Public);
Looper(A, RelayClient, ChannelList, RelayClientInternal, ChannelList, ;, ;, list<Lacewing::RelayClient::ChannelListing *>::iterator, Lacewing::RelayClient::ChannelListing *, ;);

AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, Connect)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, ConnectionDenied)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, Disconnect) 
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, ServerMessage)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, ChannelMessage)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, PeerMessage)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, Error)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, Join)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, JoinDenied)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, Leave)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, LeaveDenied)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, NameSet)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, NameChanged)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, NameDenied)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, PeerConnect)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, PeerDisconnect)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, PeerChangeName)
AutoHandlerFunctions(Lacewing::RelayClient, RelayClientInternal, ChannelListReceived)

