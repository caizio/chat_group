#include <iostream>
#include <event.h>
#include <event2/listener.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>

#include "caizi_database.h"
#include "caizi_list.h"
#include "caizi_thread.h"

#define IP "192.168.1.1"
#define PORT 8888


namespace caizi{

class Server{

public:
    Server();
    ~Server();
    void listen(const char *, int);
    void updata_group_info();
    static void listen_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int, void *);
    void server_alloc_event(int);

private:
	struct event_base *base;   //用于监听连接事件集合   
	DataBase *db;              //数据库对象
	Info *info;            //数据结构对象
	Thread *pool;          //线程池对象   
	int thread_num;            //线程数量
	int cur_thread;            //当前的线程
};

}

