# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

from struct import Struct

from lacewing.baseloader import _BaseLoader, _EmptyLoader
from lacewing.bitdict import BitDict
from lacewing.packetloaders.common import (_BinaryMessageMixin, 
    _ObjectMessageMixin)
from lacewing.packetloaders.common import (CONNECT, SET_NAME, JOIN_CHANNEL, 
    LEAVE_CHANNEL)

REQUEST_TYPE = Struct('<B')
FLAG_DATA = Struct('<B')
CHANNEL_DATA = Struct('<H')
SUBCHANNEL_DATA = Struct('<B')
CHANNEL_MESSAGE_DATA = Struct('<BH')
PEER_MESSAGE_DATA = Struct('<BHH')
CHANNEL_MASTER_DATA = Struct('<HBH')

MASTER_KICK, = xrange(1)

CHANNEL_FLAGS = BitDict(
    'HideChannel',
    'AutoClose'
)

class Request(_BaseLoader):
    request = None
    version = None
    name = None
    flags = None
    channel = None
    
    def initialize(self):
        self.flags = CHANNEL_FLAGS.copy()

    def read(self, data):
        request, = REQUEST_TYPE.unpack_from(data)
        data = buffer(data, 1)
        self.request = request
        if request == CONNECT:
            self.version = str(data)
        elif request == SET_NAME:
            self.name = str(data)
        elif request == JOIN_CHANNEL:
            flags, = FLAG_DATA.unpack_from(data)
            self.flags.setFlags(flags)
            self.name = str(buffer(data, FLAG_DATA.size))
        elif request == LEAVE_CHANNEL:
            self.channel, = CHANNEL_DATA.unpack_from(data)

    def generate(self):
        request = self.request
        requestByte = REQUEST_TYPE.pack(request)
        if request == CONNECT:
            return requestByte + self.version
        elif request == SET_NAME:
            return requestByte + self.name
        elif request == JOIN_CHANNEL:
            return (requestByte + FLAG_DATA.pack(
                self.flags.getFlags()) + self.name)
        elif request == LEAVE_CHANNEL:
            return requestByte + CHANNEL_DATA.pack(self.channel)
        else:
            return requestByte

class _ServerMessage(_BaseLoader):
    subchannel = None
    def read(self, data):
        self.subchannel, = SUBCHANNEL_DATA.unpack_from(data)
        self.readMessage(buffer(data, 1))
    
    def generate(self):
        return SUBCHANNEL_DATA.pack(self.subchannel) + self.generateMessage()

class _ChannelMessage(_BaseLoader):
    subchannel = None
    channel = None
    def read(self, data):
        self.subchannel, self.channel = CHANNEL_MESSAGE_DATA.unpack_from(data)
        self.readMessage(buffer(data, CHANNEL_MESSAGE_DATA.size))
    
    def generate(self):
        return CHANNEL_MESSAGE_DATA.pack(self.subchannel, 
            self.channel) + self.generateMessage()

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

class BinaryServerMessage(_BinaryMessageMixin, _ServerMessage):
    pass

class BinaryChannelMessage(_BinaryMessageMixin, _ChannelMessage):
    pass

class BinaryPeerMessage(_BinaryMessageMixin, _PeerMessage):
    pass

class ObjectServerMessage(_ObjectMessageMixin, _ServerMessage):
    pass

class ObjectChannelMessage(_ObjectMessageMixin, _ChannelMessage):
    pass

class ObjectPeerMessage(_ObjectMessageMixin, _PeerMessage):
    pass

class UDPHello(_EmptyLoader):
    pass

class Pong(_EmptyLoader):
    pass

class ChannelMaster(_BaseLoader):
    channel = None
    action = None
    peer = None

    def read(self, data):
        self.channel, self.action, self.peer = CHANNEL_MASTER_DATA.unpack_from(
            data)
    
    def generate(self):
        return CHANNEL_MASTER_DATA.pack(self.channel, self.action, self.peer)