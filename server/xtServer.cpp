// xtServer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "App.h"

int main()
{
	if (theApp.startup()) {
		theApp.service();
		theApp.cleanup();
	}
	else {
		theApp.cleanup();
		printf(" ��ʼ��ʧ�� \n");
		getchar();
	}

    return 0;
}

