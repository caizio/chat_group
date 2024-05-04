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

    void user_register(Bevent* buf_evnt, Json::Value *data);
    void user_login(Bevent* buf_evnt, Json::Value *data);
    void user_addfriend(Bevent* buf_evnt, Json::Value *data);
    int pares_string(std::string&, std::string *);
    void private_chat(Bevent *, Json::Value &);
	void create_group(Bevent *, Json::Value &);
	void join_group(Bevent *, Json::Value &);
	void group_chat(Bevent *, Json::Value &);
	void transfer_file(Bevent *, Json::Value &);
	void client_offline(Bevent *, Json::Value &);
	void get_group_member(Bevent *, Json::Value &);

public:
	static void worker(Thread *);
	static void timeout_cb(evutil_socket_t fd, short event, void *arg);
	static void thread_readcb(Bevent *, void *);
	static void thread_eventcb(Bevent *, short, void *);

private:
    std::thread *m_thread;
    std::thread::id m_id;
    struct event_base *m_base;
    Info *m_info;
    DataBase *m_db;
};

}

#endif