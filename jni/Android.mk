LOCAL_PATH := $(call my-dir)
  
include $(CLEAR_VARS)

LOCAL_LDLIBS := -llog -lssl -lcrypto

LOCAL_CPP_EXTENSION := cc
LOCAL_MODULE        := lacewing
LOCAL_SRC_FILES     := \
    ../src/Global.cc \
    ../src/Sync.cc \
    ../src/Filter.cc \
    ../src/Address.cc \
    ../src/StreamGraph.cc \
    ../src/Stream.cc \
    ../src/Pipe.cc \
    ../src/Error.cc \
    ../src/FlashPolicy.cc \
	../src/Pump.cc \
    ../src/webserver/Webserver.cc \
    ../src/webserver/Sessions.cc \
    ../src/webserver/Request.cc \
    ../src/webserver/MimeTypes.cc \
    ../src/webserver/http/HTTP.cc \
    ../src/webserver/http/Multipart.cc \
    ../deps/http-parser/http_parser.c \
    ../src/unix/Event.cc \
    ../src/unix/EventPump.cc \
    ../src/unix/Server.cc \
    ../src/unix/Timer.cc \
    ../src/unix/UDP.cc \
    ../src/unix/Client.cc \
    ../src/unix/Thread.cc \
    ../src/unix/File.cc \
    ../src/unix/FDStream.cc \
    ../src/c/addr_flat.cc \
    ../src/c/pump_flat.cc \
    ../src/c/stream_flat.cc \
    ../src/c/pipe_flat.cc \
    ../src/c/file_flat.cc \
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
    ../src/c/ssync_flat.cc \
    ../src/c/thread_flat.cc \
	../src/c/flashpolicy_flat.cc

LOCAL_CFLAGS := -DLacewingLibrary -fno-exceptions -fno-rtti -fvisibility=hidden -DHTTP_PARSER_STRICT=0 -DHTTP_PARSER_DEBUG=0 

include $(BUILD_SHARED_LIBRARY)

