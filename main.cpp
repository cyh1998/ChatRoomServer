#include "Epoll/Server.h"

using namespace std;

int main() {

    Server server;
    if (server.InitServer("127.0.0.1", 8888)) {
        server.Run();
    }

    return 0;
}
