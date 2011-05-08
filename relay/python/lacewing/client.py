# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
The client classes.
"""

from time import time

from twisted.internet import reactor, defer
from twisted.internet import protocol
from twisted.internet.task import LoopingCall

from lacewing.protocol import BaseProtocol
from lacewing.packet import ServerPacket, ClientPacket
from lacewing.packetloaders import server, client
from lacewing.multidict import MultikeyDict
from lacewing.packetloaders.common import (detectType, CONNECT, SET_NAME, 
    JOIN_CHANNEL, LEAVE_CHANNEL, CHANNEL_LIST)

class Peer(object):
    """
    Represents a remote client.
    @ivar name: The name of the client.
    @ivar id: The ID of the client
    @ivar master: True if this peer is the master of the channel
    """
    user = None
    name = None
    id = None
    master = False
    
    def __init__(self, name, id, master, channel):
        self.name = name
        self.id = id
        self.master = master
        self.channel = channel
        self.user = channel.user

    def sendMessage(self, message, subchannel, typeName = None, 
                    asObject = False, asDatagram = False):
        """
        Send a message to another peer
        
        @type message: str/number/ByteReader
        @param subchannel: The subchannel of the message in the range 0-256
        @param typeName: If not specified, the type will be 
        automatically detected (see L{lacewing.packetloaders.common.DATA_TYPES}
        for possible values)
        """
        if asObject:
            newMessage = client.ObjectPeerMessage()
        else:
            newMessage = client.BinaryPeerMessage()
        newMessage.value = message
        newMessage.subchannel = subchannel
        newMessage.channel = self.channel.id
        newMessage.peer = self.id
        newMessage.setDataType(typeName or detectType(message))
        self.user.sendLoader(newMessage, asDatagram)
    
    def kick(self, peer):
        pass

class ClientChannel(object):
    """
    Represents a server channel.
    @ivar id: The ID of the channel.
    @ivar name: The name of the channel.
    @ivar connections: A dict of signed on connections.
    """
    user = None

    id = None
    name = None

    def __init__(self, user):
        self.connections = MultikeyDict()
        self.user = user

    def sendMessage(self, message, subchannel, typeName = None, asObject = False,
                    asDatagram = False):
        """
        Send a message to the channel
        
        @type message: str/number/ByteReader
        @param subchannel: The subchannel of the message in the range 0-256
        @param typeName: If not specified, the type will be 
        automatically detected (see L{lacewing.packetloaders.common.DATA_TYPES}
        for possible values)
        """
        if typeName is None:
            typeName = detectType(message)
        if asObject:
            newMessage = client.ObjectChannelMessage()
        else:
            newMessage = client.BinaryChannelMessage()
        newMessage.value = message
        newMessage.subchannel = subchannel
        newMessage.channel = self.id
        newMessage.setDataType(typeName)
        self.user.sendLoader(newMessage, asDatagram)

class ClientProtocol(BaseProtocol):
    """
    The client protocol.
    
    @ivar datagram: The ClientDatagram protocol this connection is using for
        UDP handling
    """
    datagram = None
    _udpCheck = None # LoopingCall for checking if UDP enabled
    udpTimeout = 3000
    _loginName = None

    _channelList = None
    _channelListDefer = None
    _receivePacket = ServerPacket

    def connectionMade(self):
        """
        Sends a connection acceptance request to the server
        after the connection has been made.
        """
        BaseProtocol.connectionMade(self)
        self._udpCheck = LoopingCall(self._requestUDP)
        self._channelList = []
        self._channelListDefer = []
        hello = client.Request()
        hello.request = CONNECT
        hello.version = self.revision
        self.sendLoader(hello)

    def loaderReceived(self, loader, isDatagram = False):
        packetId = loader.id
        if packetId == server.Response.id:
            if not loader.success:
                self.serverDenied(loader)
                return
            response = loader.response
            if response == CONNECT:
                datagram = ClientDatagram(self)
                reactor.listenUDP(0, datagram)
                self.datagram = datagram
                self.serverGreeted(loader.welcome)
                self.id = loader.playerId
                self._enableUDP()
                
            elif response == SET_NAME:
                name = self.name = loader.name
                if self.loggedIn:
                    self.nameChanged(name)
                else:
                    self.loggedIn = True
                    self.loginAccepted(self.name)
                    
            elif response == JOIN_CHANNEL:
                channelId = loader.channel
                channelName = loader.name
                channel = ClientChannel(self)
                channel.id = channelId
                channel.name = channelName
                self.channels[channelId, channelName] = channel
                connections = channel.connections
                for peer in loader.peers:
                    clientId = peer.id
                    name = peer.name
                    peer = Peer(name, clientId, peer.isMaster, channel)
                    self.channelUserExists(channel, peer)
                    connections[clientId, name] = peer
                self.channelJoined(channel)
                
            elif response == LEAVE_CHANNEL:
                channel, = self.channels[loader.channel]
                del self.channels[channel]
                self.channelLeft(channel)
            
            elif response == CHANNEL_LIST:
                self._channelListDefer.pop(0).callback(loader.channels)
            return

        elif packetId == server.UDPWelcome.id and not self.udpEnabled:
            self._udpCheck.stop()
            self.isAccepted = True
            self.connectionAccepted()
    
        elif packetId in (server.BinaryServerMessage.id, 
                          server.ObjectServerMessage.id):
            self.messageReceived(loader)

        elif packetId in (server.BinaryChannelMessage.id, 
                          server.ObjectChannelMessage.id):
            channel, = self.channels[loader.channel]
            sender, = channel.connections[loader.peer]
            self.channelMessageReceived(channel, sender, loader)
        
        elif packetId == server.Ping.id:
            self.sendLoader(client.Pong())
            self.pingReceived()
        
        elif packetId == server.Peer.id:
            channel, = self.channels[loader.channel]
            connections = channel.connections
            clientId = loader.peer
            name = loader.name
            if name is not None:
                if not clientId in connections:
                    # create a new client instance
                    peer = Peer(name, clientId, loader.isMaster, channel)
                    self.channelUserJoined(channel, peer)
                else:
                    peer, = channel.connections[clientId]
                    del channel.connections[peer]
                    peer.name = name
                    peer.master = loader.isMaster
                    self.channelUserChanged(channel, peer)
                connections[clientId, name] = peer
            else:
                peer, = channel.connections[clientId]
                self.channelUserLeft(channel, peer)
                del connections[peer]

        elif packetId in (server.BinaryPeerMessage.id, 
                          server.ObjectPeerMessage.id):
            channel, = self.channels[loader.channel]
            peer, = channel.connections[loader.peer]
            self.privateMessageReceived(channel, peer, loader)

        else:
            # packet type not known
            raise NotImplementedError('unknown packet type (%s)' % packetId)

    def sendLoader(self, loader, asDatagram = False):
        """
        Sends a packetloader to the server
        @param loader: The packetloader to send.
        """
        if asDatagram:
            self.datagram.sendLoader(loader)
        else:
            newPacket = ClientPacket()
            newPacket.loader = loader
            self.transport.write(newPacket.generate())
            
    def _enableUDP(self):
        self._udpCheck.start(self.udpTimeout / 1000)
        
    def _requestUDP(self):
        self.sendLoader(client.UDPHello(), True)

    def requestChannelList(self):
        """
        Request a channel list from the server.
        
        @return: A deferred that will fire with 
        a list of ChannelList objects
        """
        d = defer.Deferred()
        self._channelListDefer.append(d)
        request = client.Request()
        request.request = CHANNEL_LIST
        self.sendLoader(request)
        return d

    def sendMessage(self, message, subchannel, typeName = None, 
                    asObject = False, asDatagram = False):
        """
        Send a message directly to the server.
        @param message: The message to send.
        @type message: str/number/ByteReader
        @param subchannel: A subchannel in the range 0-256
        @param typeName: If not specified, the type will be 
        automatically detected (see L{lacewing.packetloaders.common.DATA_TYPES}
        for possible values)
        """
        if typeName is None:
            typeName = detectType(message)
        if asObject:
            newMessage = client.ObjectServerMessage()
        else:
            newMessage = client.BinaryServerMessage()
        newMessage.value = message
        newMessage.subchannel = subchannel
        newMessage.setDataType(typeName)
        self.sendLoader(newMessage, asDatagram)

    def joinChannel(self, channelName, hidden = False, autoClose = False):
        """
        Join a channel by name.
        
        @param hidden: If this connection is creating the channel, this will
            hide the channel from channel listing
        @param autoClose: If this connection is creating the channel and if
            master rights are enabled on the server, this will close the
            channel when the client leaves
        """
        request = client.Request()
        request.request = JOIN_CHANNEL
        request.name = channelName
        request.flags['HideChannel'] = hidden
        request.flags['AutoClose'] = autoClose
        self.sendLoader(request)

    def leaveChannel(self, channel):
        """
        Leave a channel.
        @param channel: The channel to leave.
        @type channel: str, number, or OChannel object
        """
        channel, = self.channels[channel]
        request = client.Request()
        request.request = LEAVE_CHANNEL
        request.channel = channel.id
        self.sendLoader(request)

    def setName(self,name):
        """
        Request a name change.
        @param name: The name to request.
        @type name: str
        """
        newName = client.Request()
        newName.request = SET_NAME
        newName.name = name
        self.sendLoader(newName)

    # events specific to the client

    def serverGreeted(self, welcome):
        """
        Called when the server has responded to our welcome.
        """

    def connectionAccepted(self):
        """
        Called when the server has accepted the requested connection.
        """

    def loginAccepted(self, name):
        """
        Called when the server has accepted the requested name
        (or has specified one itself).
        @arg name: The name of the client.
        @type name: str object
        """

    def nameChanged(self, name):
        """
        Called when the server has accepted name change
        (or has given the client a new one).
        @type name: str
        @arg name: The new name for the client.
        """

    def serverDenied(self, response):
        """
        Called when the server disconnects or when the
        server denies an client request.
        @arg response: The response of the action.
        """

    def messageReceived(self, message):
        """
        Called when a server message arrives.
        @type message: L{server.BinaryServerMessage} or 
        L{server.ObjectServerMessage}
        """

    def channelMessageReceived(self, channel, user, message):
        """
        Called when a message from a channel arrives.
        @arg channel: the channel the message came from.
        @arg user: the sender of the message.
        @type message: L{server.BinaryChannelMessage} or 
        L{server.ObjectChannelMessage}
        """

    def privateMessageReceived(self, channel, sender, message):
        """
        Called when a private message has been received
        @arg channel: the channel the user is sending from.
        @type message: L{server.BinaryChannelMessage} or 
        L{server.ObjectChannelMessage}
        @type sender: L{Peer} object
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

    def channelUserChanged(self, channel, client):
        """
        Called when a client in the channel has changed name.
        @arg channel: The channel the client resides in.
        """
    
    def pingReceived(self):
        """
        Called when a server ping has been received.
        """

class ClientDatagram(protocol.DatagramProtocol):
    connection = None
    host = None
    port = None
    def __init__(self, connection):
        """
        @param connection: The ClientProtocol instance this datagram should use
        """
        self.connection = connection
        port = connection.transport.getPeer()
        self.host, self.port = port.host, port.port
    
    def startProtocol(self):
        self.transport.connect(self.host, self.port)

    def datagramReceived(self, data, (host, port)):
        newPacket = ServerPacket(data, datagram = True)
        self.connection.loaderReceived(newPacket.loader, True)

    def sendLoader(self, loader):
        newPacket = ClientPacket(datagram = True)
        newPacket.fromId = self.connection.id
        newPacket.loader = loader
        self.transport.write(newPacket.generate())