#ifndef _CAI_LSIT_H
#define _CAI_LSIT_H

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <mutex>
#include <event.h>

namespace caizi{

struct User{
    std::string name;
    struct bufferevent *bev;    // 用于管理网络连接的读写缓冲区, 记录与用户的连接信息，接受或发送
};

// 用户和群组对象
class Info{
public:
    Info();
    ~Info();

    struct bufferevent* get_user_buffevent(std::string user_name);
    bool update_users(std::string user_name, struct bufferevent *bev);
    void delete_user(std::string user_name);
    bool user_is_in_group(std::string user_name,std::string group_name);

    bool group_is_exist(std::string);
    void get_group_member(std::string group_name, std::string &result);
    std::list<std::string> &get_group_members(std::string group_name);
    void add_new_group(std::string group_name, std::string user);
    void update_groups(std::string *,int size);
    void print_groups();
    void update_groups_member(std::string group, std::string user);
private:
    std::list<User> *m_users;                                   // 在线用户
    std::map<std::string, std::list<std::string>> *m_groups;    // 群信息
    std::mutex m_user_mutex;
    std::mutex m_group_mutex;
};
   
}
#endif