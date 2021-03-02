//
// Created by cyh on 2021/2/26.
//

#include "Epoll/MultiProcessServer.h"

using namespace std;

int main() {

    MultiProcessServer server;
    if (server.InitServer("127.0.0.1", 8888)) {
        server.Run();
    }

    return 0;
}
