
%rename(error) _error;
%rename(event) _event;
%rename(pump) _pump;
%rename(eventpump) _eventpump;
%rename(thread) _thread;
%rename(timer) _timer;
%rename(sync) _sync;
%rename(sync_lock) _sync_lock;
%rename(stream) _stream;
%rename(pipe) _pipe;
%rename(fdstream) _fdstream;
%rename(file) _file;
%rename(address) _address;
%rename(filter) _filter;
%rename(client) _client;
%rename(server) _server;
%rename(server_client) _server_client;
%rename(udp) _udp;
%rename(webserver) _webserver;
%rename(webserver_request) _webserver_request;
%rename(webserver_request_header) _webserver_request_header;
%rename(webserver_request_cookie) _webserver_request_cookie;
%rename(webserver_sessionitem) _webserver_sessionitem;
%rename(webserver_request_param) _webserver_request_param;
%rename(webserver_upload) _webserver_upload;
%rename(webserver_upload_header) _webserver_upload_header;
%rename(flashpolicy) _flashpolicy;

%module lacewing
%{
#include "../include/lacewing.h"
%}

%include "../include/lacewing.h"

