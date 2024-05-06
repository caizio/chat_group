#ifndef _MY_CHREAD_H
#define _MY_CHREAD_H

#include <thread>
#include <event.h>

#include "caizi_list.h"
#include "caizi_database.h"

namespace caizi{

typedef bufferevent Bevent;

class Thread{
public:
    Thread();
    ~Thread();
    void start(Info *info, DataBase *db);
    void run();
    std::thread::id get_id();
    struct event_base* get_event_base();

    bool read_data(Bevent* buf_evnt, char *buf);
    void write_Data(Bevent* buf_evnt, Json::Value *data);

    void user_register(Bevent* buf_evnt, Json::Value &data);
    void user_login(Bevent* buf_evnt, Json::Value &data);
    void user_addfriend(Bevent* buf_evnt, Json::Value &data);
    int parse_string(std::string& need_parsed_string, std::string* result);
    void private_chat(Bevent *buf_event, Json::Value &data);
	void create_group(Bevent *buf_event, Json::Value &data);
	void join_group(Bevent *buf_event, Json::Value &data);
	void group_chat(Bevent *buf_event, Json::Value &data);
	void transfer_file(Bevent *buf_event, Json::Value &data);
	void client_offline(Bevent *buf_event, Json::Value &data);
	void get_group_member(Bevent *buf_event, Json::Value &data);

public:
	static void worker(Thread* t);
	static void timeout_cb(evutil_socket_t fd, short event, void *arg);
	static void thread_readcb(Bevent *, void *);
	static void thread_eventcb(Bevent *buf_event, short flag, void *arg);

private:
    std::thread *m_thread;
    std::thread::id m_id;
    struct event_base *m_base;
    Info *m_info;
    DataBase *m_db;
};

}

#endif