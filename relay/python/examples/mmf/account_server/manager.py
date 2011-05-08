# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

import os
import yaml

class Account(object):
    name = None
    passhash = None
    manager = None

    def __init__(self, manager):
        self.manager = manager
    
    def checkPassword(self, passhash):
        """
        Check if two password hashs match
        """
        return passhash.lower() == self.passhash.lower()

    def load(self, name):
        """
        Loads the account with the name 'name'
        """
        self.initialize(**yaml.load(self.manager.openAccount(name, 'rb')))
        self.name = name
        self.manager.items[name] = self
    
    def create(self, name, **data):
        """
        Creates a new account with the name 'name' (and sets the attributes
        of the account using 'data')
        """
        self.name = name
        self.initialize(**data)
        self.manager.items[name] = self
    
    def unload(self):
        """
        Unloads the account from the manager
        """
        del self.manager.items[self.name]
        self.save()
    
    def save(self):
        """
        Saves the account to disk (so it can be removed from memory)
        """
        dataDict = {}
        dataDict['passhash'] = self.passhash
        yaml.dump(dataDict, self.manager.openAccount(self.name, 'wb'))
    
    def initialize(self, **data):
        """
        Initializes the account attributes with the 'data' dict
        """
        for key, value in data.iteritems():
            setattr(self, key, value)

class AccountManager(object):
    directory = None
    items = None
    extension = 'txt'

    def __init__(self, directory = './accounts'):
        self.directory = directory
        self.items = {}
        try:
            os.mkdir(directory)
        except OSError:
            # the directory exists, just ignore
            pass
    
    def openAccount(self, name, mode):
        """
        Open an accounts file with specified mode
        """
        return open(self.accountPath(name), mode)
    
    def createAccount(self, name, **data):
        """
        Creates an account.
        Returns False if an account with that
        name exists.
        """
        if self.accountExists(name):
            return False
        newAccount = Account(self)
        newAccount.create(name, **data)
        return newAccount
    
    def loggedIn(self, name):
        return name in self.items
    
    def getAccount(self, name):
        """
        Get an existing account.
        Returns False if the account does not exist.
        """
        if not self.accountExists(name):
            return False
        if name in self.items:
            return self.items[name]
        newAccount = Account(self)
        newAccount.load(name)
        return newAccount
    
    def accountExists(self, name):
        """
        Returns True if an account with 'name' exists, otherwise False
        """
        return name in self.items or os.path.isfile(self.accountPath(name))
    
    def accountPath(self, name):
        """
        Get the path of an account file
        """
        return os.path.join(self.directory, name + '.' + self.extension)