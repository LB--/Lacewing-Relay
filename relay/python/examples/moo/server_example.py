# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

from twisted.internet import reactor

from lacewing.moo.server import MooServerProtocol, MooServerFactory

class MyProtocol(MooServerProtocol):
    def nameSet(self, name):
        print '%r connected' % name
    
    def nameChanged(self, oldName, newName):
        print '%r changed name to %r' % (oldName, newName)
    
    def messageReceived(self, message, subchannel):
        print '%s: %r on subchannel %s' % (self.name, message.value, subchannel)
        
    def channelJoined(self, channel):
        print '%s joined channel %r' % (self.name, channel.name)
    
    def channelLeft(self, channel):
        print '%s left channel %r' % (self.name, channel.name)

    def channelMessageReceived(self, channel, message, subchannel):
        print '(%s) %s: %r on subchannel %s' % (channel.name, self.name, message.value, subchannel)

class MyFactory(MooServerFactory):
    motd = 'Just testing!'
    protocol = MyProtocol

def main():
    port = reactor.listenTCP(1203, MyFactory())
    print 'Listening on port %s...' % port.port
    reactor.run()

if __name__ == '__main__':
    main()