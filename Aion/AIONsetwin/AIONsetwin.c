#include <windows.h>
#include <stdio.h>
#include <string.h>
#define BUF_LIM 1024

const char c_str_walker[] = "AION Client";
int g_res = 0;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	char buf[BUF_LIM];
	WINDOWPLACEMENT wpm;

	GetWindowText(hwnd, buf, BUF_LIM);

	if(strstr(buf,c_str_walker)) {
		memset(&wpm,0,sizeof(WINDOWPLACEMENT));
		wpm.length = sizeof(WINDOWPLACEMENT);
		wpm.showCmd = SW_RESTORE;
		SetWindowPlacement(hwnd,&wpm);
		SetWindowPos(hwnd,HWND_BOTTOM,-4,-4,1688,g_res,0);
		printf("Found AION window of handle: %X\n",(unsigned int)hwnd);
	}

	return TRUE;
}

int main(int argc, char** argv)
{
	g_res = atoi(argv[1]);
	if(!g_res)
		g_res = 1050 - 25;	//default for 1024
	else
		g_res -= 25;
	EnumWindows(&EnumWindowsProc, 0);
	return 0;
}
