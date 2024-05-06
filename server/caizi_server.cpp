#include "caizi_server.h"

namespace caizi{

Server::Server(){
    m_base = event_base_new();
    m_db = new DataBase();
    if(!m_db->database_init_table()){
        std::cout << "database init table error" << std::endl;
        exit(1);
    }

    m_info = new Info();

    updata_group_info();

    m_pool = new Thread[THREAD_NUM];
    m_cur_thread = 0;
    for(int i = 0; i < 3 ; i++){
        m_pool[i].start(m_info, m_db);
    }
}
Server::~Server(){
	if (m_db) delete m_db;
	if (m_info) delete  m_info;
}

// 更新群组信息
void Server::updata_group_info(){
    m_db->database_connect();
    std::string group_info[1024];
    int number = m_db->database_get_group_info(group_info);
    m_db->database_disconnect();
    m_info->update_groups(group_info, number);
}

// 监听端口
void Server::listen(const char *ip, int port){
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    struct evconnlistener* listener = evconnlistener_new_bind(m_base, listen_cb, 
        this, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 5, (struct sockaddr*)&addr, sizeof(addr));

    if(NULL == listener){
        std::cout << "evconnlistener_new_bind error" << std::endl;
        exit(1);
    }

    event_base_dispatch(m_base);
    evconnlistener_free(listener);
    event_base_free(m_base);
}


// 监听端口的回调函数，接收到消息，触发此函数
void Server::listen_cb(struct evconnlistener *listerner, evutil_socket_t fd, struct sockaddr *addr, int socklen, void* arg){
    Server* server = (Server*)arg;
    struct sockaddr_in *client_addr = (struct sockaddr_in*)addr;
    server->server_alloc_event(fd);
}

// 服务器接收一个新的连接
void Server::server_alloc_event(int socket_fd){
    struct event_base* t_base = m_pool[m_cur_thread].get_event_base();
    struct bufferevent* bev = bufferevent_socket_new(t_base, socket_fd, BEV_OPT_CLOSE_ON_FREE);
    if(NULL == bev){
        std::cout << "bufferevent_socket_new error" << std::endl;
        exit(1);
    }

   	bufferevent_setcb(bev, Thread::thread_readcb, NULL, Thread::thread_eventcb, &m_pool[m_cur_thread]);
	bufferevent_enable(bev, EV_READ);

    m_cur_thread = (m_cur_thread + 1) % THREAD_NUM;
}
}

