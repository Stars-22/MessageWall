#include <bits/stdc++.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <conio.h>
//#pragma comment (lib, "ws2_32.dll")
using namespace std;
string version = "1.0.1"; //�汾��
const int Max = 64; //�ͻ������������
int message_max = 200; //��󻺴������¼ 
char message[200][300]; //�����¼(����Ϊǰ[])
void* th_accept(void* arg); //�����߳�-����
void* th_listen(void* arg); //�����߳�-����
void* th_send(void* arg); //�����߳�-����(�㲥)
void* th_Dashboard(void* arg); //�����߳�-�Ǳ���
void ip_(); //����������-ȡ������IP
string time_(); //��������-��ȡʱ��
string day_(); //��������-��ȡ����
void write_(string fpm, char* data_); //��������-�ļ�д��
void data_read(); //��������-�����¼��ȡ 
////////////////////////////////////////////////////////////////
pthread_mutex_t lock; //����������
pthread_t tid_listen; //�����߳�ID 
pthread_t tid_send; //�����߳�ID 
pthread_t tid_Dashboard; //�Ǳ����߳�ID 
//-�����ͻ�����Ϣ�ṹ��
struct SockInfo{ //�ͻ�����Ϣ�ṹ��
    int fd; //�ļ�������
    pthread_t tid; //�߳�ID
    string ip; //�ͻ���IP
    int port; //�ͻ��˶˿�
    string message_listen; //���յ���Ϣ
    string name; //�ͻ����ǳ�
};
struct SockInfo infos[Max]; //�ͻ�����Ϣ��
pthread_t key[Max]; //�ͻ��˶�Ӧ�߳�ID
char* sip; //������IP
char* data_name; //�����¼�ļ��� 
char* user_name; //�û���¼�ļ��� 
bool change_state = false; //�û�״̬�Ƿ���Ҫ���� 
bool change_message = false; //�����¼�Ƿ���Ҫ���� 
int user_order[Max]; //�û����� 
int tail_p = 0; //�����¼ָ�� 
bool cycle = false; //�����¼ѭ�� 
////////////////////////////////////////////////////////////////
int main(){
	system("title ��Ϣǽ�����");
	WSADATA wsadata = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	ip_(); //��ȡ����IP 
	string data_name_, user_name_;
	//data_name_ = day_() + ".txt"; //��ȡ��ǰ����
	data_name_ = "Test_chat.txt";
	user_name_ = "Test_user.txt";
	data_name = (char*)data_name_.data();
	user_name = (char*)user_name_.data();
	pthread_mutex_init(&lock, NULL); //��ʼ��������
	for(int i=0; i<Max; i++){ //���ݳ�ʼ��(����߳���Ϣ)
		infos[i].fd = -1;
		infos[i].tid = -1;
		memset(&key,0 ,sizeof(key));
		infos[i].message_listen = "|NULL|";
		infos[i].name = "|NULL|";
	}
	data_read();
	pthread_create(&tid_listen, NULL, th_accept, NULL); //���������߳�
	pthread_detach(tid_listen); //�����߳�
	pthread_create(&tid_send, NULL, th_send, NULL); //���������߳� 
	pthread_detach(tid_send); //�����߳�
	pthread_create(&tid_Dashboard, NULL, th_Dashboard, NULL); //�����Ǳ����߳� 
	pthread_join(tid_Dashboard, NULL); //�ȴ��Ǳ����߳��˳�
	WSACleanup();
	return 0;
}
////////////////////////////////////////////////////////////////
void* th_accept(void* arg){ //-�����߳�
	//-�����������׽���
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1){ //�������
		perror("socket");
		closesocket(fd);
		return NULL;
	}
	//-�󶨱���IP port
	struct sockaddr_in addr; //�������Ϣ 
	addr.sin_family = AF_INET;
	addr.sin_port = htons(19999);
	addr.sin_addr.s_addr = INADDR_ANY; //0 = 0.0.0.0 
	int ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	if(ret == -1){ //�������
		perror("bind");
		closesocket(fd);
		return NULL;
	}
	//-���ü���
	ret = listen(fd, 128);
	if(ret == -1){ //�������
		perror("listen");
		closesocket(fd);
		return NULL;
	}
	//-��ʼ����
	int len = sizeof(struct sockaddr);
	struct sockaddr_in caddr; //�ͻ�����Ϣ 
	struct in_addr ipaddr; //�ͻ���IP 
	while(true){
		//-Ѱ�ҿյ����߳�
		int key_;
		pthread_mutex_lock(&lock); //��ȡԿ��
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
		pthread_mutex_unlock(&lock); //�ͷ�Կ��
		//-�������ȴ��ͻ�������
		int cfd = accept(fd, (struct sockaddr*)&caddr, &len);
		if(cfd == -1){ //�������
			perror("accept");
			return NULL;
		}
		pthread_mutex_lock(&lock); //��ȡԿ��
		change_state = true;
		infos[key_].fd = cfd; //�����ļ�������
		ipaddr.s_addr = caddr.sin_addr.s_addr;
		infos[key_].ip = inet_ntoa(ipaddr); //����IP
		infos[key_].port = ntohs(caddr.sin_port); //����˿�
		pthread_create(&key[key_], NULL, th_listen, NULL); //���������߳�
        pthread_detach(key[key_]); //�����߳� 
        pthread_mutex_unlock(&lock); //�ͷ�Կ��
	}
	closesocket(fd); //�ͷ��ļ�������
	return NULL; //�߳̽���
}
////////////////////////////////////////////////////////////////
void* th_listen(void* arg){ //-�����߳�
	freopen("CON","r",stdin);
	cin.clear();
	bool name_ = false;
	char name1[9];
	memset(&name1, 0, sizeof(name1)); 
	//-��ѯ�߳�ID��Ӧ�ͻ�����Ϣ���
	pthread_t th_t = pthread_self(); //��ȡ�߳�ID
	int key_; //���̶߳�Ӧ�ͻ�����Ϣ���
	pthread_mutex_lock(&lock); //��ȡԿ��
	for(int i=0; i<Max; i++){ //Ѱ�Ҷ�Ӧ�ͻ�����Ϣ���
		if(pthread_equal(key[i], th_t) != 0){
			key_ = i;
			break;
		}
	}
	int cfd = infos[key_].fd; //��ȡ�ļ�������
	pthread_mutex_unlock(&lock); //�ͷ�Կ��
	while(true){ //��������
		int len = 0;
		char buf[256];
		memset(&buf, 0, sizeof(buf)); 
		if(name_ == false){
			len = recv(cfd, name1, sizeof(name1), 0); //�����û��ǳ�
			//read(cfd, NULL, 0);
		}
		else {
			len = recv(cfd, buf, sizeof(buf), 0);
		}
		if(len > 0){
			if(name_ == false){
				string fpm;
				pthread_mutex_lock(&lock); //��ȡԿ��
				change_state = true;
				infos[key_].name = name1;
				string temporary1, temporary2;
				stringstream temporary3;
				temporary3 << infos[key_].port;
				temporary3 >> temporary2;
				for(int t=0; t<11-infos[key_].name.size(); t++)temporary1 = temporary1 + ' ';
				fpm = time_()+"   ϵͳ��Ϣ : "+infos[key_].name+temporary1+infos[key_].ip+":"+temporary2+" ������";
				write_(fpm, user_name);
				int num = tail_p;
				int t = 0;
				if(cycle == true){
					num = message_max;
					t = tail_p;
				}
				char num_[1];
				num_[0]=num;
				send(infos[key_].fd, num_, sizeof(num_), 0); //���������¼���� 
				for(int i=0; i<num; i++){
					Sleep(100);
					send(infos[key_].fd, message[t], sizeof(message[t]), 0); //����������Ϣ
					t++;
					if(t == message_max)t = 0;
				}
				pthread_mutex_unlock(&lock); //�ͷ�Կ��
				name_ = true;
				continue;
			}
			while(true){
				pthread_mutex_lock(&lock); //��ȡԿ��
				if(infos[key_].message_listen == "|NULL|"){
					change_message = true;
					infos[key_].message_listen = buf;
					pthread_mutex_unlock(&lock); //�ͷ�Կ��
					break;
				}
				pthread_mutex_unlock(&lock); //�ͷ�Կ��
				sleep(2);
			}
		}
		else{
			pthread_mutex_lock(&lock); //��ȡԿ��
			change_state = true;
			pthread_mutex_unlock(&lock); //�ͷ�Կ��
			break; 
		}
		pthread_mutex_unlock(&lock); //�ͷ�Կ��
	}
	string fpm;
	pthread_mutex_lock(&lock); //��ȡԿ��
	if(infos[key_].name != "|NULL|"){
		string temporary1, temporary2;
		stringstream temporary3;
		temporary3 << infos[key_].port;
		temporary3 >> temporary2;
		for(int t=0; t<11-infos[key_].name.size(); t++)temporary1 = temporary1 + ' ';
		fpm = time_() + "   ϵͳ��Ϣ : " + infos[key_].name + temporary1 + infos[key_].ip + ":" + temporary2 + " ������";
		write_(fpm, user_name);
	}
	change_state = true;
	infos[key_].fd = -1; //�ͷ���Ϣ�ռ�
	key[key_] = -1; //�ͷ���Ϣ�ռ�
	infos[key_].name = "|NULL|"; //�ͷ���Ϣ�ռ� 
	pthread_mutex_unlock(&lock); //�ͷ�Կ��
	closesocket(cfd); //�ͷ��ļ�������
	return NULL; //�߳̽���
}
////////////////////////////////////////////////////////////////
void* th_send(void* arg){ //����(�㲥)�߳� 
	while(true){
		sleep(1);
		pthread_mutex_lock(&lock); //��ȡԿ��
		if(change_message){ //�յ���Ϣ 
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
						send(infos[user_order[t]].fd, message[tail_p], sizeof(message[tail_p]), 0); //����������Ϣ
					}
					tail_p++;
					if(tail_p == message_max){
						tail_p = 0;
						cycle = true;
					}
				}
			}
		}
		pthread_mutex_unlock(&lock); //�ͷ�Կ��
	}
	return NULL; //�߳̽���
}
////////////////////////////////////////////////////////////////
void* th_Dashboard(void* arg){ //-�Ǳ����߳� 
	bool have = false; 
	cout<<"��Ϣǽ�����-V"<<version<<"  by����Stars"<<endl;
	cout<<"  ������IP: "<<sip<<" (��IPΪ����IP)"<<endl<<endl;
	cout<<"�ȴ�����...";
	while(true){
		pthread_mutex_lock(&lock); //��ȡԿ��
		if(change_state){ //�����û�״̬
			for(int i=0; i<Max; i++) user_order[i] = -1;
			have = true;
			change_state = false;
			int num = 0;
			system("cls");
			cout<<"��Ϣǽ�����-V"<<version<<"  by����Stars"<<endl;
			cout<<"    ������IP: "<<sip<<" (��IPΪ����IP)"<<endl<<endl;
			cout<<"�����û�:"<<endl;
			for(int i=0; i<Max; i++){ //����û��б�
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
			//-�����������¼ 
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
		pthread_mutex_unlock(&lock); //�ͷ�Կ��
		sleep(1);
	}
	return NULL; //�߳̽���
}
////////////////////////////////////////////////////////////////
void ip_(){ //-��ȡ����IP���� 
	char hostname[256];
    gethostname(hostname, sizeof(hostname));
    struct hostent* host;
    host = gethostbyname(hostname);
    sip = inet_ntoa(*(struct in_addr*)*host->h_addr_list);
}
////////////////////////////////////////////////////////////////
string time_(){ //-��ȡʱ�亯�� 
	time_t t = time(0); 
	char temporary[32];
	memset(&temporary,0,sizeof(temporary));
	strftime(temporary, sizeof(temporary), "%Y-%m-%d %H:%M:%S",localtime(&t));
	string time_temporary = temporary;
	return time_temporary;
}
string day_(){ //-��ȡ���ں��� 
	time_t t = time(0); 
	char temporary[32];
	memset(&temporary,0,sizeof(temporary));
	strftime(temporary, sizeof(temporary), "%Y-%m-%d",localtime(&t));
	string time_temporary = temporary;
	return time_temporary;
}
////////////////////////////////////////////////////////////////
void write_(string fpm, char* data_){ //-�ļ�д�뺯�� 
	FILE *fp;
	char fp_m[300];
	strcpy(fp_m,fpm.c_str());
	fp = fopen(data_, "a");
	fprintf(fp, "%s\n", fp_m);
	fclose(fp);
}
////////////////////////////////////////////////////////////////
void data_read(){ //�����¼��ȡ���� 
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
