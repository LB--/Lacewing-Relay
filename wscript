
# This is ONLY for building the Node module.  To build liblacewing, just use the regular Makefile.

from os import *

def set_options(opt):
    opt.tool_options("compiler_cxx")

def configure(conf):
    conf.check_tool("compiler_cxx")
    conf.check_tool("node_addon")
    system("./configure")

def build(bld):
    obj = bld.new_task_gen("cxx", "shlib", "node_addon")
    obj.target = "liblacewing"
    obj.cxxflags = ["-DHAVE_CONFIG_H"]
    obj.source = "js/v8/LacewingNode.cc js/v8/LacewingV8.cc ev/LacewingLEv.cc \
                    src/Global.cc \
                    src/Sync.cc \
                    src/SpinSync.cc \
                    src/Filter.cc \
                    src/Address.cc \
                    src/Error.cc \
                    src/RelayServer.cc \
                    src/webserver/Webserver.cc \
                    src/webserver/Webserver.Incoming.cc \
                    src/webserver/Webserver.Outgoing.cc \
                    src/webserver/Webserver.Sessions.cc \
                    src/webserver/Webserver.Multipart.cc \
                    src/webserver/Webserver.MimeTypes.cc \
                    src/unix/Event.cc \
                    src/unix/EventPump.cc \
                    src/unix/Server.cc \
                    src/unix/Timer.cc \
                    src/unix/UDP.cc \
                    src/unix/Client.cc \
                    src/unix/Pump.cc"
                    
    
