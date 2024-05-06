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

    event_assign(&timeout, m_base, -1, EV_PERSIST, timeout_cb, this);

    // 计时器重置并设置为3
	evutil_timerclear(&tv);
	tv.tv_sec = 3;
	event_add(&timeout, &tv);

	std::cout << "thread：" << m_id << " start working ..." << std::endl;

	event_base_dispatch(m_base);
	event_base_free(m_base);
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

void Thread::user_register(Bevent* buf_evnt, Json::Value& data){
    m_db->database_connect();
    if(m_db->database_user_is_exist(data["username"].asString()) ){
        Json::Value val;
		val["cmd"] = "register_reply";
		val["result"] = "user_exist";
        write_Data(buf_evnt, &val);
    }else{
        m_db->database_insert_user_info(data);
        Json::Value val;
        val["cmd"] = "register_reply";
		val["result"] = "success";
        write_Data(buf_evnt, &val);
    }
    m_db->database_close();

}

// @brief 用户登录
void Thread::user_login(Bevent* buf_evnt, Json::Value& data){
    Bevent* bv = m_info->user_is_in_m_users(data["username"].asString());
    if(bv){
        // 用户已经在线
        Json::Value val;
        val["cmd"] = "login_reply";
        val["result"] = "already_login";
        write_Data(buf_evnt, &val);
        return;
    }

    m_db->database_connect();
    if(!m_db->database_user_is_exist(data["username"].asString())){
        // 用户不存在
        Json::Value val;
        val["cmd"] = "login_reply";
        val["result"] = "user_not_exist";
        write_Data(buf_evnt, &val);
        m_db->database_disconnect();
        return;
    }
       
    // 用户存在，但密码错误
    if(m_db->database_password_correct(data)){
        Json::Value val;
        val["cmd"] = "login_reply";
        val["result"] = "password_error";
        write_Data(buf_evnt, &val);
        m_db->database_disconnect();
        return;
    }

    // 用户存在，密码正确
    std::string friendlist, grouplist;
    if(!m_db->database_get_user_friend_and_group(data, friendlist, grouplist)){
        std::cout << "error: database_get_friend_group" << std::endl;
        m_db->database_disconnect();
        return;
    }

    m_db->database_disconnect();

    Json::Value val;
    val["cmd"] = "login_reply";
    val["result"] = "success";
    val["firendlist"] = friendlist;
    val["grouplist"] = grouplist;

    write_Data(buf_evnt, &val);
    m_info->update_users(data, buf_evnt);
    
    // 更新在线用户的朋友
    if(friendlist.empty()){
        return;
    }

    // 遍历在线用户的朋友，发送在线的消息
    int index = friendlist.find('|');
    int start = 0;
    while(index != -1){
        std::string name = friendlist.substr(start, index - start);
        Bevent* bv_f = m_info->user_is_in_m_users(name);
        if(NULL != bv_f){
            Json::Value val;
            val["cmd"] = "online";
            val["username"] = data["username"];
            write_Data(bv_f, &val);
        }
        start = index + 1;
        index = friendlist.find('|', start);
    }
    // 处理最后一个朋友
	std::string name = friendlist.substr(start, index - start);
	Bevent* bv_f = m_info->user_is_in_m_users(name);
	if(NULL != bv_f){
		val["cmd"] = "online";
		val["username"] = data["username"];
		write_Data(bv_f, &val);
	}
}

// @brief 添加好友
void Thread::user_addfriend(Bevent* buf_evnt, Json::Value& data){
    if(data["firend"] == data["username"]){
        return;
    }

    m_db->database_connect();
    if(!m_db->database_user_is_exist(data["firend"].asString())){
        Json::Value val;
        val["cmd"] = "addfriend_reply";
        val["result"] = "friend_not_exist";
        write_Data(buf_evnt, &val);
        m_db->database_disconnect();
        return;
    }

    // 已经是好友，则不用添加并返回
    std::string friendlist, grouplist;
    if(!m_db->database_get_user_friend_and_group(data, friendlist, grouplist)){
        std::string str[1024];
        int count = parse_string(friendlist, str);
        for(int i = 0; i < count; i++){
            if(str[i] == data["friend"].asString()){
                Json::Value val;
                    val["cmd"] = "addfriend_reply";
                    val["result"] = "already_friend";
                    write_Data(buf_evnt, &val);
                    m_db->database_disconnect();
                    return;
            }
        }
    }

    // 添加好友
    m_db->database_add_friend(data);
    m_db->database_disconnect();

    Json::Value val;
    val["cmd"] = "addfriend_reply";
    val["result"] = "success";
    
    // 发送添加好友的消息
    Bevent *b = m_info->user_is_in_m_users(data["firend"].asString());
    if(NULL != b){
        write_Data(b, &val);
    }
    val.clear();
    val["cmd"] = "addfriend_reply";
    val["result"] = "suceess";
    val["friend"] = data["friend"];
    write_Data(buf_evnt, &val);
}

/*
    @brief 将字符串（按照|分割）解析成字符串数组
    @param[in] need_parsed_string 含有|的字符串
    @param[out] result 解析后的字符串数组
    @param[out] count 解析后的字符串数组的长度
*/ 
int Thread::parse_string(std::string& need_parsed_string, std::string* result){
    int count = 0;
    int start = 0;
    int index = need_parsed_string.find("|");
    while(index != -1){
        result[count++] = need_parsed_string.substr(start, index - start);
        start = index + 1;
        index = need_parsed_string.find("|", start);
    }

    result[count++] = need_parsed_string.substr(start);
    return count;
}

// 发送消息 
void Thread::private_chat(Bevent *buf_event, Json::Value &data){
    std::string name = data["tofriedn"].asString();
    Bevent *friend_b = m_info->user_is_in_m_users(name);
    if(NULL == friend_b){
        Json::Value val;
        val["cmd"] = "private_reply";
        val["result"] = "offline";
        write_Data(buf_event, &val);
    }

    Json::Value val;
    val["cmd"] = "private_reply";
    val["fromfriend"] = data["username"].asString();
    val["text"] = data["text"];
    write_Data(friend_b, &val);
}

// 创建一个新的群组
void Thread::create_group(Bevent *buf_event, Json::Value &data){
    std::string group_name = data["groupname"].asString();
    if(m_info->group_is_exist(group_name)){
        Json::Value val;
        val["cmd"] = "create_group_reply";
        val["result"] = "group_exist";
        write_Data(buf_event, &val);
        return;
    }

    m_db->database_connect();
    m_db->database_add_new_group(group_name, data["owner"].asString());
    m_db->database_disconnect();

    m_info->add_new_group(group_name, data["owner"].asString());
    
    Json::Value val;
    val["cmd"] = "create_group_reply";
    val["result"] = "suceess";

    write_Data(buf_event, &val);
}

// 加入群聊
void Thread::join_group(Bevent* buf_event, Json::Value& data){
    std::string group_name = data["groupname"].asString();
    std::string user_name = data["username"].asString();
    if(!m_info->group_is_exist(group_name)){
        Json::Value val;
        val["cmd"] = "join_group_reply";
        val["result"] = "group_not_exist";
        write_Data(buf_event, &val);
        return;
    }

    if(m_info->user_is_in_group(user_name, group_name)){
        Json::Value val;
        val["cmd"] = "join_group_reply";
        val["result"] = "already_in_group";
        write_Data(buf_event, &val);
        return;
    }

    m_db->database_connect();
    m_db->database_update_group_member(group_name, user_name);
    m_db->database_disconnect();

    m_info->update_groups_member(group_name, user_name);

    Bevent* temp_bevent;
    std::string members;
    std::list<std::string> users = m_info->get_group_members(group_name);

    for(std::list<std::string>::iterator it = users.begin(); it!= users.end(); it++){
        if(*it == user_name) continue;

        members += *it;
        members += "|";

        temp_bevent = m_info->user_is_in_m_users(*it);
        if(NULL != temp_bevent){
            Json::Value val;
            val["cmd"] = "new_member_join";
            val["username"] = user_name;
            val["groupname"] = group_name;
            write_Data(temp_bevent, &val);
        }
    }

    members.erase(members.size() - 1);

    Json::Value val;
    val["cmd"] = "joingroup_reply";
    val["username"] = user_name;
    val["members"] = members;
    write_Data(buf_event, &val);
}

// 给群组所有成员发送消息
void Thread::group_chat(Bevent* buf_event, Json::Value& data){
    std::string group_name = data["groupname"].asString();
    std::list<std::string> users = m_info->get_group_members(group_name);

    Bevent* send_message;
    for(auto it = users.begin(); it != users.end(); it++){
        if(*it == data["username"].asString()) return;

        Bevent* temp_event = m_info->user_is_in_m_users(*it);
        if(NULL == temp_event) continue;

        Json::Value val;
        val["cmd"] = "groupchat_reply";
        val["from"] = data["username"];
        val["groupname"] = group_name;
        val["text"] = data["text"];

        write_Data(temp_event, &val);
    }
}

// 发送文件
void Thread::transfer_file(Bevent* buf_event, Json::Value& data){}

// 用户下线
void Thread::client_offline(Bevent* buf_event, Json::Value& data){
    std::string user_name = data["user_name"].asString();
    m_info->delete_user(user_name);
    bufferevent_free(buf_event);

	std::string friends, group;
    m_db->database_connect();
    m_db->database_get_user_friend_and_group(data, friends, group);
    m_db->database_disconnect();
    
    std::string str[1024];
    int num = parse_string(friends, str);

    for(int i = 0; i < num; i++){
        Json::Value val;
        val["cmd"] = "firend_offline";
        val["username"] = user_name;

        Bevent* b = m_info->user_is_in_m_users(str[i]);
        if(NULL != b){
            write_Data(b, &val);
        }
    }

    std::cout << user_name << ":client offline" << std::endl;
}

// 获取群组成员
void Thread::get_group_member(Bevent* buf_event, Json::Value& data){
    std::string group_name = data["groupname"].asString();
    std::string members;
    m_info->get_group_member(group_name, members);

    Json::Value val;
    val["cmd"] = "groupmember_reply";
    val["member"] = members;
    write_Data(buf_event, &val);
}

void Thread::worker(Thread* t){
    t->run();
}

// 计时器的回调函数，将arg转成所需要类
void Thread::timeout_cb(evutil_socket_t fd, short event, void *arg){
    Thread *t = (Thread *)arg;
}

// 读取回调函数的数据，并执行相应的操作
void Thread::thread_readcb(Bevent* buf_event, void *args){
    Thread *t = (Thread*) args;

    char buf[1024] = {0};
    if(!t->read_data(buf_event, buf)){
        std::cout << "read data error" << std::endl;
        return;
    }

    // std::cout << "thread_id:" << t->get_id() << "receive data:" << buf << std::endl;
    Json::Reader reader;
    Json::Value data;
    if(!reader.parse(buf, data)){
        std::cout << "parse data error" << std::endl;
        return;
    }

    if (data["cmd"] == "register"){
		t->user_register(buf_event, data);
	}else if (data["cmd"] == "login"){
		t->user_login(buf_event, data);
    }else if (data["cmd"] == "addfriend"){
		t->user_addfriend(buf_event, data);
	}else if (data["cmd"] == "private"){
		t->private_chat(buf_event, data);
	}else if (data["cmd"] == "creategroup"){
		t->create_group(buf_event, data);
	}else if (data["cmd"] == "joingroup"){
		t->join_group(buf_event, data);
	}else if (data["cmd"] == "groupchat"){
		t->group_chat(buf_event, data);
	}else if (data["cmd"] == "file"){
		t->transfer_file(buf_event, data);
	}else if (data["cmd"] == "offline"){
		t->client_offline(buf_event, data);
	}else if (data["cmd"] == "groupmember"){
		t->get_group_member(buf_event, data);
	}

}

// 检测事件是否结束，并释放Bevent
void Thread::thread_eventcb(Bevent* buf_event, short flag, void* arg){
    if(flag & BEV_EVENT_EOF){
        std::cout << "client close" << std::endl;
        free(buf_event);
    }{
        std::cout << "client error" << std::endl;
    }
}

}