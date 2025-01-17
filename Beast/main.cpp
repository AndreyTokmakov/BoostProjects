/**============================================================================
Name        : Asio.cpp
Created on  : 16.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Boost Beast modules tests
============================================================================**/

#include <iostream>
#include <vector>

#include "Client.h"
#include "HTTPServer.h"
#include "HTTPS_Server.h"
#include "WebSocketServers.h"
#include "WebSocketClients.h"


int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    // Client::TestAll();

    // HTTPServer::TestAll();
    HTTPS_Server::TestAll();

    // WebSocketServers::TestAll();
    // WebSocketClients::TestAll();

    return EXIT_SUCCESS;
}
