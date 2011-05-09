# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

import struct
from struct import unpack_from, pack

from lacewing.moo.packetloaders.server import *
from lacewing.moo.packetloaders.client import *

from lacewing.moo.packetloaders.common import OutOfData

csTypes = {
    1 : ToChannelMessage,
    2 : ClientMessage,
    3 : PrivateMessage,
    4 : JoinChannel,
    5 : LeaveChannel,
    11 : ChangeName,
    12 : SetName,

}

scTypes = {
    1 : FromChannelMessage,
    5 : PlayerLeft,
    6 : PlayerJoined,
    7 : PlayerExists,
    8 : ChannelJoined,
    10 : MOTD,
    11 : PlayerChanged,
    12 : AssignedID,
}

class _Packet(object):
    loader = None
    
    def __init__(self, **settings):
        self.settings = settings
    
    def load(self, data):
        try:
            packetId, = unpack_from('<B', data)
        except struct.error:
            raise OutOfData
        loader = self._loaderDict[packetId](**self.settings)
        self.loader = loader
        return loader.load(data[1:]) + 1

    def generate(self):
        packetId = [k for (k, v) in self._loaderDict.iteritems() 
            if isinstance(self.loader, v)][0]
        return pack('<B', packetId) + self.loader.generate()

class ServerPacket(_Packet):
    _loaderDict = scTypes

class ClientPacket(_Packet):
    _loaderDict = csTypes