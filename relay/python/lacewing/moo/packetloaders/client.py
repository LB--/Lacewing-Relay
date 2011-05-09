# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

from struct import unpack_from, pack

from lacewing.moo.packetloaders.common import (unpack_string, _MooLoader, 
    _ChannelMixin, _ConnectionMixin)

from lacewing.moo.packetloaders.message import Message

class SetName(_MooLoader, _ConnectionMixin):
    majorVersion = 3 # don't want our user to
    minorVersion = 0 # fill these in
    playerName = None
    
    def _load(self, data):
        (self.majorVersion,
        self.minorVersion,
        nameSize) = unpack_from('<BBI', data)
        self.playerName = unpack_string(data, nameSize, 6)
        return 6 + nameSize
        
    def generate(self):
        return pack('<BBI', self.majorVersion, self.minorVersion, 
            len(self.playerName)) + self.playerName

class ClientMessage(_MooLoader):
    subchannel = None
    message = None
    
    def _load(self, data):
        (self.subchannel,
        messageSize) = unpack_from('<HI', data)
        
        message = Message(**self.settings)
        message.load(str(buffer(data, 6, messageSize)))
        self.message = message
        
        return 6 + messageSize
    
    def generate(self):
        messageData = self.message.generate()
        
        return pack('<HI', self.subchannel, len(messageData)) + messageData

class JoinChannel(_MooLoader):
    channelName = None
    
    def _load(self, data):
        channelSize, = unpack_from('<I', data)
        self.channelName = unpack_string(data, channelSize, 4)
        
        return 4 + channelSize
    
    def generate(self):
        return pack('<I', len(self.channelName)) + self.channelName

class LeaveChannel(_MooLoader, _ChannelMixin):
    channelId = None
    
    def _load(self, data):
        self.channelId, = unpack_from('<I', data)
        return 4
    
    def generate(self):
        return pack('<I', self.channelId)

class ChangeName(_MooLoader):
    newName = None
    
    def _load(self, data):
        nameSize, = unpack_from('<I', data)
        self.newName = unpack_string(data, nameSize, 4)
        return 4 + nameSize
    
    def generate(self):
        return pack('<I', len(self.newName)) + self.newName

class ToChannelMessage(_MooLoader, _ChannelMixin):
    subchannel = None
    message = None
    channelId = None
    
    def _load(self, data):
        (self.subchannel,
        self.channelId,
        messageSize) = unpack_from('<HII', data)
        
        message = Message(**self.settings)
        message.load(str(buffer(data, 10, messageSize)))
        self.message = message
        
        return 10 + messageSize
    
    def generate(self):
        messageData = self.message.generate()
        
        return pack('<HII', self.subchannel, self.channelId, len(messageData)) + messageData

class PrivateMessage(_MooLoader, _ChannelMixin, _ConnectionMixin):
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
        message.load(str(buffer(data, 14, messageSize)))
        self.message = message
        
        return 14 + messageSize
    
    def generate(self):
        messageData = self.message.generate()
        
        return pack('<HIII', self.subchannel, self.channelId, 
            self.playerId, len(messageData)) + messageData

__all__ = ['SetName', 'ClientMessage', 'JoinChannel', 'LeaveChannel', 
    'ChangeName', 'ToChannelMessage', 'PrivateMessage']