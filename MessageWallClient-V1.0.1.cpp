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
int message_max = 200; //最大缓存聊天记录
char message_listen[200][300]; //聊天记录(上限为前[])
void* th_listen(void* arg); //声明线程-接收
void* th_chat(void* arg); //声明线程-聊天 
string time_(); //声明函数-获取时间
void beginning(); //声明函数-开端输出 
void data_recv(); //声明函数-初始化聊天记录 
pthread_mutex_t lock; //声明互斥锁
bool change_message = false; //是否接收到消息
int pointer = 0; //聊天记录指针 
bool cycle = false; //聊天记录循环 
int fd; //套接字文件描述符 
bool quit = false; //是否退出
char IP[20]; //服务端IP 
char name[8]; //客户端昵称 
int main(){
	pthread_mutex_init(&lock, NULL); //初始化锁变量
	system("title 信息墙客户端");
	WSADATA wsadata = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	char buf[256];
	memset(&buf, 0, sizeof(buf)); 
	//创建通信的套接字
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1){
		perror("socket");
		return -1;
	}
	//连接服务器IP port
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(19999);
	cout<<"信息墙客户端-V"<<version<<"  by繁星Stars"<<endl;
	cout<<"  如果输入不了中文，最小化一下再打开"<<endl<<endl;
	printf("请输入服务器IP:");
	scanf("%s", &IP);
	saddr.sin_addr.s_addr = inet_addr(IP);
	printf("等待连接ing...\n");
	//saddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //0 = 0.0.0.0
	while(true){
		int ret = connect(fd, (struct sockaddr*)&saddr, sizeof(saddr));
		if(ret != -1){
			break;
		}
	}
	cout<<"连接成功"<<endl<<endl; 
	//设置名字 
	int name_size;
	FILE *name_data = freopen("MessageWall_username.txt", "r", stdin);
	if(name_data == NULL){
		freopen("CON","r",stdin);
		cin.clear();
		while(true){
			cout<<"请为自己设置昵称(限8字符或8字符以内，汉字占2字符，回车确定): ";
			cin>>buf;
			if(buf[8] == 0){
				FILE *fp;
				fp = fopen("MessageWall_username.txt", "w");
				fprintf(fp, "%s\n", buf);
				fclose(fp);
				sprintf(name, buf);
				send(fd, name, sizeof(name), 0);
					break;
			} 
			cout<<"当前输入不符合昵称限制"<<endl; 
		}
	}
	else{
		cin>>buf;
		sprintf(name, buf);
		send(fd, name, sizeof(name), 0);
		freopen("CON","r",stdin);
		cin.clear();
	}
	string blank = "";
	for(int i=7;i>0;i--){
		blank = blank + ' ';
		if(name[i] != 0)break;
	}
	cout<<"接收消息中..."; 
	data_recv(); //获取聊天记录
	cout<<"接收完成"<<endl;
	system("cls");
	beginning();
	cout<<"请仔细阅读以上使用说明"<<endl<<endl;
	system("pasue"); 
	//创建线程-接收
	pthread_t tid_listen; //监听线程ID 
	pthread_create(&tid_listen, NULL, th_listen, NULL); //创建监听线程
	pthread_detach(tid_listen); //分离线程 
	//创建线程-聊天 
	pthread_t tid_chat; //聊天线程ID 
	pthread_create(&tid_chat, NULL, th_chat, NULL); //创建聊天线程
	pthread_join(tid_chat, NULL); //等待聊天线程退出
	//关闭文件描述符 
	closesocket(fd);
	system("pause");
	return 0;
}

void* th_listen(void* arg){ //-接收线程
	int len = 0;
	while(true){
		while(true){
			char message[300];
			memset(&message, 0, sizeof(message));
			len = recv(fd, message, sizeof(message), 0);
			pthread_mutex_lock(&lock); //获取钥匙
			memset(&message_listen[pointer], 0, sizeof(message_listen[pointer]));
			sprintf(message_listen[pointer], message);
			change_message = true;
			pointer++;
			if(pointer == message_max){
				cycle = true;
				pointer = 0;
			}
			pthread_mutex_unlock(&lock); //释放钥匙
			break;
		}
		if(quit) return NULL; //线程结束
		if(len == 0){
			pthread_mutex_lock(&lock); //获取钥匙
			quit = true;
			pthread_mutex_unlock(&lock); //释放钥匙
			printf("\n服务器已经断开了连接...\n");
			return NULL; //线程结束
		}
		if(len == -1 && !quit){
			pthread_mutex_lock(&lock); //获取钥匙
			quit = true;
			pthread_mutex_unlock(&lock); //释放钥匙
			perror("recv");
			return NULL; //线程结束
		}
	}
	return NULL; //线程结束
}
////////////////////////////////////////////////////////////////
void* th_chat(void* arg){ //-聊天线程 
	char buf[256];
	//界面初始化
	system("cls");
	pthread_mutex_lock(&lock); //获取钥匙
	beginning();
	pthread_mutex_unlock(&lock); //释放钥匙
	//通信
	while(true){
		memset(&buf, 0, sizeof(buf));
		Sleep(100);
		//发送数据
		if(kbhit()){
			char k = getch();
			if(k != ' ')continue;
			cout<<"请输入想要发送的信息(限256符，输入esc取消): ";
			cin>>buf;
			if(buf[0] == 'e' && buf[1] == 's' && buf[2] == 'c'){
				cout<<"已取消"<<endl;
				change_message = true;
				continue;
			}
			if(buf[0] == 'e' && buf[1] == 'x' && buf[2] == 'i' && buf[3] == 't'){
				pthread_mutex_lock(&lock); //获取钥匙
				quit = true;
				pthread_mutex_unlock(&lock); //释放钥匙
				break;
			}
			if(buf[0] == 'c' && buf[1] == 'l' && buf[2] == 's'){
				pthread_mutex_lock(&lock); //获取钥匙
				pointer = 0;
				cycle = false;
				memset(&message_listen, 0, sizeof(message_listen));
				pthread_mutex_unlock(&lock); //释放钥匙
			}
			pthread_mutex_lock(&lock); //获取钥匙
			send(fd, buf, sizeof(buf), 0);
			change_message = true;
			pthread_mutex_unlock(&lock); //释放钥匙
			continue;
		}
		if(quit)break;
		pthread_mutex_lock(&lock); //获取钥匙
		if(change_message){
			system("cls");
			beginning();
			int num = pointer;
			int t = 0;
			if(cycle == true){
				num = message_max;
				t = pointer;
			}
			for(int i=0; i<num; i++){
				cout<<message_listen[t]<<endl;
				t++;
				if(t == message_max) t = 0;
			}
			change_message = false;
		}
		pthread_mutex_unlock(&lock); //释放钥匙
	}
	return NULL; //线程结束
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

void beginning(){ //-开端输出函数 
	cout<<"信息墙客户端-V"<<version<<"  by繁星Stars"<<endl;
	cout<<"  如果输入不了中文，最小化一下再打开"<<endl;
	cout<<"  按空格键开始发送消息，回车确定"<<endl;
	cout<<"  发送exit消息(实际没发出去)则退出客户端"<<endl;
	cout<<"  发送cls消息(实际没发出去)则清空聊天记录(重启程序将重新加载)"<<endl;
	cout<<"    服务器IP: "<<IP<<endl;
	cout<<"    当前昵称: "<<name<<endl<<endl;
}

void data_recv(){ //-初始化聊天记录函数 
	char num_[1];
	recv(fd, num_, sizeof(num_), 0);
	int num = num_[0];
	char message[300];
	for(int i=0; i<num; i++){
		memset(&message, 0, sizeof(message));
		recv(fd, message, sizeof(message), 0);
		memset(&message_listen[pointer], 0, sizeof(message_listen[pointer]));
		sprintf(message_listen[pointer], message);
		change_message = true;
		pointer++;
		if(pointer == message_max){
			cycle = true;
			pointer = 0;
		}
	}
}
