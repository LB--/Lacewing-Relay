# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Minimal server
"""

PORT = 6124

from twisted.internet import reactor

from lacewing.server import ServerProtocol, ServerDatagram, ServerFactory

class MinimalProtocol(ServerProtocol):
    pass

class MinimalFactory(ServerFactory):
    protocol = MinimalProtocol

newFactory = MinimalFactory()
# connect the main TCP factory
port = reactor.listenTCP(PORT, newFactory)
# connect the UDP factory
reactor.listenUDP(PORT, ServerDatagram(newFactory))
# just so we know it's working
print 'Opening new server on port %s...' % PORT
reactor.run()