# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

from twisted.internet import protocol

from lacewing.moo.packetloaders.common import OutOfData

from lacewing.multidict import MultikeyDict

class MooProtocol(protocol.Protocol):
    name = None
    id = None
    
    isAccepted = False

    channels = None
    _packetBuffer = ''

    def __init__(self):
        self.channels = MultikeyDict()
    
    def sendLoader(self, loader):
        newPacket = self._sendPacket(**self.settings)
        newPacket.loader = loader
        self.transport.write(newPacket.generate())
    
    def dataReceived(self, data):
        packetBuffer = ''.join([self._packetBuffer, data])
        newPacket = self._receivePacket(**self.settings)
        try:
            while 1:
                bytesRead = newPacket.load(packetBuffer)
                self.loaderReceived(newPacket.loader)
                packetBuffer = packetBuffer[bytesRead:]
                if len(packetBuffer) == 0:
                    break
        except OutOfData:
            pass
        self._packetBuffer = packetBuffer
        
    def loaderReceived(self, loader):
        """
        Called upon receiving a packet from the
        other end.
        """
        raise NotImplementedError