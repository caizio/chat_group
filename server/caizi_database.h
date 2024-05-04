#ifndef _CAIZI_DATABASE_H
#define _CAIZI_DATABASE_H

#include <mysql/mysql.h>
#include <mutex>
#include <iostream>
#include <stdio.h>
#include <jsoncpp/json/json.h>
#include <cstring>

namespace caizi{

class DataBase{
private:
    MYSQL *mysql;
    std::mutex _mutex;
public:
	DataBase();
	~DataBase();
	bool database_connect();
	void database_disconnect();
	void database_close();
	bool database_init_table();
	bool database_user_is_exist(std::string);
	void database_insert_user_info(Json::Value &);
	int database_get_group_info(std::string *);
	bool database_password_correct(Json::Value &);
	bool database_get_friend_group(Json::Value &, std::string &, std::string &);
	void database_add_friend(Json::Value &);
	void database_update_friendlist(std::string &, std::string &);
	void database_add_new_group(std::string, std::string);
	void database_update_group_member(std::string, std::string);
	void database_update_info(std::string, std::string, std::string);
};

}






#endif