# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

from twisted.internet import reactor

from twisted.internet.protocol import ClientCreator

from lacewing.moo.client import MooClientProtocol

class MyProtocol(MooClientProtocol):
    def __init__(self, name, *arg, **kw):
        self.name = name
        MooClientProtocol.__init__(self, *arg, **kw)
    
    def connectionLost(self, reason):
        print 'Disconnected: %s' % reason.value
        reactor.stop()
    
    def motdReceived(self, motd):
        print 'MOTD: %r' % motd
    
    def connectionAccepted(self):
        print 'Connection accepted'
        message = raw_input('Message to server: ')
        self.sendMessage(message, 0)
        channelName = raw_input('Channel name: ')
        self.joinChannel(channelName)
    
    def channelJoined(self, channel):
        print 'Channel %r joined' % channel.name
        message = raw_input('Message to channel: ')
        channel.sendMessage(message, 0)
        newName = raw_input('Change name to: ')
        self.changeName(newName)
    
    def nameChanged(self, channel, oldName, newName):
        print 'Changed name to %r on %r' % (newName, channel.name)
        print 'End of example!'
        
    def channelUserExists(self, channel, player):
        print 'Player %r exists' % player.name

    def channelUserJoined(self, channel, player):
        print 'Player %r joined' % player.name

    def channelUserLeft(self, channel, player):
        print 'Player %r left' % player.name

    def messageReceived(self, message, subchannel):
        print 'Server: %r on subchannel %s' % (message.value, subchannel)

    def channelMessageReceived(self, channel, sender, message, subchannel):
        print '(%s) %s: %r on subchannel %s' % (channel.name, sender.name, message.value, subchannel)

def main():
    host = raw_input('Server IP: ')
    # the initial name to set
    name = raw_input('Username: ')
    # connect the TCP protocol
    ClientCreator(reactor, MyProtocol, name).connectTCP(host, 1203)
    reactor.run()

if __name__ == '__main__':
    main()