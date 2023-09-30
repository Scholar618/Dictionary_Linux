#include "dic.h"

int search_word(int sfd); // 查询单词
int search_history(int sfd); // 查询历史

// 注册
int do_register(int sfd, struct sockaddr_in sin) {
	char name[N] = ""; 	// 用户名
	char pwd[N] = "";	// 密码
	char wuf[128] = ""; // 发送包
	char ch;
	int i = 0;
	ssize_t res;

	while(1) {
		// 用户名
		memset(name, 0, sizeof(name)); // 填充用户名
		printf("请输入用户名 ： ");
		fgets(name, sizeof(name), stdin);
		name[strlen(name) - 1] = 0; // 将读取的'\n'去除

		// 密码(屏蔽密码，用'*'代替)
		struct termios old_term, new_term;
		tcgetattr(STDIN_FILENO, &old_term); // 保存旧状态
		new_term = old_term;
		new_term.c_lflag &= ~(ECHO | ICANON); // 禁用回显和行缓冲两个属性
		tcsetattr(STDIN_FILENO, TCSANOW, &new_term); // 使用新状态

		printf("请输入密码 ： ");
		while((ch = getchar()) != '\n' && i < N) {
			pwd[i++] = ch;
			printf("*");
		}
		pwd[i] = 0;

		// 恢复终端属性
		tcsetattr(STDIN_FILENO, TCSANOW,&old_term);

		// 组装一个登录请求包
		int size;
		char *pr = wuf;
		unsigned short *pr1 = (unsigned short*) pr;
		*pr1 = htons(R); // 注册操作码，赋值为R(0)

		// 存放用户名
		char *pr2 = pr + 2;
		strcpy(pr2, name);

		// 存放结束符'\0'
		char *pr3 = pr2 + strlen(pr2);
		*pr3 = 0;

		// 存放密码
		char *pr4 = pr3 + 1;
		strcpy(pr4, pwd);

		// 登录请求包大小
		size = 2 + strlen(pr2) + 1 + strlen(pr4) + 1;

		// 发送请求包
		if(send(sfd, wuf, size, 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
		printf("\nsend R success!\n");
		printf("%s %s", name, wuf + strlen(name) + 1 + 2); // 打印用户名和密码

		// 接收请求包
		memset(wuf, 0, sizeof(wuf));
		res = recv(sfd, wuf, sizeof(wuf), 0);
		if(res < 0) {
			ERR_MSG("recv");
			return -1;
		} else {
			// 判断返回包
			char* str = wuf;
			unsigned short *str1 = (unsigned short *)str;
			int num = ntohs(*str1);

			if(2 == num) {
				printf("\nRegister success! Please return and login!\n");
				goto END;
			} else if(3 == num) {
				printf("\nFailed register! Please try again!\n");
			}

		}

	}
	END:
		return 0;
}

// 登录
int do_login(int sfd, struct sockaddr_in sin) {
	char name[N] = ""; 	// 用户名
	char pwd[N] = "";	// 密码
	char wuf[128] = ""; // 发送包
	char ch, c;
	int i = 0;
	ssize_t res;

	while(1) {
		// 用户名
		memset(name, 0, sizeof(name)); // 填充用户名
		printf("请输入用户名 ： ");
		fgets(name, sizeof(name), stdin);
		name[strlen(name) - 1] = 0; // 将读取的'\n'去除

		// 密码(屏蔽密码，用'*'代替)
		struct termios old_term, new_term;
		tcgetattr(STDIN_FILENO, &old_term); // 保存旧状态
		new_term = old_term;
		new_term.c_lflag &= ~(ECHO | ICANON); // 禁用回显和行缓冲两个属性
		tcsetattr(STDIN_FILENO, TCSANOW, &new_term); // 使用新状态

		printf("请输入密码 ： ");
		while((ch = getchar()) != '\n' && i < N) {
			pwd[i++] = ch;
			printf("*");
		}
		pwd[i] = 0;

		// 恢复终端属性
		tcsetattr(STDIN_FILENO, TCSANOW,&old_term);

		// 组装一个登录请求包
		int size;
		char *pr = wuf;
		unsigned short *pr1 = (unsigned short*) pr;
		*pr1 = htons(L); // 注册操作码，操作码2个字节

		// 存放用户名
		char *pr2 = pr + 2;
		strcpy(pr2, name);

		// 存放结束符'\0'
		char *pr3 = pr2 + strlen(pr2);
		*pr3 = 0;

		// 存放密码
		char *pr4 = pr3 + 1;
		strcpy(pr4, pwd);

		// 登录请求包大小
		size = 2 + strlen(pr2) + 1 + strlen(pr4) + 1;
		
		// 发送请求包
		if(send(sfd, wuf, size, 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
		printf("\nsend L success!\n");

		// 接收请求包
		memset(wuf, 0, sizeof(wuf));
		res = recv(sfd, wuf, sizeof(wuf), 0);
		if(res < 0) {
			ERR_MSG("recv");
			return -1;
		} else {
			// 判断返回包
			char* str = wuf;
			unsigned short *str1 = (unsigned short *)str;
			int num = ntohs(*str1);

			if(Y == num) {
				printf("Login success!\n");
				goto LOOP; // 进入登陆成功界面
			} else if(E == num) {
				printf("Failed Login! Please try again!\n");
			}

		}
	}
LOOP:
	while(1) {
		// 二级菜单
		system("clear"); // 清屏
		printf("----------------------------\n");
		printf("----------S.查询单词---------\n");
		printf("----------H.查询历史---------\n");
		printf("----------Q.返回上一级-------\n");
		printf("---------------------------\n");

		printf("Please choose : ");
		c = getchar();
		while(getchar() != 10); // 不断读取字符，直到读取了'\n'退出

		switch (c)
		{
		case 'S':
			search_word(sfd);
			break;
		case 'H':
			search_history(sfd);
		case 'Q':
			goto END;
		default:
			printf("输入错误，请重新选择！\n");
		}
	
	}
END:
	return 0;

}

// 查询单词
int search_word(int sfd) {
	char word[N];
	char wuf[20] = ""; // 发送包
	ssize_t res;
	char b;

	while(1) {
		printf("退出查询请输入Q!!\n");
		printf("请输入要查询单词(小写开头) : ");
		memset(word, 0, sizeof(word)); // 填充单词
		fgets(word, sizeof(word), stdin);

		if(strcasecmp(word, "Q") != 0) {
			word[strlen(word) - 1] = 0; // 单词末尾填上'\0'
			memset(wuf, 0, sizeof(wuf));

			// 组包变量,封装单词请求包
			int size;
			char *pr = wuf;
			unsigned short *pr1 = (unsigned short *)pr;
			*pr1 = htons(D); // 查询操作码赋值为D(4)

			// 单词
			char* pr2=pr+2;
			strcpy(pr2,word);
 
			size=2+strlen(pr2);//单词请求包大小

			// 发送请求包
			if(send(sfd, wuf, size, 0) < 0) {
				ERR_MSG("send");
				return -1;
			}

			printf("%s", pr2); // 单词
			printf("send L success!\n");

			// 接收返回包
			memset(wuf, 0, sizeof(wuf));
			res = recv(sfd, wuf, sizeof(wuf), 0);
			if(res < 0) {
				ERR_MSG("res");
				return -1;
			} else {
				// 判断返回包
				char *str = wuf;
				unsigned short *str1 = (unsigned short *)str;
				int num = ntohs(*str1);

				if(Y == num) {
					printf("查询成功 %s : %s\n",word, wuf+2);
					printf("是否继续查询(Y/N) : ");
					scanf("%c", &b);
					if(b == 'Y') {
						continue;
					} else if(b == 'N'){
						break;
					} else {
						printf("输入错误！自动退出！！\n");
						break;
					}
				} else if(E == num) {
					printf("查询失败，返回重新输入！\n");
				}
			}
		} else { // word == 'Q' 
			memset(wuf, 0, sizeof(wuf));
			char *w = wuf;
			unsigned short *q = (unsigned short *)w;
			*q = htons(6);
			if(send(sfd, wuf, 2, 0) < 0) {
				ERR_MSG("send");
				return -1;
			}
			goto END;
		}
	}
END:
	return 0;
}

// 查询历史
int search_history(int sfd) {
	char name[N]; // 用户名
	char pwd[N]; // 密码
	char wuf[128]="";//发送包
	ssize_t res ;
	char b;

	while(1) {
		printf("---结束查询输入Q！---\n");
		printf("请输入要查询的用户名：");
		// 用户名
		memset(name, 0, sizeof(name));
		fgets(name, sizeof(name), stdin);
		if(strcasecmp(name, "Q") != 0) {
			name[strlen(name)-1] = 0;
			memset(wuf, 0, sizeof(wuf));
			// 封装查询历史请求包
			int size;
			char *pr = wuf;
			unsigned short *pr1 = (unsigned short *)pr;
			*pr1 = htons(S); // 查询历史操作码
			
			char *pr2 = pr + 2;
			strcpy(pr2, name);

			size = 2 + strlen(pr2);

			// 发送请求包
			if(send(sfd, wuf, size, 0) < 0) {
				ERR_MSG("send");
				return -1;
			} else {
				char* str=wuf;
				unsigned short* w1=(unsigned short*)str;
	
				int num=ntohs(*w1);
				printf("num=%d\n",num);
 
				if(Y==num)
				{
					printf("查询成功\n");
					printf("用户[%s]的查询记录为：%s\n", name, wuf+2);
					break;
				}
				else if(E==num)
				{
					printf("查询失败,请重新输入\n");
				}

			}
		} else { // word = 'Q'
			memset(wuf, 0, sizeof(wuf));
			char *w = wuf;
			unsigned short *q = (unsigned short *)w;
			*q = htons(6);
			if(send(sfd, wuf, 2, 0) < 0) {
				ERR_MSG("send");
				return -1;
			}
			goto END;
		}
	}
END:
	return 0;
}



int main(int argc, const char* argv[]) 
{
	// 流式套接字
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd < 0) {
		ERR_MSG("sfd");
		return -1;
	}

	// 要连接服务器的IP和PORT
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);				// 端口号
	sin.sin_addr.s_addr = inet_addr(IP);	// IP地址
	
	// 连接服务器
	if(connect(sfd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		ERR_MSG("coonect");
		return -1;
	}
	printf("Connect success!\n");

	char c; // 输入的字符
	while(1) {
		// 一级菜单
		system("clear"); // 清屏
		printf("--------------------------\n");
		printf("----------R.注册-----------\n");
		printf("----------L.登录-----------\n");
		printf("----------Q.退出-----------\n");
		printf("--------------------------\n");

		printf("Please choose : ");
		c = getchar();
		while(getchar() != 10); // 不断读取字符，直到读取了'\n'退出
		switch(c) {
			case 'R':
				do_register(sfd, sin);
				break;
			case 'L':
				do_login(sfd, sin);
				break;
			case 'Q':
				printf("退出应用！！\n");
				close(sfd);
				return 0;
			default:
				printf("输入字符错误！\n");

		}

		printf("------输入任意字符清屏-------：");
		while(getchar()!=10);
	}




	return 0;
}