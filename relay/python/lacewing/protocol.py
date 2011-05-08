# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
The base for server and client.
"""

from twisted.internet import protocol
from lacewing.multidict import MultikeyDict
from lacewing.exceptions import OutOfData

class BaseProtocol(protocol.Protocol):
    """
    Baseclass that contains properties and methods common to
    both server and client protocols.

    @ivar name: The name of the client.
    @ivar id: The ID of the client.
    @ivar privateAllowed: False if the client has requested no
    private messages must be sent to it.
    @ivar isAccepted: True if the client connection has been
    accepted.
    @ivar loggedIn: True if the client is logged in with a name.
    @ivar channels: Channels the client is signed on to
    (has keys for both the ID and name).
    @type channels: MultikeyDict
    """
    revision = 'revision 2'

    name = None
    id = None
    isAccepted = False
    loggedIn = False
    udpEnabled = False
    
    channels = None
    
    # for packet parsing
    
    _packetBuffer = ''
    
    def connectionMade(self):
        self.channels = MultikeyDict()

    def dataReceived(self, data):
        packetBuffer = ''.join([self._packetBuffer, data])
        newPacket = self._receivePacket()
        try:
            while 1:
                bytesRead = newPacket.read(packetBuffer)
                self.loaderReceived(newPacket.loader)
                packetBuffer = packetBuffer[bytesRead:]
                if len(packetBuffer) == 0:
                    break
        except OutOfData:
            pass
        self._packetBuffer = packetBuffer
    
    def sendLoader(self, loader, asDatagram = False, **settings):
        raise NotImplementedError('sendLoader() not implemented')
    
    def loaderReceived(self, loader, isDatagram = False):
        """
        Called when a packet is received.

        @param isDatagram: True if the message recieved was sent
        using UDP.
        """
        raise NotImplementedError('loaderReceived() method not implemented')

    def validName(self, name):
        """
        Returns True if name is a valid Lacewing
        channel or client name.
        """
        if not name.strip() == '':
            return True
        return False