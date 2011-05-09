# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Example Flash policy server.
Useful for compatibility with Flash Lacewing clients.
"""

from twisted.internet import protocol, reactor
from twisted.protocols import basic

class FlashPolicyProtocol(basic.LineReceiver):
    delimiter = '\x00'
    MAX_LENGTH = 64

    def lineReceived(self, request):
        if request != '<policy-file-request/>':
            self.transport.loseConnection()
            return
        self.transport.write(self.factory.policyData)

class FlashPolicyFactory(protocol.ServerFactory):
    protocol = FlashPolicyProtocol
    policyData = None

    def __init__(self):
        self.policyData = open('policy.xml', 'rb').read() + '\x00'

if __name__ == '__main__':
    reactor.listenTCP(843, FlashPolicyFactory())
    print 'Starting Flash Policy server on port 843...'
    reactor.run()