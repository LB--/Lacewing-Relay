# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Server that reselects the master client
"""

from twisted.internet import reactor

from lacewing.server import ServerProtocol, ServerFactory, ServerDatagram
from lacewing.bytereader import ByteReader

# server-to-client
SELECT_MASTER = 0

class ReselectProtocol(ServerProtocol):
    def channelJoined(self, channel):
        self.sendMessage(channel.master.id, SELECT_MASTER)
    
    def channelLeft(self, channel):
        if channel.master is self and channel.connections:
            channel.master = channel.connections.values()[0]
            for connection in channel.connections.values():
                connection.sendMessage(channel.master.id, SELECT_MASTER)
    
    def connectionLost(self, reason):
        channels = self.channels.values()
        ServerProtocol.connectionLost(self, reason)
        for channel in channels:
            self.channelLeft(channel)
        
class ReselectFactory(ServerFactory):
    protocol = ReselectProtocol
    
newFactory = ReselectFactory()
# connect the main TCP factory
port = reactor.listenTCP(6121, newFactory)
reactor.listenUDP(6121, ServerDatagram(newFactory))
print 'Opening new server on port %s...' % port.port
reactor.run()