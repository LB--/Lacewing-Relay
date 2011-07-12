LOCAL_PATH := $(call my-dir)
  
include $(CLEAR_VARS)
  
LOCAL_LDLIBS := -llog -lssl -lcrypto

LOCAL_CPP_EXTENSION := cc
LOCAL_MODULE        := lacewing
LOCAL_SRC_FILES     := \
    ../src/Global.cc \
    ../src/Sync.cc \
    ../src/SpinSync.cc \
    ../src/Filter.cc \
    ../src/Address.cc \
    ../src/Error.cc \
    ../src/RelayServer.cc \
    ../src/webserver/Webserver.cc \
    ../src/webserver/Webserver.Incoming.cc \
    ../src/webserver/Webserver.Outgoing.cc \
    ../src/webserver/Webserver.Sessions.cc \
    ../src/webserver/Webserver.Multipart.cc \
    ../src/webserver/Webserver.MimeTypes.cc \
    ../src/unix/Event.cc \
    ../src/unix/EventPump.cc \
    ../src/unix/Server.cc \
    ../src/unix/Timer.cc \
    ../src/unix/UDP.cc \
    ../src/unix/Client.cc \
    ../src/unix/Pump.cc \
    ../src/c/addr_flat.cc \
    ../src/c/eventpump_flat.cc \
    ../src/c/global_flat.cc \
    ../src/c/server_flat.cc \
    ../src/c/timer_flat.cc \
    ../src/c/webserver_flat.cc \
    ../src/c/filter_flat.cc \
    ../src/c/udp_flat.cc \
    ../src/c/client_flat.cc \
    ../src/c/error_flat.cc \
    ../src/c/sync_flat.cc \
    ../src/c/ssync_flat.cc

LOCAL_CFLAGS        := -DLacewingAndroid -fno-exceptions -fno-rtti

include $(BUILD_SHARED_LIBRARY)
