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
	cout << "server_key�ĳ���" << server_key.size() << endl;
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
	cout << ">>>>>>>>>>>>>>>>>>>>>>>���鳤��:" << length << endl;
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
		printf("Ҫ���͵������ֽڣ�%d\n", charb[i]);
	}
	char charbuf[100] = "";
	memcpy(charbuf, buf, length + tmp);
	send(sockClient, charbuf, 100, 0);
}

/*��ȡkey��Э��*/
void requestInfo(SOCKET sockClient, char* request) {
	char revData[1024]; //�洢���ܵ�����
	int ret = recv(sockClient, revData, 2048, 0);
	revData[ret] = '\0';
	string revDataString = revData;
	string getString = "GET";
	string::size_type idx;
	idx = revDataString.find(getString);//��a�в���b.
	if (idx == string::npos)//�����ڡ�
		cout << "��ǰ��������Э��, ����ʧ��";
	else//���ڡ�
		cout << "��ǰ������Э��, ��ʼ����" << endl;
	cout << "revDataString-->" << revDataString.c_str() << endl;
	// ����Э���е�Sec-WebSocket-Key �ֶ�
	size_t index = revDataString.find("Sec-WebSocket-Key");
	if (index > 0) {
		// ��ȡ Sec-WebSocket-Key �ֶε�����
		string secWebSocketKeyString = revDataString.substr(index + 19, 24);
		char request[1024] = "";
		// ����Sec-WebSocket-Key�ֶμ���, ���ܺ󷵻ؿͻ���, ������ֶ���
		getKey(request, secWebSocketKeyString);
	}
	else {
		cout << "��ǰЭ���в�����Sec-WebSocket-Key�ֶ�, ����ʧ��" << endl;
	}
}

/*����Э��*/
void respondInfo(SOCKET sockClient, char* request) {
	send(sockClient, request, (int)strlen(request), 0);
}

void getClientInfo(SOCKET sClient, char clieninfo[])
{
	int point = 0;            //�ֽ�ָ��λ��
	int tmppoint = 0;         //��ʱָ�����
	byte b[4096] = "";

	memcpy(b, clieninfo, 2048);
	cout << "�ֽڳ���: " << end(b) - begin(b) << endl;
	for (int i = 0; i < 30; i++) {
		cout << "�ֽ�" << i << ":" << b[i] << endl;
	}
	for (int i = 0; i <= 10; i++) {
		printf("%d\t", b[i]);
	}
	printf("\n");

	//����websocket��Ϣ
	// 
	//ȡ��һ���ֽ�
	cout << "��һ���ֽ�:" << b[point] << endl;
	int first = b[point] & 0xFF;
	printf("��һ����%d,%d,%d\n", point, b[point], first);
	byte opCode = (byte)(first & 0x0F);             //0000 1111 ����λΪopCode 00001111
	if (opCode == 8) {
		cout << "�ر�" << endl;
		cout << "opCode:" << opCode << endl;
		closesocket(sClient);
	}

	//ȡ�ڶ����ֽ�
	first = b[++point];
	//���س���
	int payloadLength = first & 0x7F;
	printf("�ڶ�����%d,[%d],%d\n", point, b[point], payloadLength);
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
		tmppoint = ++point;     //������ʱָ��
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
	//��126��127�û���
	if ((payloadLength != 126) || (payloadLength != 127)) {
		point = 1;
	}

	cout << "���س���:" << payloadLength << endl;

	//�������ֽڣ�����
	byte mask[4] = "";
	tmppoint = ++point;
	//��Ϊ������һ�Σ�������Ҫ����
	point = --point;
	//ȡ����ֵ
	for (int i = 0; i < 4; i++) {
		mask[i] = b[tmppoint + i];
		point++;
		printf("����mask����%d,[%d],%d\t\n", point, mask[i], payloadLength);
	}
	byte changeb[4096] = "";

	//���ݵĳ��ȱ�����ѭ�������Ѿ����ı�
	int length = payloadLength;
	int readThisFragment = 1;
	//ͨ�����������ʵ������
	while (payloadLength > 0) {
		int maskbyte = b[++point];
		int index = (readThisFragment - 1) % 4;
		maskbyte = maskbyte ^ (mask[index] & 0xFF);
		changeb[readThisFragment - 1] = (byte)maskbyte;
		printf("���ݣ�%d,[%d],%d\n", point, maskbyte, readThisFragment);
		payloadLength--;
		readThisFragment++;
	}
	//��ӡ�ͻ��˵�����
	char charb[4096] = "";
	memcpy(charb, changeb, length);
	//charb[length] = 0;
	for (int i = 0; i < length; i++) {
		printf("%d\t", charb[i]);
	}
	printf("%d\n", 0);
	string s = charb;
	cout << "�ͻ�������========================================================��" << s << endl;
}

//����Э��
void responseInfo(SOCKET sClient)
{
	char message[] = "123456";
	byte test[20] = "";
	memcpy(test, message, strlen(message));
	respondClient(sClient, test, (int)strlen(message), true);
	cout << "�ҷ�������------------------------------" << endl;
}

// ר�Ŵ����׽���ͨѶ���̱߳���������߳� �����µ�����
/*�����߳�*/
void WorkThread(SOCKET sockClient) {
	char request[1024] = "";  //������Ϣ
	char clieninfo[2048] = ""; //���ֺ���Ӧ��Ϣ
	int len = 0;              //���صĳ���
	int point = 0;            //�ֽ�ָ��λ��
	int tmppoint = 0;         //��ʱָ�����

	//����Э���
	//��һ����������
	requestInfo(sockClient, request);
	cout << "server response-->" << request << endl;
	respondInfo(sockClient, request);
	cout << "request-->" << request << endl;
	//����������Э��
	//����Э�������Ҳ���Ƿ��񷵻ظ��ͻ��˺󣬿ͻ�����һ�η���
	//���ݸ���������������ǽ����ͻ��˵ķ�������
	//������ȫ����ȡ����
	len = recv(sockClient, clieninfo, 2048, 0);
	string ss = clieninfo;
	cout << "���ݳ���\n��" << len << "\n�������ݣ�" << ss << endl;

	if (len > 0) {
		/*����b�ֽ������ǿͻ��˵�������Ϣ����Ҫע��point���ָ��ı仯��������Ҫȥ�������Э�飬Э����ÿ���ֽ����������ʲô��Ϣ��Ҫ��    ����������*/
		byte b[4096] = "";
		//תΪ�ֽ�������
		memcpy(b, clieninfo, 2048);
		cout << "�ֽ����ݣ�" << b << endl;
		for (int i = 0; i <= 33; i++) {
			printf("%d\t", b[i]);
		}
		printf("\n");
		//ȡ��һ���ֽ�
		int first = b[point] & 0xFF;
		printf("��һ����%d,%d,%d\n", point, b[point], first);
		byte opCode = (byte)(first & 0x0F);             //0000 1111 ����λΪopCode 00001111
		if (opCode == 8) {
			closesocket(sockClient);
		}
		//ȡ�ڶ����ֽ�
		first = b[++point];
		//���س���
		int payloadLength = first & 0x7F;
		printf("�ڶ�����%d,[%d],%d\n", point, b[point], payloadLength);
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
			tmppoint = ++point;     //������ʱָ��
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

		//��126��127�û���
		if ((payloadLength != 126) || (payloadLength != 127)) {
			point = 1;
		}

		cout << "���س���:" << payloadLength << endl;
		//�������ֽڣ�����
		byte mask[4] = "";
		tmppoint = ++point;
		//��Ϊ������һ�Σ�������Ҫ����
		point = --point;
		//ȡ����ֵ
		for (int i = 0; i < 4; i++) {
			mask[i] = b[tmppoint + i];
			point++;
			printf("����mask����%d,[%d],%d\t\n", point, mask[i], payloadLength);
		}
		byte changeb[4096] = "";

		//���ݵĳ��ȱ�����ѭ�������Ѿ����ı�
		int length = payloadLength;
		int readThisFragment = 1;

		//ͨ�����������ʵ������
		while (payloadLength > 0) {
			int maskbyte = b[++point];
			int index = (readThisFragment - 1) % 4;
			maskbyte = maskbyte ^ (mask[index] & 0xFF);
			changeb[readThisFragment - 1] = (byte)maskbyte;
			printf("���ݣ�%d,[%d],%d\n", point, maskbyte, readThisFragment);
			payloadLength--;
			readThisFragment++;
		}
		char a[4096] = "1231";
		byte test[1024] = "";
		memcpy(test, a, strlen(a));

		Sleep(5000);//������ģ�⴦����������
		//��Ӧ�ͻ���
		//respondClient(sockClient, changeb, length, true);
		respondClient(sockClient, test, (int)strlen(a), true);

		//��ӡ�ͻ��˵�����
		char charb[4096] = "";
		memcpy(charb, changeb, length);
		//charb[length] = 0;
		for (int i = 0; i < length; i++) {
			printf("%d\t", charb[i]);
		}
		printf("%d\n", 0);
		string s = charb;
		cout << "�ͻ������ݣ�" << s << endl;
		//closesocket(sockClient);
	}
}

// ��ʼ��Socket ͬʱ��������˶˿ڼ��� 
// port �����Ķ˿ں�
void Initsocket(int port) {
	WORD imgrequest;
	WSADATA wsadata;
	imgrequest = MAKEWORD(1, 1);
	int err;
	err = WSAStartup(imgrequest, &wsadata);
	if (!err) {
		printf("�����Ѿ�����\n");
	}
	else {
		printf("����δ����\n");
		return;
	}
	SOCKET sersocket = socket(AF_INET, SOCK_STREAM, 0);//TCP

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);      //ip��ַ
	addr.sin_port = htons(port);                        //�󶨶˿�
	bind(sersocket, (SOCKADDR*)&addr, sizeof(SOCKADDR));//�����
	listen(sersocket, 10);                              //���еڶ������������ܹ����յ�����������

	SOCKADDR_IN clientsocket;
	int len = sizeof(SOCKADDR);
	boolean isConnected = false;

	int i = 0;
	while (true) {
		// �������� ÿ�λ�ȡ����ȡ��һ�����Ӻ�, ����һ���µ��׽���, ֮��������ӵ�ͨѶ��������׽������
		SOCKET serConn = accept(sersocket, (SOCKADDR*)&clientsocket, &len);
		printf("�ͻ�������\n");
		//����������һ���߳�������Э��
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
	if (SOCKET_ERROR == listen(slisten, 10))//��ཨ��10������
	{
		printf("listen error !");
		return 0;
	}

	SOCKET sClient;
	sockaddr_in remoteAddr;
	int nAddrlen = sizeof(remoteAddr);
	printf("�ȴ�����...\n");
	sClient = accept(slisten, (SOCKADDR*)&remoteAddr, &nAddrlen);
	if (INVALID_SOCKET == sClient)
	{
		printf("accept error !");
	}

	char sendBuf[20] = { '\0' };
	char revData[1024];
	printf("���յ�һ�����ӣ�%s \r\n", inet_ntop(AF_INET, (void*)&remoteAddr.sin_addr, sendBuf, 16));
	int ret = recv(sClient, revData, 2048, 0);
	string strRevData = revData;
	string::size_type idx;
	idx = strRevData.find("GET");
	if (idx == string::npos)//�����ڡ�
		cout << "��ү������";
	else//���ڡ�
		cout << "��ү,������������" << endl;
	cout << "��ү��������?" << endl;

	size_t index = strRevData.find("Sec-WebSocket-Key");
	string strSecWebSocketKey = strRevData.substr(index + 19, 24);
	char request[1024] = "";
	getKey(request, strSecWebSocketKey);
	cout << "������Ǵ�ү" << endl;
	cout << request << endl;
	cout << "������Ǵ�ү" << endl;

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
	cout << "���ֳɹ�, ���ǿ�������������" << endl;

	string ss;
	char clieninfo[2048] = ""; //���ֺ���Ӧ��Ϣ

	for (int i = 0; i < 4; i++) {

		//�������ݸ��ͻ���
		cout << "����˵��" << endl;
		memset(clieninfo, '\0', sizeof(clieninfo));
		ret = recv(sClient, clieninfo, 2048, 0);
		ss = clieninfo;
		cout << "���ݳ���:\n" << ret << "\n�������ݣ�" << ss << endl;
		if (ret > 0) {
			// ��ȡ�ͻ�������
			getClientInfo(sClient, clieninfo);
		}
		responseInfo(sClient);
		Sleep(1000);
	}
	Sleep(10000);

	closesocket(slisten); //�رռ������׽���
	WSACleanup();
	return 0;

#else

#endif
}