#include <iostream>
#include "server/caizi_server.h"

int main(){
    caizi::Server server;
    server.listen(IP, PORT);

    return 0;
}