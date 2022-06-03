#include<iostream>
#include<Winsock2.h>//socket头文件
#include<cstring>
#include<time.h>
#pragma comment(lib,"ws2_32.lib")   //socket库 //载入系统提供的socket动态链接库
using namespace std;

//==============================全局变量区===================================
const int BUFFER_SIZE = 1024;//缓冲区大小
int RECV_TIMEOUT = 10;//接收消息超时
int SEND_TIMEOUT = 10;//发送消息超时
const int WAIT_TIME = 10;//每个客户端等待事件的时间，单位毫秒
const int MAX_LINK_NUM = 15;//服务端最大链接数
SOCKET cliSock[MAX_LINK_NUM];//客户端套接字 0号为服务端
SOCKADDR_IN cliAddr[MAX_LINK_NUM];//客户端地址
WSAEVENT cliEvent[MAX_LINK_NUM];//客户端事件 0号为服务端,它用于让程序的一部分等待来自另一部分的信号。例如，当数据从套接字变为可用时，winsock 库会将事件设置为信号状态
int total = 0;//当前已经链接的客服端服务数 最多连接10个
struct Data //数据包
{
	int length;
	char receivemessage[1024]; //内容信息
	int fin;
};
Data DATA;

//==============================函数声明===================================
DWORD WINAPI servEventThread(LPVOID IpParameter);//服务器端处理线程
//DOUBLE WORD 双字即为4个字节，每个字节是8位，共32位 常用来保存地址(或者存放指针)。
//WINAPI：__stdcall  函数的参数被从右到左推送到堆栈上，被调用函数在返回之前从堆栈中弹出这些参数。
void clear();


int main()
{
	DATA.fin = 0;
	DATA.length = 0;
	DATA.receivemessage[0]='\0';
	//1、初始化socket库
	WSADATA wsaData;//获取版本信息，说明要使用的版本
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))//MAKEWORD(主版本号, 副版本号)
	{
		cout << "初始化网络失败 错误代号：" << GetLastError();
		return -1;
	}

	//2、创建socket
	SOCKET servSock = socket(AF_INET, SOCK_STREAM, 0);//面向网路的流式套接字 通信域、通信类型（字节流）、使用的协议
	if (servSock == INVALID_SOCKET)
	{
		printf("创建套接字失败.错误代号:%d\n", GetLastError());
		return -1;
	}

	//3、将服务器地址打包在一个结构体里面 
	SOCKADDR_IN servAddr; //sockaddr_in 是internet环境下套接字的地址形式
	servAddr.sin_family = AF_INET;//和服务器的socket一样，sin_family表示协议簇，一般用AF_INET表示TCP/IP协议。
	servAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//服务端地址设置为本地回环地址 任意ip地址  
	servAddr.sin_port = htons(52000);//host to net short（0->65535）htonl 通信端口：网络字节顺序


	//4、绑定服务端的socket和打包好的地址 即绑定套接字
	if (bind(servSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)//socket、结构体（端口、地址）、结构体大小
	{
		printf("绑定套接字失败.错误代号:%d\n", GetLastError());
		return -1;
	}

	//4.5给服务端sokect绑定一个事件对象，用来接收客户端链接的事件
	WSAEVENT servEvent = WSACreateEvent();//创建一个人工重设为传信的事件对象
	WSAEventSelect(servSock, servEvent, FD_ALL_EVENTS);//绑定事件对象，并且监听所有事件。将事件与某个套接字关联在一起，同时注册自己感兴趣的网络事件类型。
	cliSock[0] = servSock;
	cliEvent[0] = servEvent;

	//5、开启监听
	if (listen(servSock, 10) == SOCKET_ERROR)//监听队列长度为10
	{
		printf("监听失败.错误代号:%d\n", GetLastError());
		return -1;
	}

	//6、创建接受链接的线程

	//不需要句柄所以直接关闭
	//1.定子进程是否可以继承返回的句柄。如果 lpThreadAttributes为NULL，则无法继承句柄。2.堆栈的初始大小 3.指向由线程执行的应用程序定义函数的指针。该指针表示线程的起始地址
	//4.指向要传递给线程的变量的指针。5.该线程在创建后立即运行。 6.指向接收线程标识符的变量的指针。如果此参数为 NULL，则不返回线程标识符。
	CloseHandle(CreateThread(NULL, 0, servEventThread, (LPVOID)&servSock, 0, 0));//CloseHandle关闭线程：在进程终止运行时撤消线程
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clear, 0, 0, 0);
					//创建线程					//LPVOID是一个没有类型的指针，也就是说你可以将任意类型的指针赋值给LPVOID类型的变量（一般作为参数传递）
	//关闭了一个线程句柄，引用计数-1，表示 我不对这个句柄对应的线程做任何干预（如waitforsingleobject之类），但并没有结束线程。
	cout << "聊天室服务器已开启" << endl;
	//connect;
		//int addrLen = sizeof(SOCKADDR);//用于接收客户端的地址包结构体长度
		//SOCKET cliSOCK = accept(servSock, (SOCKADDR*)&servAddr,&addrLen);
		//if (cliSOCK != INVALID_SOCKET) 
		//{
		//	cout << "链接成功" << endl;
		//}
		//while (1)
		//{
		//	char buf[100] = { 0 };//测试缓冲区
		//	int nrecv = recv(cliSOCK, buf, sizeof(buf), 0);
		//	if (nrecv > 0)//如果接收到客户端的信息就输出到屏幕
		//	{
		//		cout << buf << endl;
		//	}
		//}
		//需要让主线程一直运行下去
		//发送消息给全部客户端
		while (1)
		{
			char contentBuf[BUFFER_SIZE] = { 0 };
			char sendBuf[BUFFER_SIZE] = { 0 };
			cin.getline(contentBuf, sizeof(contentBuf));//cin.getline(字符数组名，接收长度，结束符)可以将空格输入进去；（cin在遇到空格/tab时，就会停止读取）
			sprintf(sendBuf, "悠悠:%s", contentBuf);
			for (int j = 1; j <= total; j++)
			{
				send(cliSock[j], sendBuf, sizeof(sendBuf), 0);//传给每一个客户端  接收方、发送的内容、发送的长度、协议
			}
		}
	//1-关闭socket库的收尾工作
	closesocket(servSock);
	WSACleanup();
	return 0;
}

DWORD WINAPI servEventThread(LPVOID IpParameter) //服务器端线程
{
	//该线程负责处理服务端和各个客户端发生的事件
	//将传入的参数初始化
	SOCKET servSock = *(SOCKET*)IpParameter;//LPVOID为空指针类型，需要先转成SOCKET类型再引用，即可使用传入的SOCKET
	while (1) //不停执行	
	{
		for (int i = 0; i < total + 1; i++)//i代表现在正在监听事件的终端
		{
			//若有一个客户端链接，total==1，循环两次，包含客户端和服务端
			//对每一个终端（客户端和服务端），查看是否发生事件，等待WAIT_TIME毫秒 等待事件的触发
			//1 参数为事件的个数 2.事件数组，数组元素的类型为WSAEVENT类型 3.说明等一个事件，还是等所有的事件（false为前者）4.等待的时间 5.设为0 
			int index = WSAWaitForMultipleEvents(1, &cliEvent[i], false, WAIT_TIME, 0);//若成功，返回0 否则返回SOCKET_ERROR(-1)

			index -= WSA_WAIT_EVENT_0;//此时index为发生事件的终端下标   返回值需要减去WSA_WAIT_EVENT_0,才能得到真正的数组索引，但在windows,WSA_WAIT_EVENT_0=0

			if (index == WSA_WAIT_TIMEOUT || index == WSA_WAIT_FAILED)//index＝258L 或 超时
			{
				continue;//如果出错或者超时，即跳过此终端
			}

			else if (index == 0)//返回成功
			{
				WSANETWORKEVENTS networkEvents;
				//1.被触发的套接字句柄 需要被重置的Event 3.指向_WSANETWORKEVENTS 结构体指针，包含了到底是发生了何种网络事件，或者网络错误。
				WSAEnumNetworkEvents(cliSock[i], cliEvent[i], &networkEvents);//查看是什么事件 成功返回0

				//事件选择
				if (networkEvents.lNetworkEvents & FD_ACCEPT)//若产生accept事件（此处与位掩码相与） 
				{
					if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
					{
						cout << "连接时产生错误，错误代码" << networkEvents.iErrorCode[FD_ACCEPT_BIT] << endl;
						continue;
					}
					//接受链接
					if (total + 1 < MAX_LINK_NUM)//若增加一个客户端仍然小于最大连接数，则接受该链接
					{
						//total为已连接客户端数量
						int nextIndex = total + 1;//分配给新客户端的下标
						int addrLen = sizeof(SOCKADDR);
						//非阻塞式socket
						SOCKET newSock = accept(servSock, (SOCKADDR*)&cliAddr[nextIndex], &addrLen);//1.标识服务端套接字 2.保存客户端套接字对应的“地方” 3.“地方”占地大小
						if (newSock != INVALID_SOCKET)
						{
							//设置发送和接收时限
							/*setsockopt(newSock, SOL_SOCKET, SO_SNDTIMEO, (const char*) & SEND_TIMEOUT, sizeof(SEND_TIMEOUT));
							setsockopt(newSock, SOL_SOCKET, SO_SNDTIMEO, (const char*) &RECV_TIMEOUT, sizeof(RECV_TIMEOUT));*/
							//给新客户端分配socket
							cliSock[nextIndex] = newSock;
							//新客户端的地址已经存在cliAddr[nextIndex]中了
							//为新客户端绑定事件对象,同时设置监听，close，read，write
							WSAEVENT newEvent = WSACreateEvent();
							WSAEventSelect(cliSock[nextIndex], newEvent, FD_CLOSE | FD_READ | FD_WRITE);
							cliEvent[nextIndex] = newEvent;
							total++;//客户端连接数增加
							cout << "#" << nextIndex << "游客（IP：" << inet_ntoa(cliAddr[nextIndex].sin_addr) << ")进入了聊天室，当前连接数：" << total << endl;

							//给所有客户端发送消息
							char buf[BUFFER_SIZE] = "悠悠:欢迎游客（IP：";
							strcat(buf, inet_ntoa(cliAddr[nextIndex].sin_addr));
							strcat(buf, ")进入聊天室");
							for (int j = i; j <= total; j++)
							{
								send(cliSock[j], buf, sizeof(buf), 0);
							}
						}
					}
				}
				else if (networkEvents.lNetworkEvents & FD_CLOSE)//客户端被关闭，即断开连接
				{
					//i表示已关闭的客户端下标
					total--;
					cout << "#" << i << "游客（IP：" << inet_ntoa(cliAddr[i].sin_addr) << ")退出了聊天室,当前连接数：" << total << endl;
					//释放这个客户端的资源
					closesocket(cliSock[i]);
					WSACloseEvent(cliEvent[i]);

					//数组调整,用顺序表删除元素
					for (int j = i; j < total; j++)
					{
						cliSock[j] = cliSock[j + 1];//套接字
						cliEvent[j] = cliEvent[j + 1];//事件
						cliAddr[j] = cliAddr[j + 1];//地址
					}
					//给所有客户端发送退出聊天室的消息
					char buf[BUFFER_SIZE] = "悠悠:（IP：";
					strcat(buf, inet_ntoa(cliAddr[i].sin_addr));
					strcat(buf, ")退出聊天室");
					for (int j = 1; j <= total; j++)
					{
						send(cliSock[j], buf, sizeof(buf), 0);
					}
				}
				else if (networkEvents.lNetworkEvents & FD_READ)//接收到消息
				{
					char buffer[BUFFER_SIZE] = { 0 };//字符缓冲区，用于接收和发送消息
					char buffer2[BUFFER_SIZE] = { 0 };
					int flag = 1;
					for (int j = 1; j <= total; j++)
					{
						//if (flag == 0)//如果收到了图片，则跳出这次for循环
						//	break;
						int nrecv = recv(cliSock[j], buffer, sizeof(buffer), 0);///nrecv是接收到的字节数
						if (strcmp(buffer, "send a picture") == 0)//接收图片
						{
							FILE* fp = fopen("E:\\TEST.jpeg", "wb"); //以二进制方式打开(创建)文件
							if (fp == NULL)
							{
								printf("Cannot open file, press any key to exit!\n");
							}
							int nCount;
							while ((nCount = recv(cliSock[j], (char*)(&DATA), sizeof(DATA), 0)) > 0)//会阻塞
							{
								if (DATA.fin == 1)
								{
									fwrite(DATA.receivemessage, DATA.length, 1, fp);//最后一部分数据
									printf("文件保存在E:\\TEST.jpeg\n");
									break;
								}
								fwrite(DATA.receivemessage, DATA.length, 1, fp);
							}fclose(fp);
							getchar();
						}
						if (nrecv > 0)//如果接收到的字符数大于0
						{
							sprintf(buffer2, "#%d:%s", j, buffer);//写入buffer2中
							//在服务端显示
							cout << buffer2 << endl;
							//在其他客户端显示（广播给其他客户端） 包括发送方自己
							for (int k = 1; k <= total; k++)
							{
								send(cliSock[k], buffer2, sizeof(buffer), 0);
							}
						}
					}
				}
			}
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