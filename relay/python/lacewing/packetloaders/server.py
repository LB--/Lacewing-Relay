# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

from struct import Struct

from lacewing.baseloader import _BaseLoader, _EmptyLoader
from lacewing.bitdict import BitDict
from lacewing.packetloaders.common import (CONNECT, SET_NAME, JOIN_CHANNEL, 
    LEAVE_CHANNEL, CHANNEL_LIST)
from lacewing.packetloaders.common import (_BinaryMessageMixin, 
    _ObjectMessageMixin)

RESPONSE_DATA = Struct('<BB')
NAME_DATA = Struct('<B')
SUBCHANNEL_DATA = Struct('<B')
JOIN_CHANNEL_DATA = Struct('<BB')
CHANNEL_ID = Struct('<H')
YOUR_ID = Struct('<H')
PEER_DATA = Struct('<HBB')
CHANNEL_LIST_DATA = Struct('<HB')
CHANNEL_MESSAGE_DATA = Struct('<BH')
PEER_MESSAGE_DATA = Struct('<BHH')
SERVER_CHANNEL_MESSAGE_DATA = Struct('<BH')
NEW_PEER_DATA = Struct('<HHB')
PEER_LEAVE_DATA = Struct('<HH')

class ChannelPeer(_BaseLoader):
    id = None
    isMaster = None
    name = None
    
    def read(self, data):
        self.id, flags, size = PEER_DATA.unpack_from(data)
        self.isMaster = flags
        self.name = str(buffer(data, PEER_DATA.size, size))
        return PEER_DATA.size + size
    
    def generate(self):
        return PEER_DATA.pack(self.id, int(self.isMaster), 
            len(self.name)) + self.name

class Response(_BaseLoader):
    response = None
    success = None
    welcome = None
    name = None
    isMaster = None
    channel = None
    peers = None
    channels = None
    playerId = None
    value = None

    def read(self, data):
        self.response, self.success = RESPONSE_DATA.unpack_from(data)
        data = buffer(data, 2)
        if self.response == CONNECT and self.success:
            self.playerId, = YOUR_ID.unpack_from(data)
            self.welcome = str(buffer(data, 2))
        elif self.response == SET_NAME:
            size, = NAME_DATA.unpack_from(data)
            self.name = str(buffer(data, 1, size))
            data = buffer(data, 1 + size)
        elif self.response == JOIN_CHANNEL:
            self.isMaster, size = JOIN_CHANNEL_DATA.unpack_from(data)
            self.name = str(buffer(data, JOIN_CHANNEL_DATA.size, size))
            data = buffer(data, JOIN_CHANNEL_DATA.size + size)
            if self.success:
                self.channel, = CHANNEL_ID.unpack_from(data)
                data = buffer(data, 2)
                self.peers = peers = []
                while data:
                    peer = ChannelPeer()
                    peers.append(peer)
                    bytesRead = peer.read(data)
                    data = buffer(data, bytesRead)
        elif self.response == LEAVE_CHANNEL:
            self.channel, = CHANNEL_ID.unpack_from(data)
            data = buffer(data, 2)
        elif self.response == CHANNEL_LIST and self.success:
            self.channels = channels = []
            while data:
                count, size = CHANNEL_LIST_DATA.unpack_from(data)
                name = buffer(data, CHANNEL_LIST_DATA.size, size)
                data = buffer(data, CHANNEL_LIST_DATA.size + size)
                channels.append((name, count))
        if not self.success:
            self.value = str(data)
    
    def addPeer(self, name, id, isMaster):
        if self.peers is None:
            self.peers = []
        peer = ChannelPeer()
        peer.id = id
        peer.isMaster = isMaster
        peer.name = name
        self.peers.append(peer)

    def generate(self):
        data = RESPONSE_DATA.pack(self.response, self.success)
        if self.response == CONNECT:
            data += YOUR_ID.pack(self.playerId or 0)
            data += self.welcome or ''
        elif self.response == SET_NAME:
            data += NAME_DATA.pack(len(self.name)) + self.name
        elif self.response == JOIN_CHANNEL:
            data += JOIN_CHANNEL_DATA.pack(
                int(self.isMaster or 0), len(self.name)) + self.name
            if self.success:
                data += CHANNEL_ID.pack(self.channel)
                for item in self.peers or ():
                    data += item.generate()
        elif self.response == LEAVE_CHANNEL:
            data += CHANNEL_ID.pack(self.channel)
        elif self.response == CHANNEL_LIST and self.success:
            for (name, count) in self.channels:
                data += CHANNEL_LIST_DATA.pack(count, len(name)) + name
        if not self.success:
            data += self.value or ''
        return data

class _ServerMessage(_BaseLoader):
    subchannel = None
    def read(self, data):
        self.subchannel, = SUBCHANNEL_DATA.unpack_from(data)
        self.readMessage(buffer(data, 1))
    
    def generate(self):
        return SUBCHANNEL_DATA.pack(self.subchannel) + self.generateMessage()

class _PeerMessage(_BaseLoader):
    subchannel = None
    channel = None
    peer = None
    def read(self, data):
        (self.subchannel, self.channel, 
            self.peer) = PEER_MESSAGE_DATA.unpack_from(data)
        self.readMessage(buffer(data, PEER_MESSAGE_DATA.size))
    
    def generate(self):
        return PEER_MESSAGE_DATA.pack(self.subchannel, 
            self.channel, self.peer) + self.generateMessage()

class _ServerChannelMessage(_BaseLoader):
    subchannel = None
    channel = None
    def read(self, data):
        (self.subchannel, 
            self.channel) = SERVER_CHANNEL_MESSAGE_DATA.unpack_from(data)
        self.readMessage(buffer(data, SERVER_CHANNEL_MESSAGE_DATA.size))
    
    def generate(self):
        return SERVER_CHANNEL_MESSAGE_DATA.pack(self.subchannel, 
            self.channel) + self.generateMessage()

class BinaryServerMessage(_BinaryMessageMixin, _ServerMessage):
    pass

class BinaryChannelMessage(_BinaryMessageMixin, _PeerMessage):
    pass

class BinaryPeerMessage(_BinaryMessageMixin, _PeerMessage):
    pass

class BinaryServerChannelMessage(_BinaryMessageMixin, _ServerChannelMessage):
    pass

class ObjectServerMessage(_ObjectMessageMixin, _ServerMessage):
    pass

class ObjectChannelMessage(_ObjectMessageMixin, _PeerMessage):
    pass

class ObjectPeerMessage(_ObjectMessageMixin, _PeerMessage):
    pass

class ObjectServerChannelMessage(_ObjectMessageMixin, _ServerChannelMessage):
    pass

class Peer(_BaseLoader):
    channel = None
    peer = None
    isMaster = None
    name = None
    
    def read(self, data):
        if len(data) == PEER_LEAVE_DATA.size:
            self.channel, self.peer = PEER_LEAVE_DATA.unpack_from(data)
        else:
            self.channel, self.peer, self.isMaster = NEW_PEER_DATA.unpack_from(
                data)
            self.name = str(buffer(data, NEW_PEER_DATA.size))
    
    def generate(self):
        if self.isMaster is None and self.name is None:
            return PEER_LEAVE_DATA.pack(self.channel, self.peer)
        else:
            return NEW_PEER_DATA.pack(self.channel, self.peer, 
                self.isMaster) + self.name

class UDPWelcome(_EmptyLoader):
    pass

class Ping(_EmptyLoader):
    pass