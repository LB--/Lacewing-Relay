# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

from struct import unpack_from, pack

from lacewing.moo.packetloaders.common import (unpack_string, _MooLoader, 
    _ChannelMixin, _ConnectionMixin)

from lacewing.moo.packetloaders.message import Message

class MOTD(_MooLoader):
    motd = None
    
    def _load(self, data):
        motdSize, = unpack_from('<I', data)
        self.motd = unpack_string(data, motdSize, 4)
        return 4 + motdSize
        
    def generate(self):
        return pack('<I', len(self.motd)) + self.motd

class AssignedID(_MooLoader, _ConnectionMixin):
    majorVersion = 3 # don't want our user to
    minorVersion = 0 # fill these in
    playerId = None

    def _load(self, data):
        (self.majorVersion,
        self.minorVersion,
        self.playerId) = unpack_from('<BBI', data)
        return 6
        
    def generate(self):
        return pack('<BBI', self.majorVersion, self.minorVersion, 
            self.playerId)


class _PlayerCommon(_MooLoader, _ChannelMixin, _ConnectionMixin):
    playerId = None
    channelId = None
    masterId = None
    playerName = None
    playerIp = None

    def _load(self, data):
        (self.playerId,
        self.channelId,
        self.masterId,
        playerNameSize) = unpack_from('<IIII', data)
        self.playerName = unpack_string(data, playerNameSize, 16)
        offset = 16 + playerNameSize
        playerIpSize, = unpack_from('<I', data, offset)
        offset += 4
        self.playerIp = unpack_string(data, playerIpSize, offset)
        offset += playerIpSize
        return offset
        
    def generate(self):
        return ''.join([
            pack('<IIII', self.playerId, self.channelId, self.masterId, 
            len(self.playerName)), self.playerName, pack('<I', len(self.playerIp)),
            self.playerIp
        ])

class PlayerExists(_PlayerCommon):
    pass

class PlayerJoined(_PlayerCommon):
    pass

class PlayerLeft(_MooLoader, _ChannelMixin, _ConnectionMixin):
    playerId = None
    channelId = None
    masterId = None

    def _load(self, data):
        (self.playerId,
        self.channelId,
        self.masterId) = unpack_from('<III', data)
        return 12
        
    def generate(self):
        return ''.join([
            pack('<III', self.playerId, self.channelId, self.masterId)
        ])

class PlayerChanged(_MooLoader, _ConnectionMixin):
    playerId = None
    playerName = None

    def _load(self, data):
        (self.playerId,
        playerNameSize) = unpack_from('<II', data)
        self.playerName = unpack_string(data, playerNameSize, 8)
        offset = 8 + playerNameSize
        otherPlayerId, otherNameSize = unpack_from('<II', data, offset)
        offset += 8
        otherPlayerName = unpack_string(data, otherNameSize, offset)
        offset += otherNameSize
        if (self.playerName != otherPlayerName or
            self.playerId != otherPlayerId):
            raise NotImplementedError
            
        return offset
        
    def generate(self):
        return ''.join([
            pack('<II', self.playerId, len(self.playerName)), self.playerName,
            pack('<II', self.playerId, len(self.playerName)), self.playerName
        ])

class ChannelJoined(_MooLoader, _ChannelMixin, _ConnectionMixin):
    playerId = None
    channelId = None
    masterId = None
    playerName = None
    playerIp = None
    channelName = None

    def _load(self, data):
        (self.playerId,
        self.channelId,
        self.masterId,
        playerNameSize) = unpack_from('<IIII', data)
        self.playerName = unpack_string(data, playerNameSize, 16)
        offset = 16 + playerNameSize
        playerIpSize, = unpack_from('<I', data, offset)
        offset += 4
        self.playerIp = unpack_string(data, playerIpSize, offset)
        offset += playerIpSize
        channelSize, = unpack_from('<I', data, offset)
        offset += 4
        self.channelName = unpack_string(data, channelSize, offset)
        offset += channelSize
        return offset
        
    def generate(self):
        return ''.join([
            pack('<IIII', self.playerId, self.channelId, self.masterId, 
            len(self.playerName)), self.playerName, pack('<I', len(self.playerIp)),
            self.playerIp, pack('<I', len(self.channelName)), self.channelName
        ])

class FromChannelMessage(_MooLoader, _ChannelMixin, _ConnectionMixin):
    subchannel = None
    message = None
    channelId = None
    playerId = None
    
    def _load(self, data):
        (self.subchannel,
        self.channelId,
        self.playerId,
        messageSize) = unpack_from('<HIII', data)
        
        message = Message(**self.settings)
        message.load(unpack_string(data, messageSize, 14))
        self.message = message
        
        return 14 + messageSize
        
    def isServer(self):
        return (self.channelId == 0 and self.playerId == 0)
        
    def setServer(self):
        self.channelId = 0
        self.playerId = 0
    
    def generate(self):
        messageData = self.message.generate()
        
        return pack('<HIII', self.subchannel, self.channelId, 
            self.playerId, len(messageData)) + messageData

__all__ = ['MOTD', 'AssignedID', 'PlayerExists', 'PlayerJoined', 
    'PlayerLeft', 'PlayerChanged', 'ChannelJoined', 'FromChannelMessage']