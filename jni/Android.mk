LOCAL_PATH := $(call my-dir)
  
include $(CLEAR_VARS)

LOCAL_LDLIBS := -llog

LOCAL_CPP_EXTENSION := .cc
LOCAL_MODULE        := lacewing
LOCAL_SRC_FILES     := \
	../src/global.c \
	../src/nvhash.c \
	../src/filter.c \
	../src/address.c \
	../src/streamgraph.c \
	../src/stream.c \
	../src/error.c \
	../src/webserver/webserver.c \
	../src/webserver/upload.c \
	../src/webserver/sessions.c \
	../src/webserver/mimetypes.c \
	../src/webserver/request.c \
	../src/webserver/multipart.c \
	../src/webserver/http/http-client.c \
	../src/webserver/http/http-parse.c \
	../deps/http-parser/http_parser.c \
	../deps/multipart-parser/multipart_parser.c \
	../src/pipe.c \
	../src/flashpolicy.c \
	../src/pump.c \
	../src/util.c \
	../src/list.c \
	../src/heapbuffer.c \
	../src/cxx/address.cc \
	../src/cxx/client.cc \
	../src/cxx/error.cc \
	../src/cxx/event.cc \
	../src/cxx/eventpump.cc \
	../src/cxx/fdstream.cc \
	../src/cxx/file.cc \
	../src/cxx/filter.cc \
	../src/cxx/flashpolicy.cc \
	../src/cxx/pipe.cc \
	../src/cxx/pump.cc \
	../src/cxx/server.cc \
	../src/cxx/stream.cc \
	../src/cxx/sync.cc \
	../src/cxx/thread.cc \
	../src/cxx/timer.cc \
	../src/cxx/udp.cc \
	../src/unix/client.c \
	../src/unix/event.c \
	../src/unix/eventpump.c \
	../src/unix/server.c \
	../src/unix/thread.c \
	../src/unix/timer.c \
	../src/unix/udp.c \
	../src/unix/fdstream.c \
	../src/unix/file.c \
	../src/unix/sync.c \
	../src/unix/global.c

LOCAL_CFLAGS := -std=gnu99 -D_lacewing_library -D_lacewing_no_ssl -D_lacewing_no_spdy -fvisibility=hidden -DHTTP_PARSER_STRICT=1 -DHTTP_PARSER_DEBUG=0
LOCAL_CXXFLAGS := -fno-exceptions -fno-rtti

include $(BUILD_SHARED_LIBRARY)

