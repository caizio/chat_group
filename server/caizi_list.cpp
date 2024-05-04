#include "caizi_list.h"

namespace caizi{

Info::Info(){
    m_users = new std::list<User>;
    m_groups = new std::map<std::string, std::list<std::string>>;
}

Info::~Info(){
    if(m_users) delete m_users;
    if(m_groups) delete m_groups;
}

// @brief 获取用户事件
struct bufferevent* Info::get_user_buffevent(std::string user_name){
    std::unique_lock<std::mutex> lck(m_user_mutex);
    for(auto it = m_users->begin(); it != m_users->end(); it++){
        if(it->name == user_name){
            return it->bev;
        }
    }
    return nullptr;
}

// @brief 更新用户
bool Info::update_users(std::string user_name, struct bufferevent *bev){
    User user = {user_name,bev};
    std::unique_lock<std::mutex> lck(m_user_mutex);
    m_users->push_back(user);
    return true;
}

bool Info::update_users(Json::Value& data, struct bufferevent *bev){
    User user = {data["username"].asString(), bev};
    std::unique_lock<std::mutex> lck(m_user_mutex);
    m_users->push_back(user);
    return true;
}

// @brief 删除用户
void Info::delete_user(std::string user_name){
    std::unique_lock<std::mutex> lck(m_user_mutex);
    for(auto it = m_users->begin(); it != m_users->end(); it++){
        if(it->name == user_name){
            m_users->erase(it);
            return;
        }
    }
}

// 判断某个用户是否在线
struct bufferevent* Info::user_is_in_m_users(const std::string& name){
    std::unique_lock<std::mutex> lck(m_user_mutex);
    for(auto it = m_users->begin(); it != m_users->end(); it++){
        if(it->name == name){
            return it->bev;
        }
    }
    return nullptr;
}


// @brief 判断user_name是否在group_name中
bool Info::user_is_in_group(std::string user_name, std::string group_name){
    std::unique_lock<std::mutex> lck(m_group_mutex);
    auto it = m_groups->find(group_name);
    if(it != m_groups->end()){
        for(auto i = it->second.begin(); i != it->second.end(); it++){
            if(*i == user_name){
                return true;
            }
        }
    }
    return false;
}

//  判断是否存在某个群组
bool Info::group_is_exist(std::string name){
	std::unique_lock<std::mutex> lck(m_group_mutex);
	for (auto it = m_groups->begin(); it != m_groups->end(); it++){
		if (it->first == name){
			return true;
		}
	}
	return false;
}

// @brief 创建一个新的群组，并添加一个用户
void Info::add_new_group(std::string group_name, std::string user_name){
    std::list<std::string> users_name;
    users_name.push_back(user_name);
    std::unique_lock<std::mutex> lck(m_group_mutex);
    m_groups->insert({group_name,users_name});
}

// @brief 更新groups
// @param[in] g 所有群组信息 {"groupname1|user1|user2|user3", "groupname2|user1|user2|user3"}
// @param[in] size 群组数量
void Info::update_groups(std::string *g, int size){
    int index, start = 0;
    std::string group_name, user_name;
    std::list<std::string> users_name;
    for(int i = 0; i < size; i++){
        index = g[i].find('|');
        group_name = g[i].substr(0,index);
        start = index + 1;
        while(1){
            index = g[i].find('|',index + 1);
            if(index == -1) break;
            user_name = g[i].substr(start,index - start); 
            users_name.push_back(user_name);
            start = index + 1;
        }
        user_name = g[i].substr(start);
        users_name.push_back(user_name);
        m_groups->insert(std::pair<std::string, std::list<std::string>>(group_name, users_name));
        users_name.clear();
    }
}

// 获取群组对应的成员
void Info::get_group_member(std::string group_name, std::string &result){
    std::unique_lock<std::mutex> lck(m_group_mutex);
    auto it = m_groups->begin();
    for(; it != m_groups->end(); it++){
        if(it->first == group_name){
            for(auto &p:it->second){
                result.append(p);
                result.append("|");
            }
            break;
        }
    }
    if(it != m_groups->end() && result.length() != 0){
        result.erase(result.length() - 1, 1);
    }
}


// @brief 获取某个群组的所有成员
std::list<std::string> & Info::get_group_members(std::string group_name){
	auto it = m_groups->begin();

	std::unique_lock<std::mutex> lck();

	for (; it != m_groups->end(); it++){
		if (it->first == group_name){
			break;
		}
	}
	return it->second;
}

// @brief 输出群组信息
void Info::print_groups(){
    for(auto it = m_groups->begin(); it != m_groups->end(); it++){
        std::cout << it->first << "\t";
        for(auto i = it->second.begin(); i != it->second.end(); i++){
            std::cout << *i << "\t";
        }
        std::cout << std::endl;
    }
}

// 往group添加user
void Info::update_groups_member(std::string group, std::string user){
	std::unique_lock<std::mutex> lck(m_group_mutex);
	for (auto it = m_groups->begin(); it != m_groups->end(); it++)
	{
		if (it->first == group)
		{
			it->second.push_back(user);
		}
	}
}

}
