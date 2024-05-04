#include "caizi_thread.h"
#include <cstring>

namespace caizi{

Thread::Thread(){
    m_thread = new std::thread(worker, this);
    m_id = m_thread->get_id();
    m_base = event_base_new();
}
Thread::~Thread(){
    delete m_thread;
    event_base_free(m_base);
}

void Thread::start(Info *info, DataBase *db){
    m_info = info;
    m_db = db;
}
void Thread::run(){
    struct event timeout;
    struct timeval tv;
}
std::thread::id Thread::get_id(){
    return m_id;
}
struct event_base* Thread::get_event_base(){
    return m_base;
}

// 读取数据，bufevent前四个字节存储的是数据的大小
bool Thread::read_data(Bevent* buf_event, char *result){
    uint32_t size = 0;
    size_t count = 0;
    if(bufferevent_read(buf_event, &size, 4) != 4) {
        return false;
    }

    char buff[1024] = {0};
    while(1){
        count += bufferevent_read(buf_event, buff, 1024);
        strcat(result, buff);
        memset(buff, 0, 1024);
        if(count >= size){
            break;
        }
    }
    return true;
}

void Thread::write_Data(Bevent* buf_evnt, Json::Value *data){
    Json::FastWriter writer;
    std::string str = writer.write(*data);

    int len = str.size();
    char buff[1024] = {0};
    memcpy(buff, &len, 4);
    memcpy(buff + 4, str.c_str(), len);
    if(bufferevent_write(buf_evnt, buff, len + 4) == -1){
        std::cout << "error: Thread::write_Data" << std::endl;
    }
}

void Thread::user_register(Bevent* buf_evnt, Json::Value *data){
    m_db->database_connect();


}
void Thread::user_login(Bevent* buf_evnt, Json::Value *data){}
void Thread::user_addfriend(Bevent* buf_evnt, Json::Value *data){}
int Thread::pares_string(std::string&, std::string *){}
void Thread::private_chat(Bevent *, Json::Value &){}
void Thread::create_group(Bevent *, Json::Value &){}
void Thread::join_group(Bevent *, Json::Value &){}
void Thread::group_chat(Bevent *, Json::Value &){}
void Thread::transfer_file(Bevent *, Json::Value &){}
void Thread::client_offline(Bevent *, Json::Value &){}
void Thread::get_group_member(Bevent *, Json::Value &){}

void Thread::worker(Thread *t){
    t->run();
}
void Thread::timeout_cb(evutil_socket_t fd, short event, void *arg){

}
void Thread::thread_readcb(Bevent *, void *){

}
void Thread::thread_eventcb(Bevent *, short, void *){

}

}