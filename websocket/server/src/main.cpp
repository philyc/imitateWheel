#include <iostream>
#include <WinSock2.h>
#include <string>
#include "sha1.h"
#include "base64.h"

using namespace std;

int main(int argc,char* atgv[])
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}
}