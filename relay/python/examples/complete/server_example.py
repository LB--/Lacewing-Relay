"""
Server with basic logging (with UDP)
"""

# try and run psyco for performance
try:
    import psyco
    psyco.full()
except ImportError:
    pass # just leave it

from twisted.internet import reactor

from lacewing.server import ServerProtocol, ServerDatagram, ServerFactory

# most in here is self-explanatory

def getProtocolType(settings):
    if settings['Datagram']:
        return 'UDP'
    else:
        return 'TCP'

class MyServer(ServerProtocol):
    def connectionAccepted(self, welcome):
        self.log('Client connection accepted.')

    def messageReceived(self, message):
        protocolType = getProtocolType(message.settings)
        self.log('(%s) %s %r' % (protocolType, message.subchannel, message.value))
        
    def channelMessageReceived(self, channel, message):
        protocolType = getProtocolType(message.settings)
        self.log('(%s)(%s) %s %r' % (protocolType, channel.name, message.subchannel, message.value))

    def privateMessageReceived(self, channel, recipient, message):
        protocolType = getProtocolType(message.settings)
        self.log('(%s)(to %s) %s %r' % (protocolType, recipient.name, message.subchannel, message.value))

    def loginAccepted(self, name):
        self.log('Name set to "%s"' % name)
        
    def channelListSent(self):
        self.log('(sent channel list)')
        
    def channelJoined(self, channel):
        self.log('Signed on to channel "%s"' % channel.name)
        
    def channelLeft(self, channel):
        self.log('Left channel "%s"' % channel.name)
        
    def nameChanged(self, name):
        self.log('Name changed to %s' % name)
        
    def connectionLost(self, reason):
        # here, we need to call OServer's connectionLost
        # because connectionLost is a twisted method, and the server
        # needs to know that the client has disconnected.
        ServerProtocol.connectionLost(self, reason)
        if self.loggedIn:
            self.log('Connection disconnected.')
    
    def disconnect(self, reason):
        print self.log('Kicked: %s' % reason)          
        ServerProtocol.disconnect(self, reason)
            
    def log(self, message):
        """
        Log a message.
        """
        print '%s: %s' % (self.id, message)
        
class MyFactory(ServerFactory):
    protocol = MyServer
    ping = True
    channelListing = True
    masterRights = True # allows the master to kick people
    welcomeMessage = 'Example pylacewing server'
    
newFactory = MyFactory()
# connect the main TCP factory
port = reactor.listenTCP(6121, newFactory)
reactor.listenUDP(6121, ServerDatagram(newFactory))
# just so we know it's working
print 'Opening new server on port %s...' % port.port
reactor.run()