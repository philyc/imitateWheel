#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include "sha1.h"
#include "base64.h"
#pragma comment(lib,"ws2_32.lib")

using namespace std;

void getKey(char* request, string clientKey)
{
	strcat(request, "HTTP/1.1 101 Switching Protocols\r\n");
	strcat(request, "Connection: upgrade\r\n");
	strcat(request, "Sec-WebSocket-Accept: ");
	string server_key = clientKey;
	server_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	SHA1 sha;
	unsigned int message_digest[5];
	cout << "server_key:" << server_key << endl;
	sha.Reset();
	sha << server_key.c_str();
	sha.Result(message_digest);
	for (int i = 0; i < 5; i++)
	{
		message_digest[i] = htonl(message_digest[i]);
	}
	base64 base;
	server_key = base.base64_encode(reinterpret_cast<const unsigned char*>(message_digest), 20);
	cout << "server_key的长度" << server_key.size() << endl;
	server_key += "\r\n";
	strcat(request, server_key.c_str());
	//strcat(request, "Server:beetle websocket server\r\n");
	//strcat(request, "Access-Control-Allow-Credentials:true\r\n");
	//strcat(request, "Access-Control-Allow-Headers:content-type\r\n");
	strcat(request, "Upgrade: websocket\r\n\r\n");
	//strcat(request, "Sec-WebSocket-Protocol:chat\r\n\r\n");
	cout << "shalserver_key:" << server_key << endl;
}

void respondClient(SOCKET sockClient, byte charb[], int length, boolean finalFragment)
{
	byte buf[100] = "";
	int first = 0x00;
	cout << "first:" << first << endl;
	int tmp = 0;
	if (finalFragment) {
		first = first + 0x80;
		cout << "first:" << first << endl;
		first = first + 0x1;
		cout << "first:" << first << endl;
	}
	buf[0] = first;
	tmp = 1;
	cout << ">>>>>>>>>>>>>>>>>>>>>>>数组长度:" << length << endl;
	unsigned int nuNum = (unsigned)length;
	if (length < 126) {
		buf[1] = length;
		tmp = 2;
	}
	else if (length < 65536) {
		buf[1] = 126;
		buf[2] = nuNum >> 8;
		buf[3] = length & 0xFF;
		tmp = 4;
	}
	else {
		buf[1] = 127;
		buf[2] = 0;
		buf[3] = 0;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = nuNum >> 24;
		buf[7] = nuNum >> 16;
		buf[8] = nuNum >> 8;
		buf[9] = nuNum & 0xFF;
		tmp = 10;
	}
	cout << "tmp:" << tmp << endl;
	for (int i = 0; i < length; i++) {
		buf[tmp + i] = charb[i];
		printf("要发送的数据字节：%d\n", charb[i]);
	}
	char charbuf[100] = "";
	memcpy(charbuf, buf, length + tmp);
	send(sockClient, charbuf, 100, 0);
}

/*获取key和协议*/
void requestInfo(SOCKET sockClient, char* request) {
	char revData[1024]; //存储接受的数据
	int ret = recv(sockClient, revData, 2048, 0);
	revData[ret] = '\0';
	string revDataString = revData;
	string getString = "GET";
	string::size_type idx;
	idx = revDataString.find(getString);//在a中查找b.
	if (idx == string::npos)//不存在。
		cout << "当前不是握手协议, 握手失败";
	else//存在。
		cout << "当前是握手协议, 开始握手" << endl;
	cout << "revDataString-->" << revDataString.c_str() << endl;
	// 查找协议中的Sec-WebSocket-Key 字段
	size_t index = revDataString.find("Sec-WebSocket-Key");
	if (index > 0) {
		// 截取 Sec-WebSocket-Key 字段的内容
		string secWebSocketKeyString = revDataString.substr(index + 19, 24);
		char request[1024] = "";
		// 进行Sec-WebSocket-Key字段加密, 加密后返回客户端, 完成握手动作
		getKey(request, secWebSocketKeyString);
	}
	else {
		cout << "当前协议中不存在Sec-WebSocket-Key字段, 握手失败" << endl;
	}
}

/*发送协议*/
void respondInfo(SOCKET sockClient, char* request) {
	send(sockClient, request, (int)strlen(request), 0);
}

void getClientInfo(SOCKET sClient, char clieninfo[])
{
	int point = 0;            //字节指针位置
	int tmppoint = 0;         //临时指针变量
	byte b[4096] = "";

	memcpy(b, clieninfo, 2048);
	cout << "字节长度: " << end(b) - begin(b) << endl;
	for (int i = 0; i < 30; i++) {
		cout << "字节" << i << ":" << b[i] << endl;
	}
	for (int i = 0; i <= 10; i++) {
		printf("%d\t", b[i]);
	}
	printf("\n");

	//解析websocket消息
	// 
	//取第一个字节
	cout << "第一个字节:" << b[point] << endl;
	int first = b[point] & 0xFF;
	printf("第一个：%d,%d,%d\n", point, b[point], first);
	byte opCode = (byte)(first & 0x0F);             //0000 1111 后四位为opCode 00001111
	if (opCode == 8) {
		cout << "关闭" << endl;
		cout << "opCode:" << opCode << endl;
		closesocket(sClient);
	}

	//取第二个字节
	first = b[++point];
	//负载长度
	int payloadLength = first & 0x7F;
	printf("第二个：%d,[%d],%d\n", point, b[point], payloadLength);
	if (payloadLength == 126) {
		byte extended[2] = "";
		extended[0] = b[++point];
		extended[1] = b[++point];
		int shift = 0;
		payloadLength = 0;
		for (int i = 2 - 1; i >= 0; i--) {
			payloadLength = payloadLength + ((extended[i] & 0xFF) << shift);
			shift += 8;
		}
	}
	else if (payloadLength == 127) {
		byte extended[8] = "";
		tmppoint = ++point;     //保存临时指针
		point = --point;
		for (int i = 0; i < 8; i++) {
			extended[i] = b[tmppoint + i];
			point++;
		}
		int shift = 0;
		payloadLength = 0;
		for (int i = 8 - 1; i >= 0; i--) {
			payloadLength = payloadLength + ((extended[i] & 0xFF) << shift);
			shift += 8;
		}
	}
	//非126和127置回来
	if ((payloadLength != 126) || (payloadLength != 127)) {
		point = 1;
	}

	cout << "负载长度:" << payloadLength << endl;

	//第三个字节，掩码
	byte mask[4] = "";
	tmppoint = ++point;
	//因为自增了一次，这里需要减掉
	point = --point;
	//取掩码值
	for (int i = 0; i < 4; i++) {
		mask[i] = b[tmppoint + i];
		point++;
		printf("第三mask个：%d,[%d],%d\t\n", point, mask[i], payloadLength);
	}
	byte changeb[4096] = "";

	//内容的长度保留，循环里面已经被改变
	int length = payloadLength;
	int readThisFragment = 1;
	//通过掩码计算真实的数据
	while (payloadLength > 0) {
		int maskbyte = b[++point];
		int index = (readThisFragment - 1) % 4;
		maskbyte = maskbyte ^ (mask[index] & 0xFF);
		changeb[readThisFragment - 1] = (byte)maskbyte;
		printf("内容：%d,[%d],%d\n", point, maskbyte, readThisFragment);
		payloadLength--;
		readThisFragment++;
	}
	//打印客户端的数据
	char charb[4096] = "";
	memcpy(charb, changeb, length);
	//charb[length] = 0;
	for (int i = 0; i < length; i++) {
		printf("%d\t", charb[i]);
	}
	printf("%d\n", 0);
	string s = charb;
	cout << "客户端数据========================================================：" << s << endl;
}

//发送协议
void responseInfo(SOCKET sClient)
{
	char message[] = "123456";
	byte test[20] = "";
	memcpy(test, message, strlen(message));
	respondClient(sClient, test, (int)strlen(message), true);
	cout << "我发送完了------------------------------" << endl;
}

// 专门处理套接字通讯的线程避免干扰主线程 建立新的链接
/*工作线程*/
void WorkThread(SOCKET sockClient) {
	char request[1024] = "";  //请求信息
	char clieninfo[2048] = ""; //握手后响应信息
	int len = 0;              //返回的长度
	int point = 0;            //字节指针位置
	int tmppoint = 0;         //临时指针变量

	//握手协议吧
	//第一步进行握手
	requestInfo(sockClient, request);
	cout << "server response-->" << request << endl;
	respondInfo(sockClient, request);
	cout << "request-->" << request << endl;
	//以上是握手协议
	//握手协议结束后，也就是服务返回给客户端后，客户端再一次返回
	//数据给服务器，下面就是解析客户端的返回数据
	//将数据全部读取出来
	len = recv(sockClient, clieninfo, 2048, 0);
	string ss = clieninfo;
	cout << "数据长度\n：" << len << "\n乱码数据：" << ss << endl;

	if (len > 0) {
		/*这里b字节数组是客户端的请求信息，需要注意point这个指针的变化，具体需要去理解它的协议，协议中每段字节里面包含了什么信息需要把    它解析出来*/
		byte b[4096] = "";
		//转为字节来处理
		memcpy(b, clieninfo, 2048);
		cout << "字节数据：" << b << endl;
		for (int i = 0; i <= 33; i++) {
			printf("%d\t", b[i]);
		}
		printf("\n");
		//取第一个字节
		int first = b[point] & 0xFF;
		printf("第一个：%d,%d,%d\n", point, b[point], first);
		byte opCode = (byte)(first & 0x0F);             //0000 1111 后四位为opCode 00001111
		if (opCode == 8) {
			closesocket(sockClient);
		}
		//取第二个字节
		first = b[++point];
		//负载长度
		int payloadLength = first & 0x7F;
		printf("第二个：%d,[%d],%d\n", point, b[point], payloadLength);
		if (payloadLength == 126) {
			byte extended[2] = "";
			extended[0] = b[++point];
			extended[1] = b[++point];
			int shift = 0;
			payloadLength = 0;
			for (int i = 2 - 1; i >= 0; i--) {
				payloadLength = payloadLength + ((extended[i] & 0xFF) << shift);
				shift += 8;
			}
		}
		else if (payloadLength == 127) {
			byte extended[8] = "";
			tmppoint = ++point;     //保存临时指针
			point = --point;
			for (int i = 0; i < 8; i++) {
				extended[i] = b[tmppoint + i];
				point++;
			}
			int shift = 0;
			payloadLength = 0;
			for (int i = 8 - 1; i >= 0; i--) {
				payloadLength = payloadLength + ((extended[i] & 0xFF) << shift);
				shift += 8;
			}
		}

		//非126和127置回来
		if ((payloadLength != 126) || (payloadLength != 127)) {
			point = 1;
		}

		cout << "负载长度:" << payloadLength << endl;
		//第三个字节，掩码
		byte mask[4] = "";
		tmppoint = ++point;
		//因为自增了一次，这里需要减掉
		point = --point;
		//取掩码值
		for (int i = 0; i < 4; i++) {
			mask[i] = b[tmppoint + i];
			point++;
			printf("第三mask个：%d,[%d],%d\t\n", point, mask[i], payloadLength);
		}
		byte changeb[4096] = "";

		//内容的长度保留，循环里面已经被改变
		int length = payloadLength;
		int readThisFragment = 1;

		//通过掩码计算真实的数据
		while (payloadLength > 0) {
			int maskbyte = b[++point];
			int index = (readThisFragment - 1) % 4;
			maskbyte = maskbyte ^ (mask[index] & 0xFF);
			changeb[readThisFragment - 1] = (byte)maskbyte;
			printf("内容：%d,[%d],%d\n", point, maskbyte, readThisFragment);
			payloadLength--;
			readThisFragment++;
		}
		char a[4096] = "1231";
		byte test[1024] = "";
		memcpy(test, a, strlen(a));

		Sleep(5000);//这里是模拟处理其他事情
		//响应客户端
		//respondClient(sockClient, changeb, length, true);
		respondClient(sockClient, test, (int)strlen(a), true);

		//打印客户端的数据
		char charb[4096] = "";
		memcpy(charb, changeb, length);
		//charb[length] = 0;
		for (int i = 0; i < length; i++) {
			printf("%d\t", charb[i]);
		}
		printf("%d\n", 0);
		string s = charb;
		cout << "客户端数据：" << s << endl;
		//closesocket(sockClient);
	}
}

// 初始化Socket 同时建立服务端端口监听 
// port 监听的端口号
void Initsocket(int port) {
	WORD imgrequest;
	WSADATA wsadata;
	imgrequest = MAKEWORD(1, 1);
	int err;
	err = WSAStartup(imgrequest, &wsadata);
	if (!err) {
		printf("服务已经启动\n");
	}
	else {
		printf("服务未启动\n");
		return;
	}
	SOCKET sersocket = socket(AF_INET, SOCK_STREAM, 0);//TCP

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);      //ip地址
	addr.sin_port = htons(port);                        //绑定端口
	bind(sersocket, (SOCKADDR*)&addr, sizeof(SOCKADDR));//绑定完成
	listen(sersocket, 10);                              //其中第二个参数代表能够接收的最多的连接数

	SOCKADDR_IN clientsocket;
	int len = sizeof(SOCKADDR);
	boolean isConnected = false;

	int i = 0;
	while (true) {
		// 阻塞方法 每次获取到获取到一个链接后, 建立一个新的套接字, 之后这个链接的通讯都由这个套接字完成
		SOCKET serConn = accept(sersocket, (SOCKADDR*)&clientsocket, &len);
		printf("客户端连接\n");
		//我这里起了一个线程来处理协议
		HANDLE hThread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkThread, (LPVOID)serConn, 0, 0);
		if (hThread1 != NULL)
		{
			CloseHandle(hThread1);
		}
		Sleep(100000);
		closesocket(serConn);
	}
}

int main(int argc, char* atgv[])
{
#ifdef _WIN32
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}
	SOCKET slisten = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == slisten)
	{
		printf("socket init error !");
		return 0;
	}

	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(6789);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (SOCKET_ERROR == bind(slisten, (LPSOCKADDR)&sin, sizeof sin))
	{
		printf("bind error !");
		return 0;
	}
	if (SOCKET_ERROR == listen(slisten, 10))//最多建立10个链接
	{
		printf("listen error !");
		return 0;
	}

	SOCKET sClient;
	sockaddr_in remoteAddr;
	int nAddrlen = sizeof(remoteAddr);
	printf("等待连接...\n");
	sClient = accept(slisten, (SOCKADDR*)&remoteAddr, &nAddrlen);
	if (INVALID_SOCKET == sClient)
	{
		printf("accept error !");
	}

	char sendBuf[20] = { '\0' };
	char revData[1024];
	printf("接收到一个连接：%s \r\n", inet_ntop(AF_INET, (void*)&remoteAddr.sin_addr, sendBuf, 16));
	int ret = recv(sClient, revData, 2048, 0);
	string strRevData = revData;
	string::size_type idx;
	idx = strRevData.find("GET");
	if (idx == string::npos)//不存在。
		cout << "大爷不来啊";
	else//存在。
		cout << "大爷,您可算是来了" << endl;
	cout << "大爷您结束了?" << endl;

	size_t index = strRevData.find("Sec-WebSocket-Key");
	string strSecWebSocketKey = strRevData.substr(index + 19, 24);
	char request[1024] = "";
	getKey(request, strSecWebSocketKey);
	cout << "下面的是大爷" << endl;
	cout << request << endl;
	cout << "上面的是大爷" << endl;

	if (ret > 0)
		cout << "ret-->" << ret << endl;
	{
		revData[ret] = 0x00;
		printf(revData);
	}
	send(sClient, request, (int)strlen(request), 0);
	for (int i = 100; i < (size_t)strlen(request); i++) {
		//cout << "request" << i << ":";
		//cout << request[i] << ":ASCII:" << (int)request[i] << endl;
	}
	cout << "握手成功, 我们可以愉快的聊天了" << endl;

	string ss;
	char clieninfo[2048] = ""; //握手后响应信息

	for (int i = 0; i < 4; i++) {

		//发送数据给客户端
		cout << "等你说话" << endl;
		memset(clieninfo, '\0', sizeof(clieninfo));
		ret = recv(sClient, clieninfo, 2048, 0);
		ss = clieninfo;
		cout << "数据长度:\n" << ret << "\n乱码数据：" << ss << endl;
		if (ret > 0) {
			// 获取客户端数据
			getClientInfo(sClient, clieninfo);
		}
		responseInfo(sClient);
		Sleep(1000);
	}
	Sleep(10000);

	closesocket(slisten); //关闭监听的套接字
	WSACleanup();
	return 0;

#else

#endif
}