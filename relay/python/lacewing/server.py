# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
The server classes.
"""

import sys

from twisted.internet import reactor
from twisted.internet import protocol
from twisted.internet.task import LoopingCall

from lacewing.protocol import BaseProtocol
from lacewing.idpool import IDPool
from lacewing.multidict import MultikeyDict
from lacewing.packet import ServerPacket, ClientPacket
from lacewing.packetloaders import client, server
from lacewing.packetloaders.common import (detectType, CONNECT, SET_NAME, 
    JOIN_CHANNEL, LEAVE_CHANNEL, CHANNEL_LIST)
import lacewing

class ServerChannel(object):
    """
    Represents a channel.

    @ivar id: ID of the channel.
    @ivar name: The name of the channel.
    @ivar connections: List of the clients in the channel.
    @ivar autoClose: True if this channel will close after the master leaves
    @ivar master: The master connection of this channel
    @type master: L{ServerProtocol} object
    """

    id = None
    name = None
    hidden = False
    autoClose = False
    master = None

    def __init__(self, name, id, hidden, autoClose, master):
        self.name = name
        self.id = id
        self.hidden = hidden
        self.autoClose = autoClose
        self.master = master
        self.connections = MultikeyDict()

    def addConnection(self, connection):
        """
        Add a client to the channel, and notify the other clients.

        @param connection: The protocol instance to add.
        """

        toClient = server.Peer()
        toClient.name = connection.name
        toClient.peer = connection.id
        toClient.channel = self.id
        toClient.isMaster = connection is self.master
        self.sendLoader(toClient)
        self.connections[connection.name, connection.id] = connection

    def removeConnection(self, connection):
        """
        Remove a client from the channel, and notify the other
        clients.

        @param connection: The protocol instance to remove.
        """

        # tell the other clients this client is leaving
        toClient = server.Peer()
        toClient.peer = connection.id
        toClient.channel = self.id
        self.sendLoader(toClient, [connection])
        # remove client
        del self.connections[connection]
        
        # close the channel if autoClose is set, and the removed client was
        # the master
        if connection is self.master and self.autoClose:
            close = len(self.connections) > 0
            for connection in self.connections.values():
                connection.leaveChannel(self)
                connection.channelLeft(self)
            return close
        
        return len(self.connections) > 0

    def sendMessage(self, message, subchannel = 0, fromConnection = None,
                    asObject = False, typeName = None, **settings):
        """
        Send a channel message from the given protocol instance.

        @param fromConnection: The client that the message is sent
        from (or None if the message is from the server).
        @param message: The message to send.
        @type message: str/number/ByteReader
        @param subchannel: The subchannel to send the message on.
        @type subchannel: number
        @param typeName: If not specified, the type will be 
        automatically detected (see L{lacewing.packetloaders.common.DATA_TYPES}
        for possible values)
        """
        if typeName is None:
            typeName = detectType(message)
        if asObject:
            if fromConnection is None:
                newMessage = server.ObjectServerChannelMessage()
            else:
                newMessage = server.ObjectChannelMessage()
        else:
            if fromConnection is None:
                newMessage = server.BinaryServerChannelMessage()
            else:
                newMessage = server.BinaryChannelMessage()
        newMessage.channel = self.id
        newMessage.value = message
        newMessage.subchannel = subchannel
        if fromConnection is not None:
            newMessage.peer = fromConnection.id
        newMessage.setDataType(typeName)
        self.sendLoader(newMessage, [fromConnection], **settings)

    def sendPrivateMessage(self, message, subchannel, sender, recipient, 
                           asObject = False, typeName = None, **settings):
        """
        Send a message from this client to a recipient

        @type message: str/number/ByteReader
        @param subchannel: The subchannel of the message in the range 0-256
        @param sender: Sending connection
        @type sender: L{ServerProtocol} object
        @param recipient: Connection to send message to
        @type recipient: L{ServerProtocol} object
        @param typeName: If not specified, the type will be 
        automatically detected (see L{lacewing.packetloaders.common.DATA_TYPES}
        for possible values)
        """
        if typeName is None:
            typeName = detectType(message)
        if asObject:
            newMessage = server.ObjectPeerMessage()
        else:
            newMessage = server.BinaryPeerMessage()
        newMessage.channel = self.id
        newMessage.value = message
        newMessage.subchannel = subchannel
        newMessage.peer = sender.id
        newMessage.setDataType(typeName)
        recipient.sendLoader(newMessage, **settings)

    def sendLoader(self, type, notClients = [], **settings):
        """
        Send the specified type to all the clients in the channel,
        apart from notClients if specified.
        """

        toClients = [connection for connection in self.connections.values()
                    if connection not in notClients]

        for client in toClients:
            client.sendLoader(type, **settings)

class ServerProtocol(BaseProtocol):
    """
    The server protocol.
    @ivar latency: The latency between the server and client.
    @ivar datagramPort: The UDP port that the connection 
        sends/receives data on.
    """
    _timeoutCall = None

    _currentPing = None
    _pingTime = None # time when ping was sent
    latency = None # latency between server and client in seconds

    datagramPort = None
    _receivePacket = ClientPacket
    
    _firstByte = True

    def connectionMade(self):
        """
        When a connection is made, a timeout timer is started,
        and if the user does not request connection acceptance
        within reasonable time, disconnect (to prevent flood attacks).
        """
        BaseProtocol.connectionMade(self)
        timeOut = self.factory.timeOut
        if timeOut is not None:
            self._timeoutCall = reactor.callLater(timeOut, self._timedOut)
    
    def _timedOut(self):
        self.timeoutCall = None
        self.disconnect(CONNECT, 'Connection timed out')

    def ping(self, timeOut = None):
        """
        Ping the client with the given timeout.
        @param timeOut: The number of seconds the client has to
        respond to the ping.
        """
        if self._pingTime is not None:
            return
        newPing = server.Ping()
        self.sendLoader(newPing)
        self._pingTime = reactor.seconds()
        timeOut = timeOut or self.factory.maxPing
        if timeOut is not None:
            self._currentPing = reactor.callLater(timeOut, self._pingTimedOut)
    
    def _pingTimedOut(self):
        self._currentPing = None
        self.disconnect(CONNECT, 'No ping response')

    def connectionLost(self, reason):
        """
        Once the connection is lost, put back the client ID into the pool,
        and remove the client from all the channels it has signed on to.
        """
        BaseProtocol.connectionLost(self, reason)
        factory = self.factory
        if self._timeoutCall is not None and self._timeoutCall.active():
            self._timeoutCall.cancel()
            self._timeoutCall = None
        if self._currentPing is not None and self._currentPing.active():
            self._currentPing.cancel()
            self._currentPing = None
        
        for channel in self.channels.values():
            self.leaveChannel(channel)
        if self.isAccepted:
            factory.userPool.putBack(self.id)
            del self.factory.connections[self]
    
    def dataReceived(self, data):
        if self._firstByte:
            if data[0] != '\x00':
                # we don't support the HTTP relay
                self.disconnect()
                return
            data = data[1:]
            self._firstByte = False
        BaseProtocol.dataReceived(self, data)

    def loaderReceived(self, loader, isDatagram = False):
        packetId = loader.id
        if packetId == client.Request.id:
            if loader.request == CONNECT:
                if self.isAccepted:
                    self.disconnect(CONNECT, 'Already accepted')
                    return
                
                if self._timeoutCall is not None:
                    self._timeoutCall.cancel()
                    self._timeoutCall = None
                
                factory = self.factory

                if loader.version != self.revision:
                    self.disconnect(CONNECT, 'Invalid revision')
                    return

                # connection checks
                
                acceptedConnections = [connection for connection in
                    factory.connections.values() if connection.isAccepted]

                if len(acceptedConnections)+1 > self.factory.maxUsers:
                    self.disconnect(CONNECT, 'Server full')
                    return

                # if hook is not interrupted
                if self.acceptConnection(loader) == False:
                    return
                self.id = factory.userPool.pop() # select id for this player

                self.factory.connections[(self.id,)] = self

                newWelcome = server.Response()
                newWelcome.response = CONNECT
                newWelcome.playerId = self.id
                newWelcome.success = True
                newWelcome.welcome = self.factory.getWelcomeMessage(self)
                self.sendLoader(newWelcome)
                self.isAccepted = True
                self.connectionAccepted(loader)
            
            elif loader.request == SET_NAME:
                name = loader.name
                
                if self.acceptLogin(name) == False:
                    return

                if not self.validName(name):
                    self.warn(SET_NAME, 'Invalid name', name = name)
                    return
                
                for channel in self.channels.values():
                    if name in channel.connections:
                        self.warn(SET_NAME, 'Name already taken', name = name)
                        return

                if not self.loggedIn:
                    self.setName(name)
                    self.loginAccepted(name)
                else:
                    self.setName(name)
                    self.nameChanged(name)
            
            elif loader.request == JOIN_CHANNEL:
                channelName = loader.name
                if not self.loggedIn:
                    self.disconnect(JOIN_CHANNEL, 'Name not set',
                        name = channelName)
                    return
                if channelName in self.channels:
                    self.warn(JOIN_CHANNEL, 'Already part of channel',
                        name = channelName)
                    return
                if self.acceptChannelJoin(channelName) == False:
                    return
                if not self.validName(channelName):
                    self.warn(JOIN_CHANNEL, 'Invalid channel name',
                        name = channelName)
                    return
                try:
                    channel, = self.factory.channels[channelName]
                    if self.name in channel.connections:
                        self.warn(JOIN_CHANNEL, 'Name already taken',
                            name = channelName)
                        return
                except KeyError:
                    pass
                flags = loader.flags
                channel = self.joinChannel(channelName, flags['HideChannel'], 
                    flags['AutoClose'])
                self.channelJoined(channel)
            
            elif loader.request == LEAVE_CHANNEL:
                channelId = loader.channel
                if not channelId in self.channels:
                    self.warn(LEAVE_CHANNEL, 'No such channel', 
                        channel = channelId)
                    return
                channel, = self.channels[channelId]
                if self.acceptChannelLeave(channel) == False:
                    return
                self.leaveChannel(channel)
                self.channelLeft(channel)
            
            elif loader.request == CHANNEL_LIST:
                if not self.factory.channelListing:
                    self.disconnect(CHANNEL_LIST, 'Channel listing not enabled')
                    return
                if self.acceptChannelListRequest() == False:
                    return
                channels = []
                for channel in self.factory.channels.values():
                    if not channel.hidden:
                        channels.append(
                            (channel.name, len(channel.connections)))
                self.sendChannelList(channels)
                self.channelListSent()

        elif packetId in (client.BinaryServerMessage.id, 
                          client.ObjectServerMessage.id):
            self.messageReceived(loader)

        elif packetId in (client.BinaryChannelMessage.id,
                          client.ObjectChannelMessage.id):
            channelId = loader.channel

            if not channelId in self.channels:
                return

            channel, = self.channels[channelId]

            if self.acceptChannelMessage(channel, loader) == False:
                return
            channel.sendMessage(loader.value, loader.subchannel, self,
                typeName = loader.getDataType(), asObject = loader.isObject,
                asDatagram = loader.settings.get('datagram', False))
            self.channelMessageReceived(channel, loader)

        elif packetId in (client.BinaryPeerMessage.id,
                          client.ObjectPeerMessage.id):
            channelId = loader.channel
            
            if channelId not in self.channels:
                return
                
            channel, = self.channels[channelId]
            try:
                player, = channel.connections[loader.peer]
            except KeyError:
                return
            if self.acceptPrivateMessage(channel, player, loader) == False:
                return
            channel.sendPrivateMessage(loader.value, loader.subchannel,
                self, player, typeName = loader.getDataType(), 
                asObject = loader.isObject,
                asDatagram = loader.settings.get('datagram', False))
            self.privateMessageReceived(channel, player, loader)
        
        elif packetId == client.ChannelMaster.id:
            if not self.factory.masterRights:
                return
            channelId = loader.channel
            if channelId not in self.channels:
                return
            channel, = self.channels[channelId]
            if channel.master is not self:
                return
            try:
                peer, = channel.connections[loader.peer]
            except KeyError:
                return
            action = loader.action
            if self.acceptMasterAction(channel, peer, action) == False:
                return
            if action == client.MASTER_KICK:
                peer.leaveChannel(channel)
            self.masterActionExecuted(channel, peer, action)

        elif packetId == client.UDPHello.id and isDatagram: # enable UDP
            self.udpEnabled = True
            self.sendLoader(server.UDPWelcome(), True)

        elif packetId == client.Pong.id: # ping response
            if self._pingTime is None:
                self.disconnect(CONNECT, 'No pong requested')
                return
            if self._currentPing:
                self._currentPing.cancel()
                self._currentPing = None
            self.latency = reactor.seconds() - self._pingTime
            self._pingTime = None
            self.pongReceived(self.latency)
        else:
            print '(unexpected packet: %r)' % loader
            self.disconnect() # unexpected packet!
            
    def sendLoader(self, loader, asDatagram = False):
        """
        Sends a packetloader to the client
        @param loader: The packetloader to send.
        @param asDatagram: True if the packet should be sent as a datagram 
        packet
        """
        # if the sender wants the packet sent as UDP, it has to be 
        # enabled by the server also.
        if asDatagram:
            if (self.factory.datagram is not None 
            and self.datagramPort is not None):
                self.factory.datagram.sendLoader(self, loader)
        else:
            newPacket = ServerPacket()
            newPacket.loader = loader
            data = newPacket.generate()
            self.transport.write(data)

    def disconnect(self, response = None, *arg, **kw):
        """
        Disconnect the the user for a specific reason.
        """
        if response is not None:
            self.warn(response, **kw)
        self.transport.loseConnection()

    def warn(self, response, warning = '', name = None, channel = None):
        """
        Notify the client about a denial.
        """
        newResponse = server.Response()
        newResponse.response = response
        newResponse.success = False
        newResponse.value = warning
        newResponse.name = name
        newResponse.channel = channel
        newResponse.playerId = self.id
        self.sendLoader(newResponse)

    def sendMessage(self, message, subchannel, channel = None, typeName = None, 
                    asObject = False, asDatagram = False):
        """
        Send a direct message to the client.

        @type message: str/number/ByteReader
        @param subchannel: The subchannel of the message in the range 0-256
        @param typeName: If not specified, the type will be 
        automatically detected (see L{lacewing.packetloaders.common.DATA_TYPES}
        for possible values)
        """
        if typeName is None:
            typeName = detectType(message)
        if asObject:
            if channel is None:
                newMessage = server.ObjectServerMessage()
            else:
                newMessage = server.ObjectServerChannelMessage()
        else:
            if channel is None:
                newMessage = server.BinaryServerMessage()
            else:
                newMessage = server.BinaryServerChannelMessage()
        newMessage.value = message
        newMessage.subchannel = subchannel
        if channel is not None:
            newMessage.channel = channel.id
        newMessage.setDataType(typeName)
        self.sendLoader(newMessage, asDatagram)

    def leaveChannel(self, channel):
        """
        Remove the client from a specific channel.

        @param channel: The channel to remove the client from.
        @type channel: ServerChannel object
        """
        factory = self.factory
        channels = self.channels
        id = channel.id
        name = channel.name
        if not channel.removeConnection(self):
            factory.destroyChannel(channel)
            factory.channelPool.putBack(id)
        del self.channels[channel]
        newResponse = server.Response()
        newResponse.success = True
        newResponse.response = LEAVE_CHANNEL
        newResponse.channel = channel.id
        self.sendLoader(newResponse)

    def joinChannel(self, channelName, hidden = False, autoClose = False):
        """
        Add the client to a new channel.
        If the channel does not already exist, it is created.
        
        @param hidden: If this connection is creating the channel, this
            will hide it from channel listing
        @param autoClose: If this connection is creating the channel,
            this will close the channel when the connection leaves
        """
        factory = self.factory
        newResponse = server.Response()
        newResponse.response = JOIN_CHANNEL
        newResponse.success = True
        newResponse.name = channelName
        newChannel = self.factory.createChannel(channelName, hidden, autoClose,
            self)
        newResponse.channel = newChannel.id
        newResponse.isMaster = newChannel.master is self
        for item in newChannel.connections.values():
            newResponse.addPeer(item.name, item.id, newChannel.master is item)
        self.sendLoader(newResponse)
        newChannel.addConnection(self)
        self.channels[channelName, newChannel.id] = newChannel
        return newChannel

    def setName(self, name):
        """
        Set the name of the client and notify it about it.
        """
        # if already named, remove name from namelist
        if self in self.factory.connections:
            del self.factory.connections[self]
        self.factory.connections[name, self.id] = self
        self.name = name
        newResponse = server.Response()
        newResponse.response = SET_NAME
        newResponse.success = True
        newResponse.name = name
        self.sendLoader(newResponse)
        self.loggedIn = True
 
        if self.channels:
            newClient = server.Peer()
            newClient.name = name
            newClient.peer = self.id

            for channel in self.channels.values():
                newClient.isMaster = channel.master is self
                newClient.channel = channel.id
                del channel.connections[self]
                channel.connections[self.id, name] = self
                channel.sendLoader(newClient, [self])

    def sendChannelList(self, channels):
        """
        Sends a list of channels and their peer count to the other end of
        the connection.
        
        @param channels: List of 2-item tuples (channelName, peerCount)
        """
        channelList = server.Response()
        channelList.response = CHANNEL_LIST
        channelList.success = True
        channelList.channels = channels
        self.sendLoader(channelList)

    def connectionAccepted(self, welcome):
        """
        Called when the server has accepted the requested connection.
        @type welcome: L{client.Request} object
        """

    def loginAccepted(self, name):
        """
        Called when the server has accepted the requested name.
        @arg name: The name of the client.
        @type name: str object
        """

    def channelListSent(self):
        """
        Called after a requested channel list is sent.
        """

    def nameChanged(self, name):
        """
        Called when the client changed name (after logging in).
        @type name: str
        @arg name: The name the client has changed to.
        """

    def messageReceived(self, message):
        """
        Called when a client message arrives.
        @type message: L{client.BinaryServerMessage} or 
        L{client.ObjectServerMessage}
        """

    def channelMessageReceived(self, channel, message):
        """
        Called when the client sends a channel message.
        @arg channel: the channel the user is sending to.
        @type message: L{client.BinaryChannelMessage} or 
        L{client.ObjectChannelMessage}
        """
        
    def privateMessageReceived(self, channel, recipient, message):
        """
        Called when the client has sent a private message
        @arg channel: the channel the user is sending from.
        @type message: L{client.BinaryPeerMessage} or
        L{client.ObjectPeerMessage}
        @type recipient: L{ServerProtocol} object
        """

    def channelJoined(self, channel):
        """
        Called when the client has joined a new channel.
        @arg channel: The channel the client has joined.
        """

    def channelLeft(self, channel):
        """
        Called when the client has left a channel.
        @arg channel: The channel the client has left.
        """
    
    def pongReceived(self, latency):
        """
        Called when a ping response has been received.
        @arg latency: Round-trip time in seconds between the client and
        the server.
        """

    def masterActionExecuted(self, channel, peer, action):
        """
        Called when after a master action has been executed.
        @arg channel: The channel the action took place in
        @arg peer: The peer the action was executed on
        @arg action: The action the master requested
        @type action: Currently, only {pylacewing.constants.MASTER_KICK}
        """

    # Server hooks, so you can abort or cancel an action

    def acceptLogin(self, name):
        """
        Cancels the client login if False is returned.
        Useful if the server specifies client names.
        @arg name: The requested client name.
        @return: Return False to cancel client login.
        @rtype: bool
        """

    def acceptNameChange(self, name):
        """
        Cancels the client name change if False is returned.
        Useful if the server specifies client names.
        @arg name: The requested client name.
        @return: Return False to cancel name change.
        @rtype: bool
        """

    def acceptConnection(self, welcome):
        """
        Cancels the connection accept response if False is returned.
        Useful for banning.
        @type welcome: L{client.Request} object
        @return: Return False to cancel connection acceptance.
        @rtype: bool
        """

    def acceptChannelMessage(self, channel, message):
        """
        Cancels the channel message if False is returned.
        Useful for blocking invalid messages.
        @type message: L{client.BinaryChannelMessage} or 
        L{client.ObjectChannelMessage}
        @return: Return False to stop the channel message
        before it is sent.
        @rtype: bool
        """

    def acceptPrivateMessage(self, channel, recipient, message):
        """
        Cancels the private message if False is returned.
        Useful for blocking invalid messages.
        @arg channel: the channel the user sending from.
        @type message: L{client.BinaryPeerMessage} or
        L{client.ObjectPeerMessage}
        @type recipient: L{ServerProtocol} object
        @return: Return False to stop the channel message
        before it is sent.
        @rtype: bool
        """

    def acceptChannelListRequest(self):
        """
        Cancels the sending of the channel list
        if False is returned.
        Useful for servers with a strict policy.
        @return: Return False to send no channel list.
        @rtype: bool
        """

    def acceptChannelJoin(self, channelName):
        """
        Cancels the channel join accept response if False is returned.
        Useful for channel-less servers.
        @arg channelName: The name of the channel.
        @return: Return False to cancel channel join acceptance.
        @rtype: bool
        """

    def acceptChannelLeave(self, channel):
        """
        Cancels the channel leave accept response if False is returned.
        Useful if the client may not leave the current channel.
        @arg channel: The channel the client is leaving.
        @return: Return False to cancel channel join acceptance.
        @rtype: bool
        """
    
    def acceptMasterAction(self, channel, peer, action):
        """
        Cancels a master action if False is returned.
        Useful for blocking some specific master actions.
        @arg channel: The channel the action is taking place in.
        @arg peer: The peer this action is executed on.
        @arg action: The action the master has requested.
        @type action: Currently, only {pylacewing.constants.MASTER_KICK}
        @rtype: bool
        """

class ServerDatagram(protocol.DatagramProtocol):
    def __init__(self, factory):
        """
        @param factory: The master TCP factory of the server.
        """
        self.factory = factory
        factory.datagram = self

    def datagramReceived(self, data, (host, port)):
        newPacket = ClientPacket(data, datagram = True)
        playerId = newPacket.fromId
        try:
            connection, = self.factory.connections[playerId]
            if (not connection.isAccepted
            or connection.transport.getPeer().host != host):
                return
            connection.datagramPort = port
            connection.loaderReceived(newPacket.loader, True)
        except (ValueError, KeyError):
            pass

    def sendLoader(self, connection, loader):
        address = (connection.transport.getPeer().host, connection.datagramPort)
        newPacket = ServerPacket(datagram = True)
        newPacket.loader = loader
        self.transport.write(newPacket.generate(), address)

class ServerFactory(protocol.ServerFactory):
    """
    The server factory.
    
    @ivar channelClass: This is the channel class that will be used when
        creating a new channel. Subclass L{ServerChannel} and replace this
        attribute if you want to change the behaviour of channels.
    @ivar maxPing: This is the time the client has to respond
        to pings (in seconds). Can be a number or None for no max ping (default)
    @ivar pingTime: The interval between pings in seconds
    @ivar maxUsers: The max number of users allowed on the server.
    @ivar timeOut: The number of seconds the client has to send a Hello packet
        before being disconnected. Can be a number or None for no timeout
    @ivar welcomeMessage: The message sent to accepted clients.
    @ivar ping: If True, pinging will be enabled on the server
    @ivar channelListing: If True, channelListing is enabled on the server
    @ivar masterRights: If True, this enables the autoclose feature for
        clients when creating channels
    """
    channelClass = ServerChannel
    timeOut = 8
    pingTime = 8
    maxPing = None
    maxUsers = 1000
    welcomeMessage = 'Welcome! Server is running pylacewing %s (%s)' % (
        lacewing.__version__, sys.platform)
    
    datagram = None
    ping = True
    channelListing = True
    masterRights = False
    
    _pinger = None

    def startFactory(self):
        self.connections = MultikeyDict()
        self.channels = MultikeyDict()
        self.userPool = IDPool()
        self.channelPool = IDPool()
        if self.ping:
            self._pinger = pinger = LoopingCall(self.globalPing)
            pinger.start(self.pingTime, False)
    
    def globalPing(self):
        """
        Pings all clients currently connected to the server
        """
        for connection in self.connections.values():
            connection.ping()
    
    def getWelcomeMessage(self, connection):
        """
        This method is called when a connection has been accepted, and
        a welcome message has to be sent. The default implementation just
        returns L{welcomeMessage}, but override this method to change that
        behaviour.
        @param connection: Connection that has been accepted
        @type connection: L{ServerProtocol} object
        @rtype: str
        """
        return self.welcomeMessage
    
    def createChannel(self, name, hidden, autoClose, master):
        if not self.masterRights:
            autoClose = False
        try:
            channel, = self.channels[name]
        except KeyError:
            id = self.channelPool.pop()
            channel = self.channelClass(name, id, hidden, autoClose, master)
            self.channels[name, id] = channel
            self.channelAdded(channel)
        return channel
    
    def destroyChannel(self, channel):
        del self.channels[channel]
        self.channelRemoved(channel)

    def channelRemoved(self, channel):
        """
        Called when a channel has no users in it, and
        is therefore removed.
        @arg channel: The channel that is being removed.
        """

    def channelAdded(self, channel):
        """
        Called when a new channel is created.
        @arg channel: The channel that has been created.
        """