#include <bits/stdc++.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <conio.h>
//#pragma comment (lib, "ws2_32.dll")
using namespace std;
string version = "1.0.1"; //版本号
const int Max = 64; //客户端最大连接数
int message_max = 200; //最大缓存聊天记录 
char message[200][300]; //聊天记录(上限为前[])
void* th_accept(void* arg); //声明线程-监听
void* th_listen(void* arg); //声明线程-接收
void* th_send(void* arg); //声明线程-发送(广播)
void* th_Dashboard(void* arg); //声明线程-仪表盘
void ip_(); //获声明函数-取服务器IP
string time_(); //声明函数-获取时间
string day_(); //声明函数-获取日期
void write_(string fpm, char* data_); //声明函数-文件写入
void data_read(); //声明函数-聊天记录读取 
////////////////////////////////////////////////////////////////
pthread_mutex_t lock; //声明互斥锁
pthread_t tid_listen; //监听线程ID 
pthread_t tid_send; //发送线程ID 
pthread_t tid_Dashboard; //仪表盘线程ID 
//-创建客户端信息结构体
struct SockInfo{ //客户端信息结构体
    int fd; //文件描述符
    pthread_t tid; //线程ID
    string ip; //客户端IP
    int port; //客户端端口
    string message_listen; //接收的信息
    string name; //客户端昵称
};
struct SockInfo infos[Max]; //客户端信息数
pthread_t key[Max]; //客户端对应线程ID
char* sip; //服务器IP
char* data_name; //聊天记录文件名 
char* user_name; //用户记录文件名 
bool change_state = false; //用户状态是否需要更新 
bool change_message = false; //聊天记录是否需要更新 
int user_order[Max]; //用户排序 
int tail_p = 0; //聊天记录指针 
bool cycle = false; //聊天记录循环 
////////////////////////////////////////////////////////////////
int main(){
	system("title 信息墙服务端");
	WSADATA wsadata = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	ip_(); //获取本机IP 
	string data_name_, user_name_;
	//data_name_ = day_() + ".txt"; //获取当前日期
	data_name_ = "Test_chat.txt";
	user_name_ = "Test_user.txt";
	data_name = (char*)data_name_.data();
	user_name = (char*)user_name_.data();
	pthread_mutex_init(&lock, NULL); //初始化锁变量
	for(int i=0; i<Max; i++){ //数据初始化(清空线程信息)
		infos[i].fd = -1;
		infos[i].tid = -1;
		memset(&key,0 ,sizeof(key));
		infos[i].message_listen = "|NULL|";
		infos[i].name = "|NULL|";
	}
	data_read();
	pthread_create(&tid_listen, NULL, th_accept, NULL); //创建监听线程
	pthread_detach(tid_listen); //分离线程
	pthread_create(&tid_send, NULL, th_send, NULL); //创建发送线程 
	pthread_detach(tid_send); //分离线程
	pthread_create(&tid_Dashboard, NULL, th_Dashboard, NULL); //创建仪表盘线程 
	pthread_join(tid_Dashboard, NULL); //等待仪表盘线程退出
	WSACleanup();
	return 0;
}
////////////////////////////////////////////////////////////////
void* th_accept(void* arg){ //-监听线程
	//-创建监听的套接字
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1){ //输出错误
		perror("socket");
		closesocket(fd);
		return NULL;
	}
	//-绑定本地IP port
	struct sockaddr_in addr; //服务端信息 
	addr.sin_family = AF_INET;
	addr.sin_port = htons(19999);
	addr.sin_addr.s_addr = INADDR_ANY; //0 = 0.0.0.0 
	int ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	if(ret == -1){ //输出错误
		perror("bind");
		closesocket(fd);
		return NULL;
	}
	//-设置监听
	ret = listen(fd, 128);
	if(ret == -1){ //输出错误
		perror("listen");
		closesocket(fd);
		return NULL;
	}
	//-开始监听
	int len = sizeof(struct sockaddr);
	struct sockaddr_in caddr; //客户端信息 
	struct in_addr ipaddr; //客户端IP 
	while(true){
		//-寻找空的子线程
		int key_;
		pthread_mutex_lock(&lock); //获取钥匙
		for(int i=0; i<Max; i++){
			if(infos[i].fd == -1){
				key_ = i;
				break;
			}
			if(i == Max-1){
				sleep(2);
				i = 0;
			}
		}
		pthread_mutex_unlock(&lock); //释放钥匙
		//-阻塞并等待客户端连接
		int cfd = accept(fd, (struct sockaddr*)&caddr, &len);
		if(cfd == -1){ //输出错误
			perror("accept");
			return NULL;
		}
		pthread_mutex_lock(&lock); //获取钥匙
		change_state = true;
		infos[key_].fd = cfd; //传入文件描述符
		ipaddr.s_addr = caddr.sin_addr.s_addr;
		infos[key_].ip = inet_ntoa(ipaddr); //传入IP
		infos[key_].port = ntohs(caddr.sin_port); //传入端口
		pthread_create(&key[key_], NULL, th_listen, NULL); //创建接收线程
        pthread_detach(key[key_]); //分离线程 
        pthread_mutex_unlock(&lock); //释放钥匙
	}
	closesocket(fd); //释放文件描述符
	return NULL; //线程结束
}
////////////////////////////////////////////////////////////////
void* th_listen(void* arg){ //-接收线程
	freopen("CON","r",stdin);
	cin.clear();
	bool name_ = false;
	char name1[9];
	memset(&name1, 0, sizeof(name1)); 
	//-查询线程ID对应客户端信息序号
	pthread_t th_t = pthread_self(); //获取线程ID
	int key_; //本线程对应客户端信息序号
	pthread_mutex_lock(&lock); //获取钥匙
	for(int i=0; i<Max; i++){ //寻找对应客户端信息序号
		if(pthread_equal(key[i], th_t) != 0){
			key_ = i;
			break;
		}
	}
	int cfd = infos[key_].fd; //获取文件描述符
	pthread_mutex_unlock(&lock); //释放钥匙
	while(true){ //接收数据
		int len = 0;
		char buf[256];
		memset(&buf, 0, sizeof(buf)); 
		if(name_ == false){
			len = recv(cfd, name1, sizeof(name1), 0); //传入用户昵称
			//read(cfd, NULL, 0);
		}
		else {
			len = recv(cfd, buf, sizeof(buf), 0);
		}
		if(len > 0){
			if(name_ == false){
				string fpm;
				pthread_mutex_lock(&lock); //获取钥匙
				change_state = true;
				infos[key_].name = name1;
				string temporary1, temporary2;
				stringstream temporary3;
				temporary3 << infos[key_].port;
				temporary3 >> temporary2;
				for(int t=0; t<11-infos[key_].name.size(); t++)temporary1 = temporary1 + ' ';
				fpm = time_()+"   系统信息 : "+infos[key_].name+temporary1+infos[key_].ip+":"+temporary2+" 上线了";
				write_(fpm, user_name);
				int num = tail_p;
				int t = 0;
				if(cycle == true){
					num = message_max;
					t = tail_p;
				}
				char num_[1];
				num_[0]=num;
				send(infos[key_].fd, num_, sizeof(num_), 0); //发送聊天记录数量 
				for(int i=0; i<num; i++){
					Sleep(100);
					send(infos[key_].fd, message[t], sizeof(message[t]), 0); //发送聊天消息
					t++;
					if(t == message_max)t = 0;
				}
				pthread_mutex_unlock(&lock); //释放钥匙
				name_ = true;
				continue;
			}
			while(true){
				pthread_mutex_lock(&lock); //获取钥匙
				if(infos[key_].message_listen == "|NULL|"){
					change_message = true;
					infos[key_].message_listen = buf;
					pthread_mutex_unlock(&lock); //释放钥匙
					break;
				}
				pthread_mutex_unlock(&lock); //释放钥匙
				sleep(2);
			}
		}
		else{
			pthread_mutex_lock(&lock); //获取钥匙
			change_state = true;
			pthread_mutex_unlock(&lock); //释放钥匙
			break; 
		}
		pthread_mutex_unlock(&lock); //释放钥匙
	}
	string fpm;
	pthread_mutex_lock(&lock); //获取钥匙
	if(infos[key_].name != "|NULL|"){
		string temporary1, temporary2;
		stringstream temporary3;
		temporary3 << infos[key_].port;
		temporary3 >> temporary2;
		for(int t=0; t<11-infos[key_].name.size(); t++)temporary1 = temporary1 + ' ';
		fpm = time_() + "   系统信息 : " + infos[key_].name + temporary1 + infos[key_].ip + ":" + temporary2 + " 离线了";
		write_(fpm, user_name);
	}
	change_state = true;
	infos[key_].fd = -1; //释放信息空间
	key[key_] = -1; //释放信息空间
	infos[key_].name = "|NULL|"; //释放信息空间 
	pthread_mutex_unlock(&lock); //释放钥匙
	closesocket(cfd); //释放文件描述符
	return NULL; //线程结束
}
////////////////////////////////////////////////////////////////
void* th_send(void* arg){ //发送(广播)线程 
	while(true){
		sleep(1);
		pthread_mutex_lock(&lock); //获取钥匙
		if(change_message){ //收到消息 
			change_message = false;
			for(int i=0; i<Max; i++){
				if(infos[i].message_listen != "|NULL|"){
					string temporary = "";
					for(int t=0; t<8-infos[i].name.size(); t++) temporary = temporary + ' ';
					string fpm;
					fpm = time_() + "  " + temporary + infos[i].name + ": " + infos[i].message_listen;
					write_(fpm, data_name);
					infos[i].message_listen = "|NULL|";
					char* send_message = (char*)fpm.data();
					sprintf(message[tail_p], send_message);
					for(int t=0; t<=Max; t++){
						if(user_order[t] == -1)break;
						send(infos[user_order[t]].fd, message[tail_p], sizeof(message[tail_p]), 0); //发送聊天消息
					}
					tail_p++;
					if(tail_p == message_max){
						tail_p = 0;
						cycle = true;
					}
				}
			}
		}
		pthread_mutex_unlock(&lock); //释放钥匙
	}
	return NULL; //线程结束
}
////////////////////////////////////////////////////////////////
void* th_Dashboard(void* arg){ //-仪表盘线程 
	bool have = false; 
	cout<<"信息墙服务端-V"<<version<<"  by繁星Stars"<<endl;
	cout<<"  服务器IP: "<<sip<<" (此IP为内网IP)"<<endl<<endl;
	cout<<"等待连接...";
	while(true){
		pthread_mutex_lock(&lock); //获取钥匙
		if(change_state){ //更新用户状态
			for(int i=0; i<Max; i++) user_order[i] = -1;
			have = true;
			change_state = false;
			int num = 0;
			system("cls");
			cout<<"信息墙服务端-V"<<version<<"  by繁星Stars"<<endl;
			cout<<"    服务器IP: "<<sip<<" (此IP为内网IP)"<<endl<<endl;
			cout<<"在线用户:"<<endl;
			for(int i=0; i<Max; i++){ //输出用户列表
				if(infos[i].fd != -1){
					user_order[num] = i;
					num++;
					cout<<num<<".";
					cout<<" "<<infos[i].name;
					for(int t=0; t<11-infos[i].name.size(); t++)cout<<" ";
					cout<<infos[i].ip<<":"<<infos[i].port<<endl;
				}
			}
			cout<<endl;
			//-重载入聊天记录 
			freopen(user_name,"r",stdin);
			string message;
			char k;
			while(true){
				k = getchar();
				if (k != EOF){
					getline(cin, message);
					message = k + message;
					cout<<message<<endl;
				}
				else break;
			}
			freopen("CON","r",stdin);
			cin.clear();
		}
		pthread_mutex_unlock(&lock); //释放钥匙
		sleep(1);
	}
	return NULL; //线程结束
}
////////////////////////////////////////////////////////////////
void ip_(){ //-获取本机IP函数 
	char hostname[256];
    gethostname(hostname, sizeof(hostname));
    struct hostent* host;
    host = gethostbyname(hostname);
    sip = inet_ntoa(*(struct in_addr*)*host->h_addr_list);
}
////////////////////////////////////////////////////////////////
string time_(){ //-获取时间函数 
	time_t t = time(0); 
	char temporary[32];
	memset(&temporary,0,sizeof(temporary));
	strftime(temporary, sizeof(temporary), "%Y-%m-%d %H:%M:%S",localtime(&t));
	string time_temporary = temporary;
	return time_temporary;
}
string day_(){ //-获取日期函数 
	time_t t = time(0); 
	char temporary[32];
	memset(&temporary,0,sizeof(temporary));
	strftime(temporary, sizeof(temporary), "%Y-%m-%d",localtime(&t));
	string time_temporary = temporary;
	return time_temporary;
}
////////////////////////////////////////////////////////////////
void write_(string fpm, char* data_){ //-文件写入函数 
	FILE *fp;
	char fp_m[300];
	strcpy(fp_m,fpm.c_str());
	fp = fopen(data_, "a");
	fprintf(fp, "%s\n", fp_m);
	fclose(fp);
}
////////////////////////////////////////////////////////////////
void data_read(){ //聊天记录读取函数 
	freopen(data_name,"r",stdin);
	string message_in;
	char* message_str_char;
	char k;
	while(true){
		memset(&message[tail_p],0,sizeof(message[tail_p]));
		k = getchar();
		if (k != EOF){
			getline(cin, message_in);
			message_in = k + message_in;
			message_str_char = (char*)message_in.data();
			sprintf(message[tail_p], message_str_char);
			tail_p++;
		}
		else break;
		if(tail_p == message_max){
			tail_p = 0;
			cycle = true;
		}
	}
	freopen("CON","r",stdin);
	cin.clear();
}
