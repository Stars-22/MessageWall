#include <bits/stdc++.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <conio.h>
//#pragma comment (lib, "ws2_32.dll")
using namespace std;
string version = "1.0.0"; //�汾�� 
void* th_listen(void* arg); //�����߳�-����
void* th_chat(void* arg); //�����߳�-���� 
string time_(); //��������-��ȡʱ��
void beginning(); //��������-������� 
void data_recv(); //��������-��ʼ�������¼ 
pthread_mutex_t lock; //����������
bool change_message = false; //�Ƿ���յ���Ϣ
char message_listen[100][300]; //�����¼ 
int pointer = 0; //�����¼ָ�� 
bool cycle = false; //�����¼ѭ�� 
int fd; //�׽����ļ������� 
bool quit = false; //�Ƿ��˳�
char IP[20]; //�����IP 
char name[8]; //�ͻ����ǳ� 
int main(){
	pthread_mutex_init(&lock, NULL); //��ʼ��������
	system("title ��Ϣǽ�ͻ���");
	WSADATA wsadata = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	char buf[256];
	memset(&buf, 0, sizeof(buf)); 
	//����ͨ�ŵ��׽���
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1){
		perror("socket");
		return -1;
	}
	//���ӷ�����IP port
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(19999);
	cout<<"��Ϣǽ�ͻ���-V"<<version<<"  by����Stars"<<endl;
	cout<<"  ������벻�����ģ���С��һ���ٴ�"<<endl<<endl;
	printf("�����������IP:");
	scanf("%s", &IP);
	saddr.sin_addr.s_addr = inet_addr(IP);
	printf("�ȴ�����ing...\n");
	//saddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //0 = 0.0.0.0
	while(true){
		int ret = connect(fd, (struct sockaddr*)&saddr, sizeof(saddr));
		if(ret != -1){
			break;
		}
	}
	cout<<"���ӳɹ�"<<endl<<endl; 
	//�������� 
	int name_size;
	while(true){
		cout<<"��Ϊ�Լ������ǳ�(��8�ַ���8�ַ����ڣ�����ռ2�ַ����س�ȷ��): ";
		cin>>buf;
		if(buf[8] == 0){
			sprintf(name, buf);
			send(fd, name, sizeof(name), 0);
			break;
		} 
		cout<<"��ǰ���벻�����ǳ�����"<<endl; 
	}
	string blank = "";
	for(int i=7;i>0;i--){
		blank = blank + ' ';
		if(name[i] != 0)break;
	}
	cout<<"������Ϣ��..."; 
	data_recv(); //��ȡ�����¼
	//�����߳�-����
	pthread_t tid_listen; //�����߳�ID 
	pthread_create(&tid_listen, NULL, th_listen, NULL); //���������߳�
	pthread_detach(tid_listen); //�����߳� 
	//�����߳�-���� 
	pthread_t tid_chat; //�����߳�ID 
	pthread_create(&tid_chat, NULL, th_chat, NULL); //���������߳�
	pthread_join(tid_chat, NULL); //�ȴ������߳��˳�
	//�ر��ļ������� 
	closesocket(fd);
	system("pause");
	return 0;
}

void* th_listen(void* arg){ //-�����߳�
	int len = 0;
	while(true){
		while(true){
			char message[300];
			memset(&message, 0, sizeof(message));
			len = recv(fd, message, sizeof(message), 0);
			pthread_mutex_lock(&lock); //��ȡԿ��
			memset(&message_listen[pointer], 0, sizeof(message_listen[pointer]));
			sprintf(message_listen[pointer], message);
			change_message = true;
			pointer++;
			if(pointer == 100){
				cycle = true;
				pointer = 0;
			}
			pthread_mutex_unlock(&lock); //�ͷ�Կ��
			break;
		}
		if(quit) return NULL; //�߳̽���
		if(len == 0){
			pthread_mutex_lock(&lock); //��ȡԿ��
			quit = true;
			pthread_mutex_unlock(&lock); //�ͷ�Կ��
			printf("\n�������Ѿ��Ͽ�������...\n");
			return NULL; //�߳̽���
		}
		if(len == -1 && !quit){
			pthread_mutex_lock(&lock); //��ȡԿ��
			quit = true;
			pthread_mutex_unlock(&lock); //�ͷ�Կ��
			perror("recv");
			return NULL; //�߳̽���
		}
	}
	return NULL; //�߳̽���
}
////////////////////////////////////////////////////////////////
void* th_chat(void* arg){ //-�����߳� 
	char buf[256];
	//�����ʼ��
	system("cls");
	pthread_mutex_lock(&lock); //��ȡԿ��
	beginning();
	pthread_mutex_unlock(&lock); //�ͷ�Կ��
	//ͨ��
	while(true){
		memset(&buf, 0, sizeof(buf));
		Sleep(100);
		//��������
		if(kbhit()){
			char k = getch();
			if(k != ' ')continue;
			cout<<"��������Ҫ���͵���Ϣ(��256��������ESCȡ��): ";
			cin>>buf;
			if(buf[0] == 'E' && buf[1] == 'S' && buf[2] == 'C'){
				cout<<"��ȡ��"<<endl;
				change_message = true;
				continue;
			}
			if(buf[0] == 'E' && buf[1] == 'X' && buf[2] == 'I' && buf[3] == 'T'){
				pthread_mutex_lock(&lock); //��ȡԿ��
				quit = true;
				pthread_mutex_unlock(&lock); //�ͷ�Կ��
				break;
			}
			pthread_mutex_lock(&lock); //��ȡԿ��
			send(fd, buf, sizeof(buf), 0);
			change_message = true;
			pthread_mutex_unlock(&lock); //�ͷ�Կ��
			continue;
		}
		if(quit)break;
		pthread_mutex_lock(&lock); //��ȡԿ��
		if(change_message){
			system("cls");
			beginning();
			int num = pointer;
			int t = 0;
			if(cycle == true){
				num = 100;
				t = pointer;
			}
			for(int i=0; i<num; i++){
				cout<<message_listen[t]<<endl;
				t++;
				if(t == 100) t = 0;
			}
			change_message = false;
		}
		pthread_mutex_unlock(&lock); //�ͷ�Կ��
	}
	return NULL; //�߳̽���
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

void beginning(){ //-����������� 
	cout<<"��Ϣǽ�ͻ���-V"<<version<<"  by����Stars"<<endl;
	cout<<"  ������벻�����ģ���С��һ���ٴ�"<<endl;
	cout<<"  ���ո����ʼ������Ϣ���س�ȷ��"<<endl;
	cout<<"  ����EXIT��Ϣ���˳��ͻ���"<<endl;
	cout<<"    ������IP: "<<IP<<endl;
	cout<<"    ��ǰ�ǳ�: "<<name<<endl<<endl;
}

void data_recv(){ //-��ʼ�������¼���� 
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
		if(pointer == 100){
			cycle = true;
			pointer = 0;
		}
	}
}
