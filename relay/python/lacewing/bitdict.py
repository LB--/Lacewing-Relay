# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

class BitDict(object):
    keys = None
    flags = 0

    def __init__(self, *arg):
        if arg[0] is False:
            return
        self.keys = dict([(item, index) for (index, item) in enumerate(arg)])
    
    def __getitem__(self, key):
        return self.flags & (2 ** self.keys[key]) != 0
    
    def __setitem__(self, key, value):
        if value:
            self.flags |= 2**self.keys[key]
        else:
            self.flags &= ~2**self.keys[key]
    
    def iteritems(self):
        for key, index in self.keys.iteritems():
            yield key, self[key]
    
    def setFlags(self, flags):
        self.flags = flags
    
    def getFlags(self):
        return self.flags
    
    def copy(self):
        newDict = BitDict(False)
        newDict.keys = self.keys
        return newDict
    
    def __str__(self):
        actual_dict = {}
        for key in self.keys:
            actual_dict[key] = self[key]
        return '%s' % actual_dict
    
    def __repr__(self):
        return str(self)