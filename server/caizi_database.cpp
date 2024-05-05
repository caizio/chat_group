#include "caizi_database.h"

namespace caizi{

DataBase::DataBase(){}

DataBase::~DataBase(){}

bool DataBase::database_connect(){
	//初始化数据库句柄
	mysql = mysql_init(NULL);

	//连接数据库
	mysql = mysql_real_connect(mysql, "localhost", "root", "123", 
			"caizi_database", 0, NULL, 0);
	if (NULL == mysql){
		std::cout << "mysql_real_connect error" << std::endl;
		return false;
	}

	//设置编码格式
	if (mysql_query(mysql, "set names utf8;") != 0){
		std::cout << "set names utf8 error" << std::endl;
		return false;
	}
	return true;
}

void DataBase::database_disconnect(){
	mysql_close(mysql);
}

void DataBase::database_close(){
	database_disconnect();
}

bool DataBase::database_init_table(){
	database_connect();
	const char *g = "create table if not exists caizi_group(groupname varchar(128), groupowner varchar(128),groupmember varchar(4096))charset utf8;";
	if (mysql_query(mysql, g) != 0){
		return false;
	}

	const char *u = "create table if not exists caizi_user(username varchar(128), password varchar(128), friendlist varchar(4096), grouplist varchar(4096))charset utf8;";

	if (mysql_query(mysql, u) != 0){
		return false;
	}

	database_disconnect();

	return true;
}

int DataBase::database_get_group_info(std::string *g)
{
	if (mysql_query(mysql, "select * from caizi_group;") != 0)
	{
		std::cout << "select error" << std::endl;
		return -1;
	}

	MYSQL_RES *res = mysql_store_result(mysql);
	if (NULL == res){
		std::cout << "store result error" << std::endl;
		return -1;
	}

	MYSQL_ROW r;
	int idx = 0;
	while (r = mysql_fetch_row(res)){
		g[idx] += r[0];
		g[idx] += '|';
		g[idx] += r[2];
		//std::cout << g[idx] << std::endl;
		idx++;
	}

	mysql_free_result(res);

	return idx;
}

bool DataBase::database_user_is_exist(std::string u)
{
	char sql[256] = {0};

	sprintf(sql, "select * from user where username = '%s';", u.c_str());

	std::unique_lock<std::mutex> lck(_mutex);

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "select error" << std::endl;
		return true;
	}

	MYSQL_RES *res = mysql_store_result(mysql);
	if (NULL == res)
	{
		std::cout << "store result error" << std::endl;
		return true;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (NULL == row)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void DataBase::database_insert_user_info(Json::Value &v)
{
	std::string username = v["username"].asString();
	std::string password = v["password"].asString();

	char sql[256] = {0};

	sprintf(sql, "insert into user (username, password) values ('%s', '%s');", username.c_str(), password.c_str());

	std::unique_lock<std::mutex> lck(_mutex);

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "insert into error" << std::endl;
	}
}

bool DataBase::database_password_correct(Json::Value &v)
{
	std::string username = v["username"].asString();
	std::string password = v["password"].asString();

	char sql[256] = {0};
	sprintf(sql, "select password from user where username = '%s';", username.c_str());

	std::unique_lock<std::mutex> lck(_mutex);

	if (mysql_query(mysql, sql) != 0) 
	{
		std::cout << "select password error" << std::endl;
		return false;
	}
	
	MYSQL_RES *res = mysql_store_result(mysql);
	if(NULL == res)
	{
		std::cout << "mysql store result error" << std::endl;
		return false;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (NULL == row)
	{
		std::cout << "fetch row error" << std::endl;
		return false;
	}

	if (!strcmp(row[0], password.c_str()))
	{
		return true;
	}
	else
	{
		return false;
	}
}

// @brief 查询v["username"]的朋友和群组
// @param[out] friList 查询的朋友列表
// @param[out] groList 查询的群组列表
bool DataBase::database_get_user_friend_and_group(Json::Value &v, std::string &friList, std::string &groList){
	char sql[1024] = {0};
	sprintf(sql, "select * from user where username = '%s';", v["username"].asString().c_str());

	std::unique_lock<std::mutex> lck(_mutex);

	if (mysql_query(mysql, sql) != 0){
		std::cout << "select * error" << std::endl;
		return false;
	}

	MYSQL_RES *res = mysql_store_result(mysql);
	if (NULL == res){
		std::cout << "store result error" << std::endl;
		return false;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (NULL == row){
		return false;
	}

	if (row[2]){
		friList = std::string(row[2]);
	}
	if (row[3]){
		groList = std::string(row[3]);
	}

	return true;
}

void DataBase::database_add_friend(Json::Value &v)
{
	std::string username = v["username"].asString();
	std::string friendname = v["friend"].asString();

	database_update_friendlist(username, friendname);
	database_update_friendlist(friendname, username);
}

void DataBase::database_update_friendlist(std::string &u, std::string &f)
{
	char sql[256] = {0};
	std::string friendlist;
	sprintf(sql, "select friendlist from user where username = '%s';", u.c_str());

	std::unique_lock<std::mutex> lck(_mutex);

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "select friendlist error" << std::endl;
		return;
	}

	MYSQL_RES *res = mysql_store_result(mysql);
	if (NULL == res)
	{
		std::cout << "store result error" << std::endl;
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (NULL == row[0])
	{
		friendlist.append(f);
	}
	else 
	{
		friendlist.append(row[0]);
		friendlist.append("|");
		friendlist.append(f);
	}

	memset(sql, 0, sizeof(sql));

	sprintf(sql, "update user set friendlist = '%s' where username = '%s';", friendlist.c_str(), u.c_str());

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "update user error" << std::endl;
	}
}

// 添加一个新的群组
void DataBase::database_add_new_group(std::string g, std::string owner)
{
	char sql[256] = {0};
	std::string grouplist;

	//修改caizi_group表
	sprintf(sql, "insert into caizi_group values ('%s', '%s', '%s');", 
			g.c_str(), owner.c_str(), owner.c_str());

	std::unique_lock<std::mutex> lck(_mutex);

	if (mysql_query(mysql, sql) != 0){
		std::cout << "insert error" << std::endl;
		return;
	}

	//修改user表
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select grouplist from user where username = '%s';", owner.c_str());

	if (mysql_query(mysql, sql) != 0){
		std::cout << "select friendlist error" << std::endl;
		return;
	}

	MYSQL_RES *res = mysql_store_result(mysql);
	if (NULL == res){
		std::cout << "store result error" << std::endl;
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (NULL == row[0]){
		grouplist.append(g);
	}
	else{
		grouplist.append(row[0]);
		grouplist.append("|");
		grouplist.append(g);
	}

	memset(sql, 0, sizeof(sql));

	sprintf(sql, "update user set grouplist = '%s' where username = '%s';", grouplist.c_str(), owner.c_str());

	if (mysql_query(mysql, sql) != 0){
		std::cout << "update user error" << std::endl;
	}
}

/*
	@brief 更新群组成员
	@param[in] g 群组名
	@param[in] u 用户名
*/ 
void DataBase::database_update_group_member(std::string g, std::string u)
{
	//先修改caizi_group内容
	database_update_info("caizi_group", g, u);

	//再修改user内容
	database_update_info("user", g, u);

}

void DataBase::database_update_info(std::string table, 
		 				std::string groupname, std::string username)
{
	//先把数据读出来
	char sql[256] = {0};
	std::string member;

	if (table == "caizi_group")
	{
		sprintf(sql, "select groupmember from caizi_group where groupname = '%s';", groupname.c_str());
	}
	else if (table == "user")
	{
		sprintf(sql, "select grouplist from user where username = '%s';", username.c_str());
	}

	std::unique_lock<std::mutex> lck(_mutex);

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "select error" << std::endl;
		return;
	}

	MYSQL_RES *res = mysql_store_result(mysql);
	if (NULL == res)
	{
		std::cout << "store result error" << std::endl;
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (row[0] == NULL)
	{
		if (table == "caizi_group")
		{
			member.append(username);
		}
		else if (table == "user")
		{
			member.append(groupname);
		}
	}
	else
	{
		if (table == "caizi_group")
		{
			member.append(row[0]);
			member.append("|");
			member.append(username);
		}
		else if (table == "user")
		{
			member.append(row[0]);
			member.append("|");
			member.append(groupname);
		}
	}

	mysql_free_result(res);

	//修改后再更新
	memset(sql, 0, sizeof(sql));

	if (table == "caizi_group")
	{
		sprintf(sql, "update caizi_group set groupmember = '%s' where groupname = '%s';", member.c_str(), groupname.c_str());
	}
	else if (table == "user")
	{
		sprintf(sql, "update user set grouplist = '%s' where username = '%s';", member.c_str(), username.c_str());
	}

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "update caizi_group error" << std::endl;
	}
}

}

