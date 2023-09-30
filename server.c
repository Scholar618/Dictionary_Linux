#include "dic.h"

//分线程功能函数处理函数
void *callback();
int do_register();
int do_login();
int update();
int do_word();
int do_login();
int do_search();
int do_searchhs();
int do_inserths();


sqlite3 *do_sqlInit();

int main(int argc, char *argv[])
{
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
	{
		ERR_MSG("socket");
		return -1;
	}

	// 控制socket地址重用(保证在一个socket被断开连接时，该socket会在一段时间保持占用状态)
	int optval = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
	{
		ERR_MSG("setsockopt");
		return -1;
	}

	// 地址信息结构体
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = inet_addr(IP); // 绑定IP

	// 绑定服务器IP地址
	if (bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		ERR_MSG("bind");
		return -1;
	}

	// 监听
	if (listen(sfd, 10) < 0)
	{
		ERR_MSG("listen");
		return -1;
	}
	printf("listen success!\n");

	// 设置对端IP
	struct sockaddr_in cin;
	socklen_t addrlen = sizeof(cin);

	pthread_t tid; // 线程
	ssize_t res;
	int newfd;

	struct msg info; // 载入结构体变量

	while (1)
	{
		newfd = accept(sfd, (struct sockaddr *)&cin, &addrlen);
		if (newfd < 0)
		{
			ERR_MSG("newfd");
			return -1;
		}
		printf("[%s:%d] newfd = %d\n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), newfd); // 输出 ： [IP:PORT] newfd = n

		info.newfd = newfd;
		info.cin = cin;

		if (pthread_create(&tid, NULL, callback, (void *)&info) != 0)
		{
			ERR_MSG("pthread_create");
			return -1;
		}

		pthread_detach(tid); // 申请结束线程
	}

	close(sfd);
	return 0;
}

//分线程功能函数处理函数
void *callback(void *arg)
{
	sqlite3 *db = do_sqlInit(); // 表格初始化

	char word[N]; // 单词
	char name[50]; // 用户名
	char pwd[N];  // 密码

	// 对端客户文件描述符
	struct msg info = *(struct msg *)arg;
	int newfd = info.newfd;
	struct sockaddr_in cin = info.cin;
	ssize_t res;
	char buf[128] = "";

	printf("用户%d上线\n", newfd);

	while (1)
	{
		memset(buf, 0, sizeof(buf));
		res = recv(newfd, buf, sizeof(buf), 0);
		if (res < 0)
		{
			ERR_MSG("res");
			break;
		}
		else if (res == 0)
		{ // 收到的字节数为0，表示远程端关闭
			fprintf(stderr, "[%s:%d] 客户端newfd = %d下线\n", inet_ntoa(cin.sin_addr), htons(cin.sin_port), newfd);

			// 更新表格中的flag列
			char sql[128] = "";
			char *errmsg = NULL;
			sprintf(sql, "UPDATE Lr SET flag=%d WHERE name=\"%s\";", R, name); // 设置flag列为R
			if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
			{
				fprintf(stderr, "__%d__sqlite3_get_table: %s \n", __LINE__, errmsg);
			}
			sqlite3_free(errmsg);
			sqlite3_close(db);
			break;
		}

		printf("[%s:%d] newfd=%d : \n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), newfd);

		// 拆包
		char *pr = buf; // 客户端传递来的信息
		printf("%s\n", buf + 2);

		unsigned short *pr1 = (unsigned short *)pr;
		int num = htons(*pr1); // 操作码
		printf("num=%d\n", num);

		// 分解信息
		switch (num)
		{
		case 0: // 注册请求
			// 用户名
			memset(name, 0, sizeof(name));
			strcpy(name, buf + 2);
			// 密码
			memset(pwd, 0, sizeof(pwd));
			strcpy(pwd, buf + 4 + strlen(name + 1));
			// 注册
			do_register(db, newfd, name, pwd);
			break;
		case 1: // 登录请求
			memset(name, 0, sizeof(name));
			strcpy(name, buf + 2);
			memset(pwd, 0, sizeof(pwd));
			strcpy(pwd, buf + 4 + strlen(name + 1));
			// 登录
			printf("正在登陆中-------!\n");
			do_login(db, newfd, name, pwd);
			break;
		default:
			update(db, newfd, name);
			break;
		}
	}
	close(newfd);
	pthread_exit(NULL);
}

// 建表
sqlite3 *do_sqlInit()
{
	// 打开数据库
	sqlite3 *db = NULL;
	if (sqlite3_open("test.db", &db) != SQLITE_OK)
	{
		fprintf(stderr, "__%d__sqlite3_open : %s\n", __LINE__, sqlite3_errmsg(db));
		fprintf(stderr, "Error : %d\n", sqlite3_errcode(db));
		return NULL;
	}
	printf("sqlite3 open success!\n");

	// 创建登录注册表格(Lr)
	char sql1[128] = "create table if not exists Lr (name char PRIMARY KEY, pwd char, flag int);";
	char *errmsg = NULL;
	if (sqlite3_exec(db, sql1, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		fprintf(stderr, "__%d__sqlite3_exec : %s", __LINE__, errmsg);
		return NULL;
	}
	printf("create Lr success!\n");

	// 创建单词表(Words)
	char sql2[128] = "create table if not exists Words (word char, means char);";
	errmsg = NULL;
	if (sqlite3_exec(db, sql2, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		fprintf(stderr, "__%d__sqlite3_exec : %s", __LINE__, errmsg);
		return NULL;
	}
	printf("create Words success!\n");

	// 创建历史记录表(Hs)
	char sql3[128] = "create table if not exists Hs (name char, word char, means char, in_time char);";
	errmsg = NULL;
	if (sqlite3_exec(db, sql3, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		fprintf(stderr, "__%d__sqlite3_exec : %s", __LINE__, errmsg);
		return NULL;
	}
	printf("create Hs success!\n");

	do_word(db); // 初始化电子词典
	return db;
}

// 注册
int do_register(sqlite3 *db, int newfd, char name[N], char pwd[N])
{
	// 注册用户信息并插入表中
	char sql[128] = "";
	char buf[128] = "";
	char **pres;
	int row, column; // 行数和列数
	char *errmsg;
	printf("\n%s\n", name);
	sprintf(sql, "select *from Lr where name = \"%s\";", name); // 查询name
	if (sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK)
	{
		fprintf(stderr, "__%d__sqlite3_get_table : %s", __LINE__, sqlite3_errmsg(db));
		printf("\nrow=%d\n", row);
		// return -1;
	}

	if (row != 1)
	{
		// 行数不为1，则表格中没有该用户，执行插入功能
		memset(sql, 0, sizeof(sql));
		printf("%s		259\n", pwd);
		sprintf(sql, "insert into Lr values(\"%s\", \"%s\", %d)", name, pwd, R);
		// sprintf(sql, "insert into Lr(name, pwd, R) values(\"%s\", \"%s\", %d)");
		if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
		{
			fprintf(stderr, "__%d__ sqlite3_exec : %s", __LINE__, errmsg);
			return -1;
		}

		memset(buf, 0, sizeof(buf));
		char *mr = buf;
		unsigned short *mr1 = (unsigned short *)mr;
		*mr1 = htons(2); // 请求确认包
		send(newfd, buf, 2, 0);
	}
	else
	{
		// 表格中有该用户，注册失败
		memset(buf, 0, sizeof(buf));
		char *mr = buf;
		unsigned short *mr1 = (unsigned short *)mr;
		*mr1 = htons(3); // 错误包
		send(newfd, buf, 2, 0);
	}
	sqlite3_free_table(pres);
	return 0;
}

// 登录
int do_login(sqlite3 *db, int newfd, char name[N], char pwd[N]) {
	char sql[128] = "";
	char buf[128] = "";
	char **pres;
	int row, column; // 行数和列数
	char *errmsg;

	int num; 
	char name1[N] = "";
	char word[N] = "";

	sprintf(sql, "select *from Lr where name=\"%s\";", name);
	if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "__%d__sqlite3_get_table : %s", __LINE__, errmsg);
		return -1;
	}

	if(row == 1) { // 存在该用户
		memset(sql, 0, sizeof(sql));
		// 判断是否重复登录
		sprintf(sql, "select *from Lr where name=\"%s\" and flag = %d", name, L);
		if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK)
		{
			fprintf(stderr, "%d sqlite3_get_table: %s\n", __LINE__, errmsg);
			return -1;
		}

		if(row != 1) { // 要登录
			memset(sql, 0, sizeof(sql));
			// 更改登录状态(上线)
			sprintf(sql, "update Lr set flag=%d where name=\"%s\";", L, name);
			if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK)
			{
				fprintf(stderr, "%d sqlite3_get_table: %s\n", __LINE__, errmsg);
				return -1;
			}	

			// 发送Y提示登陆成功
			memset(buf, 0, sizeof(buf));
			char *mr = buf;
			unsigned short* mr1 = (unsigned short*)mr;
			*mr1 = htons(2);
			send(newfd, buf, 2, 0);
			printf("登陆成功，请继续操作！\n");
			goto Loop;
		} else { // 已经登录，发送E提示登录失败
			memset(buf, 0, sizeof(buf));
			char *mr = buf;
			unsigned short* mr1 = (unsigned short*)mr;
			*mr1 = htons(3);
			send(newfd, buf, 2, 0);
		}

	} else { // 不存在该用户
		// 登录失败。
		memset(buf, 0, sizeof(buf));
		char *mr = buf;
		unsigned short* mr1 = (unsigned short*)mr;
		*mr1 = htons(3);
		send(newfd, buf, 2, 0);
	}
// 登陆成功进入查询界面
Loop:
	while(1) {
		memset(buf, 0, sizeof(buf));
		if(recv(newfd, buf, sizeof(buf), 0) < 0) { // 接收单词或历史包
			ERR_MSG("recv");
			return -1;
		}

		char *ws = buf;
		unsigned short* ws1 = (unsigned short*)ws;
		num = ntohs(*ws1);
		switch(num) {
			case 4: // 单词查询包
				memset(word, 0, sizeof(word));
				strcpy(word, buf+2);

				// 查询单词
				printf("-----单词查询中-----\n");
				do_search(db, word, newfd, name);
				break;
			case 5: // 查询历史
				strcpy(name1, buf+2);
				printf("-----历史查询中-----\n");
				do_searchhs(db, name1, newfd);
				break;
			case 6: // 退出
				update(db, newfd, name);
				exit(1);
			default:
				break;
		}
	}
	// 释放
	sqlite3_free_table(pres);
	return 0;
}

// 查询单词
int do_search(sqlite3 *db, char word[N], int newfd, char name[N]) {
	char sql[128] = "";
	char buf[128] = "";
	char means[20] = "";
	char **pres;
	int row, column; // 行数和列数
	char *errmsg;

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select *from Words where word=\"%s\";", word); // 查询单词语句
	if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "__%d__sqlite3_get_table : %s", __LINE__, errmsg);
		return -1;
	}

	if(row > 0) { // 存在该单词
		memset(buf, 0, sizeof(buf));
		char *w = buf;
		unsigned short *w1 = (unsigned short*)w;
		*w1 = htons(Y);
		// 忽略第一行名称
		for(int i = column; i < (row + 1)*column; i++) {
			if(i % column == (column - 1)) {
				// 获取单词means
				memset(means, 0, sizeof(means));
				sprintf(means, "%s", pres[i]);
			}
		}

		strcpy(buf+2, means);
		send(newfd, buf, 2+strlen(means), 0);
		printf("查询成功!!!\n");
		// 插入单词注解和用户名到历史记录表中
		do_inserths(db, word, means, name);

	} else { // 不存在该单词
		memset(buf, 0, sizeof(buf));
		char *w = buf;
		unsigned short *w1 = (unsigned short*)w;
		*w1 = htons(E);
		send(newfd, buf, 2, 0);
		printf("查询失败!!!\n");
	}
	return 0;
}

int callback1(void *data, int argc, char** argv, char **azColName) {
    int *recordCount = (int *)data;
    *recordCount = atoi(argv[0]);
    return 0;
}

// 单词表初始化
int do_word(sqlite3* db) {
	// 打开单词文件
	FILE *fp = fopen("dict3.txt", "r");
	if(fp == NULL) {
		perror("open");
		return -1;
	}
	printf("------正在初始化------\n");
	// sleep(5); // 去掉即可
	
	char word[20]="";
	char *errmsg;
	char buf[N]="";
	char means[30]="";
	char sql[100]="";
	int count = 0;

	memset(sql, 0, sizeof(sql));
	// 检查表中是否有内容，有则不执行插入语句
	sprintf(sql, "select count(*) from Words;");
	int recordCount = 0;
	if(sqlite3_exec(db, sql, callback1, &recordCount, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "__%d__ : %s", __LINE__, errmsg);
        return -1;
	}
	if(recordCount>0) {
		printf("表中已有%d个信息，存入结束!\n", recordCount);
	} else {
		while(1) {

			memset(buf, 0, sizeof(buf));
			memset(word, 0, sizeof(word));
			memset(means, 0, sizeof(means));

			char *line = fgets(buf, sizeof(buf), fp); // 读取一行
			buf[strlen(buf) - 1] = 0;
			// p为遍历buf的指针
			char *p = buf;
			if(line == NULL) { // 循环读取到最后一行结束
				fprintf(stderr, "--------读取结束,");
				break;
			} else { // 未结束
				while(1) {
					/* if(*p==' ' && *(p+1)==' ' && *(p+2)==' ') {
						*p = '\0';
						strcpy(word, buf);
						// printf("word已存入！！\n");			
						break;
					}
					p++;
				}
				while(1) {
					if(*p == ' ' && *(p+1) != ' ') {
						strcpy(means, p+1);
						// printf("means已存入！！\n");
						break;
					}
					p++;
				}
				sprintf(sql, "insert into Words values(\"%s\", \"%s\")", word, means);
				// 执行插入语句
				if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
				{
					fprintf(stderr, "__%d__sqlite3_exec : %s", __LINE__, errmsg);
					return -1;
				}	 */
					if(*p==' '&&*(p+1)==' '&&*(p+2)==' ')
					{		
						*p='\0';
						strcpy(word,buf);
						// printf("word已经读取!\n");
	/* 					while(p++) {
							if(*p!=' ') {
								strcpy(means,p);
								break;
							}
						} */
						strcpy(means, p+3);
						// printf("means已经读取!\n");
						sprintf(sql,"insert into Words values(\"%s\", \"%s\");",word,means);

						//执行插入语句
						if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)
						{
							fprintf(stderr,"__%d__:sqlite3_exec%s\n",__LINE__,errmsg);
							return -1;
						}
						break;
					}
					p++;
			}
			count++;
			printf("%d信息已经存入!\n", count);
		}
		}
	
	}
	printf("电子词典初始化成功--------\n");
	fclose(fp);
	return 0;
}

// 插入单词注解和用户名到历史记录表中
int do_inserths(sqlite3* db, char word[N], char means[N], char name[N]) {
	printf("插入历史记录表：word=%s\tname=%s\t764\n",word, name);

	char sql[N];
	char* errmsg;
	time_t newTime; // 获取查询时间
	time(&newTime);
	// 修改时间格式
	struct tm *nowTime = localtime(&newTime);
	char timeString[N];
	strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", nowTime);
	
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "insert into Hs values (\"%s\", \"%s\", \"%s\", \"%s\");", name, word, means, (char*)nowTime);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "__%d__ error_code:%d sqlite3_exec:%s\n", __LINE__,sqlite3_errcode(db),errmsg);
		return -1;
	}

	printf("已插入历史记录表中！\n");
	return 0;
}

// 查询历史记录
int do_searchhs(sqlite3* db, char name[N], int newfd) {
	char sql[128] = "";
	char buf[128] = "";

	char word[20] = "";
	char means[50] = "";
	char time[20] = "";

	int i;
	char **pres = NULL;
	char *errmsg;
	int row, column;

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select *from Hs where name=\"%s\";", name);
	if(sqlite3_get_table(db,sql,&pres,&row,&column,&errmsg)!=SQLITE_OK)
	{
		fprintf(stderr,"__%d__ error_code:%d sqlite3_exec:%s\n",__LINE__,sqlite3_errcode(db),errmsg);
		return -1;
	}
	
	if(row > 0) { // 查询成功
		printf("------历史记录查询中------\n");
		for(i = column; i < (row+1)*column; i++) {
			if(i%column == 1) { // word
				memset(word, 0, sizeof(word));
				strcpy(word, pres[i]);
				printf("word=%s\t", word);
				printf("column=%d", column);
			}
			if(i%column == 2) { // means
				memset(means, 0, sizeof(means));
				strcpy(means, pres[i]);
				printf("means=%s\t", means);
			}
			if(i%column == 3) { // time
				memset(time, 0, sizeof(time));
				strcpy(time, pres[i]);
				printf("time=%s\t", time);

				// 发送该信息请求保存
				memset(sql, 0, sizeof(sql));
				char* w = sql;
				unsigned short* w1 = (unsigned short*)w;
				*w1 = htons(Y);

				sprintf(sql+2, "%s,%s,%s", word, means, time);
				printf("%s\n", sql+2);
				if(send(newfd, sql, 2+strlen(word)+strlen(means)+strlen(time)+1+1, 0) < 0) {
					ERR_MSG("send");
					return -1;
				}

			}
		}

	} else if(row == 0) { // 无查询记录
		fprintf(stderr,"用户%d已无查询记录,程序即将退出\n",newfd);
		update(db,newfd,name);
		exit(1);

	} else { // 查询失败
		printf("查询失败\n");
		memset(sql, 0, sizeof(sql));
 
		char* q=sql;
		*q=htons(E);//错误报
 
		if(send(newfd,sql,2,0)<0)
		{
			ERR_MSG("send");
			return -1;
		}

	}
	sqlite3_free_table(pres);
	return 0;

}

// 更新用户状态
int update(sqlite3* db, int newfd, char name[N]) {
	char sql[128]="";
	char* errmsg=NULL;
	sprintf(sql,"UPDATE Lr SET flag=%d WHERE user=\"%s\";",R,name);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		fprintf(stderr, "__%d__ sqlite3_get_table: %s\n", __LINE__, errmsg);
	}
			
	sqlite3_close(db);//关闭用户表
 
	printf("用户%d已下线\n",newfd);
	return 0;


}

