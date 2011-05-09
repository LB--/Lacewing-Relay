# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

from twisted.internet.protocol import ClientFactory

from lacewing.moo.protocol import MooProtocol

from lacewing.moo.packet import ServerPacket, ClientPacket

from lacewing.moo.packetloaders.server import *
from lacewing.moo.packetloaders.client import *

from lacewing.multidict import MultikeyDict

from lacewing.moo.packetloaders.message import *

class Player(object):
    name = None
    id = None
    ip = None
    channel = None 

    def __init__(self, name, id, ip, channel):
        self.name = name
        self.id = id
        self.ip = ip
        self.channel = channel
    
    def isPeer(self):
        return self.channel.player != self
    
    def isMaster(self):
        return self.channel.getMaster() == self
    
    def sendMessage(self, *arg, **kw):
        self.channel.sendMessage(toClient = self, *arg, **kw)

class Channel(object):
    name = None
    id = None
    connection = None
    player = None
    connections = None
    
    masterId = None
    
    def __init__(self, name, id, connection):
        self.name = name
        self.id = id
        self.connection = connection
        
        self.connections = MultikeyDict()
    
    def addConnection(self, connection):
        self.connections[connection.name, connection.id] = connection
        
    def removeConnection(self, connection):
        del self.connections[connection]
    
    def sendMessage(self, value, subchannel, type = None, toClient = None):
        newValue = Message(**self.connection.settings)
        newValue.type = type or detectType(value)
        newValue.value = value
        if toClient:
            newMessage = PrivateMessage()
            newMessage.setConnection(toClient)
        else:
            newMessage = ToChannelMessage()

        newMessage.setChannel(self)
        newMessage.message = newValue
        newMessage.subchannel = subchannel
        self.connection.sendLoader(newMessage)
    
    def setMaster(self, id):
        self.masterId = id

    def getMaster(self):
        if not self.masterId in self.connections:
            return None
        return self.connections[self.masterId]

class MooClientProtocol(MooProtocol):
    _sendPacket = ClientPacket
    _receivePacket = ServerPacket
    
    motd = None
    
    playerClass = Player
    
    def __init__(self, **settings):
        self.settings = settings
        MooProtocol.__init__(self)

    def loaderReceived(self, loader):
        if isinstance(loader, MOTD):
            self.motd = loader.motd
            self.motdReceived(loader.motd)
        
        elif isinstance(loader, AssignedID):
            self.id = loader.playerId
            
            newName = SetName()
            newName.setConnection(self)
            self.sendLoader(newName)

            self.connectionAccepted()
        
        elif isinstance(loader, ChannelJoined):
            channelName = loader.channelName
            channelId = loader.channelId
            newChannel = Channel(channelName, channelId, self)
            newPlayer = self.playerClass(loader.playerName, loader.playerId, 
                loader.playerIp, newChannel)
            newChannel.player = newPlayer
            newChannel.addConnection(newPlayer)
            newChannel.setMaster(loader.masterId)
            self.channels[channelName, channelId] = newChannel
            self.channelJoined(newChannel)
        
        elif isinstance(loader, (PlayerExists, PlayerJoined)):
            playerName = loader.playerName
            playerId = loader.playerId
            playerIp = loader.playerIp
            channel, = self.channels[loader.channelId]
            newPlayer = self.playerClass(playerName, playerId, playerIp, channel)
            channel.addConnection(newPlayer)
            channel.setMaster(loader.masterId)
            if isinstance(loader, PlayerExists):
                self.channelUserExists(channel, newPlayer)
            elif isinstance(loader, PlayerJoined):
                self.channelUserJoined(channel, newPlayer)
        
        elif isinstance(loader, FromChannelMessage):
            if loader.isServer():
                self.messageReceived(loader.message, loader.subchannel)
            else:
                channel, = self.channels[loader.channelId]
                player, = channel.connections[loader.playerId]
                self.channelMessageReceived(channel, player, loader.message, 
                    loader.subchannel)
            
        elif isinstance(loader, PlayerChanged):
            playerId = loader.playerId
            playerName = loader.playerName
            for channel in self.channels.values():
                if playerId in channel.connections:
                    player, = channel.connections[playerId]
                    oldName = player.name
                    player.name = playerName
                    if player.isPeer():
                        self.channelUserChanged(player, oldName, player.name)
                    else:
                        self.nameChanged(channel, oldName, player.name)
        
        elif isinstance(loader, PlayerLeft):
            channel, = self.channels[loader.channelId]
            player, = channel.connections[loader.playerId]
            if player.isPeer():
                channel.removeConnection(player)
                channel.setMaster(loader.masterId)
                self.channelUserLeft(channel, player)
            else:
                del self.channels[channel]
                self.channelLeft(channel)
        
        else:
            raise NotImplementedError
        
    # event-like
    
    def motdReceived(self, value):
        """
        Called upon receiving the MOTD
        """
    
    def connectionAccepted(self):
        """
        Called when the connection has been accepted, and the
        client notified.
        """

    def messageReceived(self, message, subchannel):
        """
        Called upon receiving a server message
        
        @type message: L{Message} object
        """
    
    def channelMessageReceived(self, channel, connection, message, subchannel):
        """
        Called when a channel message has been received
        
        @type message: lacewing.moo.packetloaders.message.Message object
        """

    def channelJoined(self, channel):
        """
        Called when the server has accepted a channel join request.
        @arg channel: The channel the client has joined.
        """
        
    def channelLeft(self, channel):
        """
        Called when the server has accepted a channel leave request.
        @arg channel: The channel the client has left.
        """
    
    def nameChanged(self, channel, oldName, newName):
        """
        Called when the server has told us we've changed
        our name
        """

    def channelUserJoined(self, channel, client):
        """
        Called when a client has joined the channel.
        @arg channel: The channel the client has joined.
        """

    def channelUserExists(self, channel, client):
        """
        Called when a client exists in the channel.
        @arg channel: The channel the client exists in.
        """

    def channelUserLeft(self, channel, client):
        """
        Called when a client has left the channel.
        @arg channel: The channel the client has left.
        """

    def channelUserChanged(self, client, oldName, newName):
        """
        Called when a client in the channel (including has changed name.
        """
        
    # action-like
    
    def changeName(self, name):
        newChange = ChangeName()
        newChange.newName = name
        self.sendLoader(newChange)
    
    def sendMessage(self, value, subchannel, type = None):
        """
        Sends a message to the other end
        """
        newValue = Message(**self.settings)
        newValue.type = type or detectType(value)
        newValue.value = value
        newMessage = ClientMessage()
        newMessage.message = newValue
        newMessage.subchannel = subchannel
        self.sendLoader(newMessage)
    
    def sendChannelMessage(self, channel, *arg, **kw):
        channel.sendMessage(self, *arg, **kw)
    
    def joinChannel(self, channelName):
        newJoin = JoinChannel()
        newJoin.channelName = channelName
        self.sendLoader(newJoin)
    
    def leaveChannel(self, channel):
        newLeave = LeaveChannel()
        newLeave.setChannel(channel)
        self.sendLoader(newLeave)