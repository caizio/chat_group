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
#define THREAD_NUM 1

namespace caizi{

class Server{

public:
    Server();
    ~Server();
    void updata_group_info();
    void listen(const char* ip, int port);
    static void listen_cb(struct evconnlistener *listerner, evutil_socket_t fd, struct sockaddr *addr, int socklen, void* arg);
    void server_alloc_event(int);

private:
	struct event_base *m_base;   //用于监听连接事件集合   
	DataBase *m_db;              //数据库对象
	Info *m_info;                //数据结构对象
	Thread *m_pool;              //线程池对象   
	int m_thread_num;            //线程数量
	int m_cur_thread;            //当前的线程
};

}

