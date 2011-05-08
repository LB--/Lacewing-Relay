# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

from twisted.internet.protocol import ServerFactory

from lacewing.moo.protocol import MooProtocol

from lacewing.moo.packet import ServerPacket, ClientPacket

from lacewing.moo.packetloaders.server import *
from lacewing.moo.packetloaders.client import *

from lacewing.multidict import MultikeyDict

from lacewing.moo.packetloaders.message import *

from lacewing.idpool import IDPool

class Channel(object):
    name = None
    id = None
    factory = None
    
    connections = None
    
    def __init__(self, name, id, factory):
        self.name = name
        self.id = id
        self.factory = factory
        
        self.connections = MultikeyDict()
    
    def addConnection(self, connection):
        self.connections[connection.name, connection.id] = connection
        connection.channels[self.name, self.id] = self

        newJoined = ChannelJoined()
        newJoined.setChannel(self)
        newJoined.setConnection(connection)
        connection.sendLoader(newJoined)
        
        newExists = PlayerExists()
        newExists.setChannel(self)
        
        newJoined = PlayerJoined()
        newJoined.setChannel(self)
        newJoined.setConnection(connection)
        
        for otherConnection in self.connections.values():
            if otherConnection != connection:
                newExists.setConnection(otherConnection)
                connection.sendLoader(newExists)
                otherConnection.sendLoader(newJoined)
        
    def removeConnection(self, connection):
        newLeft = PlayerLeft()
        newLeft.setChannel(self)
        newLeft.setConnection(connection)
        self.sendLoader(newLeft)
        
        del self.connections[connection]
        del connection.channels[self]
        
        return len(self.connections) != 0
    
    def sendMessage(self, connection, value, subchannel, type = None, toClient = None):
        newValue = Message(**connection.settings)
        newValue.type = type or detectType(value)
        newValue.value = value
        newMessage = FromChannelMessage()
        newMessage.setChannel(self)
        newMessage.setConnection(connection)
        newMessage.message = newValue
        newMessage.subchannel = subchannel
        if toClient:
            toClient.sendLoader(newMessage)
        else:
            self.sendLoader(newMessage, connection)
        
    def sendLoader(self, loader, fromClient = None):
        for connection in self.connections.values():
            if connection != fromClient:
                connection.sendLoader(loader)
        
    def getMaster(self):
        return self.connections.values()[0]

class MooServerProtocol(MooProtocol):
    _sendPacket = ServerPacket
    _receivePacket = ClientPacket

    def connectionMade(self):
        self.settings = self.factory.settings
        if not self.acceptConnection():
            return
        newMotd = MOTD()
        newMotd.motd = self.factory.motd
        self.sendLoader(newMotd)
        newAssigned = AssignedID()
        newId = self.factory.userPool.popId()
        self.id = newId
        newAssigned.playerId = newId
        self.sendLoader(newAssigned)
        self.isAccepted = True
        self.connectionAccepted()
        
    def connectionLost(self, reason):
        for channel in self.channels.values():
            self.leaveChannel(channel)
        self.factory.userPool.putBackId(self.id)

    def loaderReceived(self, loader):
        if isinstance(loader, SetName):
            self.name = loader.playerName
            self.nameSet(self.name)
        
        elif not self.isAccepted or not self.name:
            # if the client has come this far,
            # it means it is sending messages
            # without getting accepted
            # or setting name
            self.transport.loseConnection()

        elif isinstance(loader, ClientMessage):
            self.messageReceived(loader.message, loader.subchannel)
            
        elif isinstance(loader, JoinChannel):
            channelName = loader.channelName
            if channelName in self.channels:
                # client is in the channel already,
                # so ignore
                return
            if not self.acceptChannelJoin(channelName):
                return
            self.joinChannel(channelName)
        
        elif isinstance(loader, LeaveChannel):
            channelId = loader.channelId
            if not channelId in self.channels:
                # the client isn't already
                # in the channel, so we just
                # ignore
                return
            channel, = self.channels[channelId]
            if not self.acceptChannelLeave(channel):
                return
            self.leaveChannel(channel)
        
        elif isinstance(loader, ToChannelMessage):
            channelId = loader.channelId
            if not channelId in self.channels:
                # client has not
                # joined a channel with
                # this id-- ignore.
                return
            channel, = self.channels[channelId]
            message = loader.message
            subchannel = loader.subchannel
            if not self.acceptChannelMessage(channel, message, subchannel):
                return
            channel.sendMessage(self, message.value, subchannel, message.type)
            self.channelMessageReceived(channel, message, subchannel)

        elif isinstance(loader, PrivateMessage):
            channelId = loader.channelId
            if not channelId in self.channels:
                # client has not
                # joined a channel with
                # this id-- ignore.
                return
            channel, = self.channels[channelId]
            playerId = loader.playerId
            if not playerId in channel.connections:
                # player is not to be found in the
                # channel
                return
            player, = channel.connections[playerId]
            message = loader.message
            subchannel = loader.subchannel
            if not self.acceptPrivateMessage(channel, player, message, subchannel):
                return
            channel.sendMessage(self, message.value, subchannel, message.type, player)
            self.privateMessageReceived(channel, player, message, subchannel)
        
        elif isinstance(loader, ChangeName):
            if not self.acceptNameChange(self.name, loader.newName):
                return
            self.changeName(loader.newName)
        
        else:
            raise NotImplementedError
    
    # accept-like
    
    def acceptConnection(self):
        """
        Return False to stop the server from sending
        a welcome to the client
        """
        return True
        
    def acceptNameChange(self, oldName, newName):
        """
        Return False to decline the client to change
        it's name (after setting the inital name)
        """
        return True
        
    def acceptChannelJoin(self, channelName):
        """
        Return False to stop the client from joining the specified
        channel
        """
        return True
        
    def acceptChannelMessage(self, channel, message, subchannel):
        """
        Return False to stop the channel message from being
        sent to the other channel members
        """
        return True

    def acceptPrivateMessage(self, channel, connection, message, subchannel):
        """
        Return False to stop the private message from reaching
        the recipient
        """
        return True

    def acceptChannelLeave(self, channel):
        """
        Return False to stop the client from leaving the specified
        channel
        """
        return True
        
    # event-like
    
    def connectionAccepted(self):
        """
        Called when the connection has been accepted, and the
        client notified.
        """
        
    def nameSet(self, name):
        """
        Called after the name has been set
        """

    def nameChanged(self, oldName, newName):
        """
        Called when the client has changed name
        """
        
    def messageReceived(self, message, subchannel):
        """
        Called upon receiving a client message.
        
        @type message: L{Message} object
        """
    
    def channelJoined(self, channel):
        """
        Called when the client has been let into
        a channel
        """
        
    def channelMessageReceived(self, channel, message, subchannel):
        """
        Called after a channel message has been relayed.
        
        @type message: lacewing.moo.packetloaders.message.Message object
        """

    def privateMessageReceived(self, channel, player, message, subchannel):
        """
        Called after a private message has been relayed.
        
        @type message: lacewing.moo.packetloaders.message.Message object
        """

    def channelLeft(self, channel):
        """
        Called when the client has left a channel
        """
    
    def getHost(self):
        """
        This method returns the IP address of the connection.
        Override if you would like to hide/edit the IP
        """
        return self.transport.getPeer().host
        
    # action-like
    
    def changeName(self, name):
        oldName = self.name
        
        self.name = name
        
        newChanged = PlayerChanged()
        newChanged.setConnection(self)
        
        for channel in self.channels.values():
            channel.sendLoader(newChanged)
        
        self.nameChanged(oldName, name)
    
    def sendMessage(value, subchannel, type = None):
        """
        Sends a message to the other end
        """
        newValue = Message(**self.settings)
        newValue.type = type or detectType(value)
        newValue.value = value
        newMessage = FromChannelMessage()
        newMessage.setServer()
        newMessage.message = newValue
        newMessage.subchannel = subchannel
        self.sendLoader(newMessage)
    
    def sendChannelMessage(self, channel, *arg, **kw):
        channel.sendMessage(self, *arg, **kw)
    
    def joinChannel(self, channelName):
        newChannel = self.factory.createChannel(channelName)
        newChannel.addConnection(self)
        self.channelJoined(newChannel)
    
    def leaveChannel(self, channel):
        if not channel.removeConnection(self):
            self.factory.destroyChannel(channel)
        self.channelLeft(channel)

class MooServerFactory(ServerFactory):
    motd = ''

    channels = None
    connections = None
    
    channelPool = None
    userPool = None
    
    def __init__(self, **settings):
        self.settings = settings
        self.channels = MultikeyDict()
        self.connections = MultikeyDict()
        self.userPool = IDPool(1)
        self.channelPool = IDPool(1)
    
    def createChannel(self, channelName):
        try:
            channel, = self.channels[channelName]
            return channel
        except KeyError:
            newId = self.channelPool.popId()
            newChannel = Channel(channelName, newId, self)
            self.channels[channelName, newId] = newChannel
            self.channelCreated(newChannel)
            return newChannel
    
    def destroyChannel(self, channel):
        del self.channels[channel]
        self.channelDestroyed(channel)
        
    def channelCreated(self, channel):
        """
        Called when a new channel is
        created
        """

    def channelDestroyed(self, channel):
        """
        Called when the channel is destroyed because
        it is now empty
        """