clang++ -std=c++11 -shared -o LwRelay.dll -Wl,--out-implib,libLwRelay.lib RelayServer.cc RelayClient.cc
@echo.
clang++ -std=c++11 -o example-relay-server.exe example-relay-server.cc
@echo.
clang++ -std=c++11 -o example-relay-client.exe example-relay-client.cc
@echo.
@pause
