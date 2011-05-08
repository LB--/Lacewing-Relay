# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Server that shares information about other servers
"""

from twisted.internet import reactor

from lacewing.server import ServerProtocol, ServerDatagram, ServerFactory
from lacewing.bytereader import ByteReader
from lacewing.constants import SET_NAME

# client-to-server
SET_SERVER_SUBCHANNEL = 0

# server-to-client
SERVER_LIST_SUBCHANNEL = 0

DELIMITER = '|'

class MasterProtocol(ServerProtocol):
    server = False
    def connectionAccepted(self, welcome):
        # send the client the current server list
        self.sendServers(self.factory.getServers())
    
    def messageReceived(self, message):
        # if message contains something, kick the client
        if message.value:
            self.disconnect('UnexpectedPacket')
        # client wants to set itself as server
        elif message.subchannel == SET_SERVER_SUBCHANNEL:
            if not self.server:
                self.server = True
                self.factory.update()
                self.log('Added server IP %r' % self.transport.getPeer().host)
        # unknown packet
        else:
            self.disconnect('UnexpectedPacket')
    
    def connectionLost(self, reason):
        ServerProtocol.connectionLost(self, reason)
        # update server list if this client is a server
        if self.server:
            self.server = False
            self.factory.update()
            self.log('Removed server IP %r' % self.transport.getPeer().host)
        
    def sendServers(self, item):
        self.sendMessage(DELIMITER.join(item), SERVER_LIST_SUBCHANNEL)
    
    def acceptLogin(self, name):
        # client can't set it's name
        self.warn(SET_NAME, 'Not allowed')
        return False
            
    def log(self, message):
        """
        Log a message.
        """
        print '%s: %s' % (self.id, message)
        
class MasterFactory(ServerFactory):
    protocol = MasterProtocol
    
    def update(self):
        ipList = self.getServers()
        for connection in self.connections.values():
            connection.sendServers(ipList)
    
    def getServers(self):
        ipList = []
        for connection in self.connections.values():
            if not connection.server:
                continue
            ipList.append(connection.transport.getPeer().host)
        return ipList
    
newFactory = MasterFactory()
port = reactor.listenTCP(6121, newFactory)
reactor.listenUDP(6121, ServerDatagram(newFactory))
print 'Opening new server on port %s...' % port.port
reactor.run()