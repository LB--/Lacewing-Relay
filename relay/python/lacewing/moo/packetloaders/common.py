# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

import struct
from struct import unpack_from, pack

from lacewing.baseloader import _BaseLoader
from lacewing.exceptions import OutOfData

def unpack_string(data, size, offset = 0):
    newData = buffer(data, offset, size)
    if len(newData) < size:
        raise OutOfData
    return str(newData)

class _MooLoader(_BaseLoader):
    settings = None
    def __init__(self, **settings):
        self.settings = settings

    def load(self, data):
        try:
            return self._load(data)
        except struct.error:
            raise OutOfData

class _ChannelMixin:
    def setChannel(self, channel):
        if hasattr(self, 'channelId'):
            self.channelId = channel.id
        if hasattr(self, 'channelName'):
            self.channelName = channel.name
        if hasattr(self, 'masterId'):
            self.masterId = channel.getMaster().id

class _ConnectionMixin:
    def setConnection(self, connection):
        if hasattr(self, 'playerName'):
            self.playerName = connection.name
        if hasattr(self, 'playerId'):
            self.playerId = connection.id
        if hasattr(self, 'playerIp'):
            self.playerIp = connection.transport.getPeer().host