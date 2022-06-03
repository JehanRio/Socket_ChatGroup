// 聊天程序客户端
#include<iostream>
#include<Winsock2.h>//socket头文件
#include<cstring>
#include<time.h>
#include <sys/types.h>
#include <stdlib.h>
#include<thread>
#include<string.h>
#pragma comment(lib,"ws2_32.lib")   //socket库
using namespace std;
//载入系统提供的socket动态链接库
const int BUFFER_SIZE = 1024;//缓冲区大小
time_t startt, endd;//计时20s

DWORD WINAPI recvMsgThread(LPVOID IpParameter);
void clear();
struct Data
{
	int length;
	char sendMessage[1024];
	int fin;
};
Data DATA;

int main() {
	DATA.fin = 0;
	DATA.length = 0;
	DATA.sendMessage[0] = '\0';
	//1、初始化socket库
	WSADATA wsaData;//获取版本信息，说明要使用的版本
	WSAStartup(MAKEWORD(2, 2), &wsaData);//MAKEWORD(主版本号, 副版本号)

	//2、创建socket
	SOCKET cliSock = socket(AF_INET, SOCK_STREAM, 0);//面向网路的流式套接字,第三个参数代表自动选择协议

	//3、打包地址
	//服务端
	SOCKADDR_IN servAddr = { 0 };
	servAddr.sin_family = AF_INET;//和服务器的socket一样，sin_family表示协议簇，一般用AF_INET表示TCP/IP协议。
	servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//服务端地址设置为本地回环地址
	servAddr.sin_port = htons(52000);//host to net short 端口号设置为52000
	//连接服务器
	if (connect(cliSock, (SOCKADDR*)&servAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		cout << "链接出现错误，错误代码" << WSAGetLastError() << endl;
	}
	//创建接受消息线程
	CloseHandle(CreateThread(NULL, 0, recvMsgThread, (LPVOID)&cliSock, 0, 0));
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clear, 0, 0, 0);
	//创建新的线程，第三个参数 lpStartAddress 表示新线程所执行的线程函数地址，多个线程可以使用同一个函数地址。
	//主线程用于输入要发送的消息
	while (1)
	{
		char buf[BUFFER_SIZE] = { 0 };
		cin.getline(buf, sizeof(buf));
		if (strcmp(buf, "quit") == 0)//若输入“quit”，则退出聊天室
		{
			break;
		}
		//if (strcmp(buf, "send a picture") == 0)//如果输入send a picture，则发送图片
		//{
		//	send(cliSock, buf, sizeof(buf), 0);
		//	Sleep(1000);
		//	FILE* fp = fopen("E:\\guitar.jpeg", "rb");
		//	if (fp == NULL)
		//	{
		//		printf("Cannot open file, press any key to exit!\n");
		//	}
		//	int nCount;
		//	while ((nCount = fread(DATA.sendMessage, 1, 1024, fp)) > 0)
		//	{
		//		if (nCount < 1024)
		//		{
		//			DATA.fin = 1; printf("图片发送完毕\n");
		//		}
		//		DATA.length = nCount;
		//		send(cliSock, (const char*)(&DATA), sizeof(DATA), 0);
		//		DATA.fin = 0;
		//		memset(DATA.sendMessage, '0', sizeof(DATA.sendMessage));
		//	}
		//	fclose(fp); 
		//}
		send(cliSock, buf, sizeof(buf), 0);
	}
	closesocket(cliSock);
	WSACleanup();
	return 0;
}

DWORD WINAPI recvMsgThread(LPVOID IpParameter)//接收消息的线程
{
	SOCKET cliSock = *(SOCKET*)IpParameter;//获取客户端的SOCKET参数

	while (1)
	{
		char buffer[BUFFER_SIZE] = { 0 };//字符缓冲区，用于接收和发送消息
		int nrecv = recv(cliSock, buffer, sizeof(buffer), 0);//nrecv是接收到的字节数 recv会一直等待，直到协议把数据接收完毕
		if (nrecv > 0)//如果接收到的字符数大于0
		{
			cout << buffer << endl;
		}
		else if (nrecv < 0)//如果接收到的字符数小于0就说明断开连接
		{
			cout << "与服务器断开连接" << endl;
			break;
		}
	}
	return 0;
}
void clear()
{
	time_t start, end;//计时
	start = time(0);
	while (1)
	{
		end = time(0);
		if (difftime(end, start) > 20)
		{
			system("cls");
			start = time(0);
		}
	}
}