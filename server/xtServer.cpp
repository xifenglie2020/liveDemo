// xtServer.cpp : 定义控制台应用程序的入口点。
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
		printf(" 初始化失败 \n");
		getchar();
	}

    return 0;
}

