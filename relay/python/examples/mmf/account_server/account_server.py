# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Server that manages registration and checks of accounts

Requires PyYAML
"""
    
import hashlib
from twisted.internet import reactor
from lacewing.server import ServerProtocol, ServerDatagram, ServerFactory
from lacewing.constants import SET_NAME
from manager import AccountManager

# client-to-server
REGISTRATION_SUBCHANNEL = 0
LOGIN_SUBCHANNEL = 1

# server-to-client
ERROR_SUBCHANNEL = 0

# error codes
SUCCESS = 0
ERROR = 1

DELIMITER = '|'

class AccountProtocol(ServerProtocol):
    account = None
    def messageReceived(self, message):
        # split the message by a delimiter
        value = message.value.split(DELIMITER)
        # client wants to register
        if message.subchannel == REGISTRATION_SUBCHANNEL:
            name, password = value # get the specified name and password
            # create the MD5 hash of the password
            passhash = hashlib.md5(password).hexdigest()
            newAccount = self.factory.accounts.createAccount(
                name, passhash = passhash)
            # account exists
            if newAccount == False:
                self.sendResponse(ERROR, 'Account name already exists')
                return
            self.sendResponse(SUCCESS, 'You account has been registered')
            # just unload the account from memory
            newAccount.unload()
        elif message.subchannel == LOGIN_SUBCHANNEL:
            # already logged in
            if self.account:
                self.sendResponse(ERROR, 'You have already logged in')
                return
            name, passhash = value
            # check if account is already logged in
            if self.factory.accounts.loggedIn(name):
                self.sendResponse(ERROR, 'Account already logged in')
                return
            newAccount = self.factory.accounts.getAccount(name)
            # account doesn't exist
            if newAccount == False:
                self.sendResponse(ERROR, 'Account name does not exist')
                return
            if not newAccount.checkPassword(passhash):
                newAccount.unload()
                self.sendResponse(ERROR, 'Invalid password specified')
                return
            self.account = newAccount
            self.sendResponse(SUCCESS, 'You have been logged in')
        else:
            # for some reason, the client sent us something on
            # an unknown subchannel (that we can't handle)
            self.log('Client sent unexpected data')
            self.disconnect()
    
    def sendResponse(self, code, details = ''):
        message = DELIMITER.join([str(code), details])
        self.sendMessage(message, ERROR_SUBCHANNEL)
    
    def acceptLogin(self, name):
        # client can't set it's name
        self.warn(SET_NAME, 'Not allowed')
        return False
    
    def connectionLost(self, reason):
        ServerProtocol.connectionLost(self, reason)
        # if we're logged in, unload the account
        if self.account:
            self.account.unload()
            
    def log(self, message):
        """
        Log a message.
        """
        print '%s: %s' % (self.id, message)
        
class AccountFactory(ServerFactory):
    protocol = AccountProtocol
    def __init__(self):
        self.accounts = AccountManager('./accounts/')
    
newFactory = AccountFactory()
# connect the main TCP factory
port = reactor.listenTCP(6121, newFactory)
reactor.listenUDP(6121, ServerDatagram(newFactory))
# just so we know it's working
print 'Opening new server on port %s...' % port.port
reactor.run()