# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Simple example on how to write a client with UDP features enabled
"""

from twisted.internet import reactor, protocol

from lacewing.client import ClientProtocol

def getProtocolType(settings):
    if settings.get('datagram', False):
        return 'UDP'
    else:
        return 'TCP'

class MyClient(ClientProtocol):
    def __init__(self, name):
        self.name = name

    def connectionAccepted(self):
        print 'Connection accepted!'
        self.setName(self.name)
    
    def connectionLost(self, reason):
        print 'Disconnected:', reason
        
    def serverGreeted(self, welcome):
        print 'MOTD: %s' % welcome
    
    def messageReceived(self, message):
        protocolType = getProtocolType(message.settings)
        print '(%s)(Server on %s) %s' % (protocolType, message.subchannel, 
            repr(message.value))
        
    def channelMessageReceived(self, channel, sender, message):
        protocolType = getProtocolType(message.settings)
        print '(%s)(%s)(%s on %s) %s' % (protocolType, channel.name, 
            sender.name, message.subchannel, repr(message.value))

    def privateMessageReceived(self, channel, sender, message):
        protocolType = getProtocolType(message.settings)
        print '(%s)(%s)(Private from %s on %s) %r' % (protocolType, 
            channel.name, sender.name, message.subchannel, message.value)

    def loginAccepted(self, name):
        print 'Logged in, %s' % name
        self.requestChannelList().addCallback(self.receivedChannelList)
        
    def receivedChannelList(self, channelList):
        print '* ChannelList *'
        for name, count in channelList:
            print '- %s (%s users)' % (name, count)
        print '* End of ChannelList *'
        channel = raw_input('Channel: ')
        self.joinChannel(channel)
        
    def channelJoined(self, channel):
        print 'Signed on!'
        self.sendMessages()
        
    def channelLeft(self, channel):
        print 'Signed off!'
        print 'End of example'
        
    def nameChanged(self, name):
        print 'Changed name to %s!' % name
        if len(self.channels):
            reactor.callLater(4, self.leaveChannel, self.channels.values()[0])
        
    def sendMessages(self):
        """
        This will ask the user to type in some messages that will
        be sent respectively as a direct message (client -> server)
        and channel message (client -> channel clients).
        """
        # get the first channel in the channel list
        channel = self.channels.values()[0]
        message = raw_input('Message: ')
        # 'asDatagram' sends the message with UDP
        self.sendMessage(message, 0, asDatagram = True)
        print '(sending to Server)'
        message = raw_input('Channel Message: ')
        channel.sendMessage(message, 1, asDatagram = True)
        print '(sending to "%s")' % channel.name
        name = raw_input('New Username: ')
        self.setName(name)

    def serverDenied(self, response):
        print 'Denied, reason:', response.value
        
    def channelUserJoined(self, channel, client):
        client.sendMessage('Hello!', 0)
        print '"%s" (%s) joined %s' % (client.name, client.id, channel.name) 
        
    def channelUserExists(self, channel, client):
        client.sendMessage('Hi!', 0)
        if client.master:
            print '"%s" (%s, channel master) is on %s' % (client.name, 
                client.id, channel.name)
        else:
            print '"%s" (%s) is on %s' % (client.name, client.id, channel.name)
        
    def channelUserLeft(self, channel, client):
        print '"%s" (%s) left %s' % (client.name, client.id, channel.name)
        
    def channelUserChanged(self, channel, client):
        print '%s changed name to "%s" on %s' % (client.id, client.name, channel.name)

# the host to connect to
host = raw_input('Host: ')
port = int(raw_input('Port: '))
name = raw_input('Name: ')
creator = protocol.ClientCreator(reactor, MyClient, name).connectTCP(host, port)
reactor.run()