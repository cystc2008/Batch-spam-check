/*
Copyright (c) 2013 著作权由Cystc所有。著作权人保留一切权利。

这份授权条款，在使用者符合以下三条件的情形下，授予使用者使用及再散播本
软件包装原始码及二进位可执行形式的权利，无论此包装是否经改作皆然：

对于本软件源代码的再散播，必须保留上述的版权宣告、此三条件表列，以
及下述的免责声明。
对于本套件二进位可执行形式的再散播，必须连带以文件以及／或者其他附
于散播包装中的媒介方式，重制上述之版权宣告、此三条件表列，以及下述
的免责声明。
未获事前取得书面许可，不得使用Cystc或本软件贡献者之名称，
来为本软件之衍生物做任何表示支持、认可或推广、促销之行为。

免责声明：本软件是由Cystc及本软件之贡献者以现状（"as is"）提供，
本软件包装不负任何明示或默示之担保责任，包括但不限于就适售性以及特定目
的的适用性为默示性担保。Cystc及本软件之贡献者，无论任何条件、
无论成因或任何责任主义、无论此责任为因合约关系、无过失责任主义或因非违
约之侵权（包括过失或其他原因等）而起，对于任何因使用本软件包装所产生的
任何直接性、间接性、偶发性、特殊性、惩罚性或任何结果的损害（包括但不限
于替代商品或劳务之购用、使用损失、资料损失、利益损失、业务中断等等），
不负任何责任，即在该种使用已获事前告知可能会造成此类损害的情形下亦然。
*/

#include "Batchspamchecker.h"

/*保存输入的IP*/
char *IPData=NULL;

/*保存主窗口句柄*/
HWND MainWindow=NULL;

/*保存查询线程句柄*/
HANDLE hThread=NULL;

/*保存带有通配符的一组IP*/
typedef struct
{
	char high[4],midhigh[4],midlow[4],low[4];
} IPGROUP;

/*保存带有通配符的一组穷举后的IP*/
IPGROUP *IPs=NULL;

/*设置窗口图标*/
void __stdcall SetIcon(HWND hwnd)
{
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON2)));
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON3)));
}

/*初始化通用对话框*/
OPENFILENAMEA __stdcall InitCommonDlg(LPCSTR lpstrFilter,HWND hwnd)
{
	OPENFILENAMEA ofn;
	static char ModuleName[1000]="";
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;		
	ofn.lpstrFile = ModuleName;
	ofn.nMaxFile = 1000;
	ofn.lpstrFilter =lpstrFilter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_OVERWRITEPROMPT; 
	return ofn;
}

/*获取保存文件路径*/
char* __stdcall GetSavePath(HWND hwnd)
{
	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn=InitCommonDlg("文本文件（*.txt）\0*.txt\0",hwnd);
	if(GetSaveFileNameA(&ofn))
	{
		return ofn.lpstrFile;
	}
	return "";
}

/*将结果写到文件*/
void __stdcall WriteResultFile(HWND hwnd) 
{
	int ListIndex=0;
	int ListNumber=0;
	int len=0;
	HWND hList=NULL;
	char FilePath[1000];
	char ItemData[100];
	FILE *ResultFile=NULL;
	strcpy(FilePath,GetSavePath(hwnd));
	len=strlen(FilePath);
	if(strcmp(&FilePath[len-4],".txt")!=0)
	{
		strcat(FilePath,".txt");
	}
	if(FilePath[2]=='\\')
	{
		ResultFile=fopen(FilePath,"wt+");
		if(ResultFile!=NULL)
		{
			hList=GetDlgItem(hwnd,IDC_RESULT);
			ListNumber=(int)SendMessage(hList,LB_GETCOUNT,0,0);
			for(ListIndex=0;ListIndex<ListNumber;ListIndex++)
			{
				SendMessage(hList, LB_GETTEXT, (WPARAM)ListIndex, (LPARAM)ItemData);
				fputs(ItemData,ResultFile);
				fputc('\n',ResultFile);
				ZeroMemory(ItemData, sizeof(char)*100);
			}
			fclose(ResultFile);
		}
	}
}

/*复制选中的结果到剪贴板*/
void  __stdcall CopyToClickboard(HWND hwnd)
{
	HWND hList = GetDlgItem(hwnd, IDC_RESULT);	
	int SelCount = SendMessage(hList, LB_GETSELCOUNT, 0, 0);
	int *SelIndex=(int*)GlobalAlloc(GPTR, sizeof(int)*SelCount);
	char SELData[100];
	GLOBALHANDLE hGlobal =GlobalAlloc(GHND | GMEM_SHARE, sizeof(char)*100*SelCount);
	char *SELDatas=(char*)GlobalLock(hGlobal);
	int i=0;
	SendMessage(hList, LB_GETSELITEMS, (WPARAM)SelCount, (LPARAM)SelIndex);
	for(i=0;i<SelCount;i++)
	{
		ZeroMemory(SELData, sizeof(char)*100);
		SendMessage(hList,LB_GETTEXT,(WPARAM)SelIndex[i],(LPARAM)SELData);
		strcat(SELData,"\r\n");
		strcat(SELDatas,SELData);
	}
	GlobalUnlock(hGlobal);
	OpenClipboard (hwnd) ;        
	EmptyClipboard () ;
	SetClipboardData (CF_TEXT, hGlobal);
	CloseClipboard () ;
	GlobalFree((HGLOBAL)SelIndex);
}

/*关闭结果对话框*/
void __stdcall CloseResultDlg(DWORD ExitCode,HWND hwnd)
{
	if(GetExitCodeThread(hThread,&ExitCode))
	{
		if(ExitCode==STILL_ACTIVE)
		{
			TerminateThread(hThread,ExitCode);
			if(IPs!=NULL)
			{
				GlobalFree((HGLOBAL)IPs);
				IPs=NULL;
			}
		}
	}
	if(IPData!=NULL)
	{
		GlobalFree((HGLOBAL)IPData);
		IPData=NULL;
	}
	EnableWindow(MainWindow,TRUE);
	EndDialog(hwnd, IDCANCEL);
}

/*获取控件相对坐标*/
POINT __stdcall GetClientPOS(HWND hwnd,HWND hCtrl,__out RECT* LPrectctrl)
{	
	POINT pos={0,0};
	GetWindowRect(hCtrl,LPrectctrl);
	pos.x=LPrectctrl->left;
	pos.y=LPrectctrl->top;
	ScreenToClient(hwnd,&pos);
	return pos;
}

/*获取控件尺寸*/
SIZE __stdcall GetCtrlSize(RECT rect)
{
	SIZE sz={0,0};
	sz.cx=rect.right-rect.left;
	sz.cy=rect.bottom-rect.top;
	return sz;
}

/*处理结果窗消息*/
BOOL CALLBACK ResultProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD ExitCode=0;/*保存查询线程出口码*/
	HMENU hMenu=NULL;
	RECT rect={0,0,0,0};
	//static RECT rectold;
	static RECT rectoldCANCEL;
	static RECT rectoldSAVE;
	static RECT rectoldRESULT;
	static POINT posCANCEL;
	static POINT posSAVE;
	static POINT posRESULT;
	static SIZE szCANCEL;
	static SIZE szSAVE;
	static SIZE szRESULT;
	HWND hIDCANCEL=GetDlgItem(hwnd,IDCANCEL);
	HWND hIDC_SAVE=GetDlgItem(hwnd,IDC_SAVE);
	HWND hIDC_RESULT=GetDlgItem(hwnd,IDC_RESULT);
	switch(message)
	{
		case WM_INITDIALOG:
			SetIcon(hwnd);
			break;		
		case WM_CONTEXTMENU:
			hMenu=LoadMenuA(GetModuleHandleA(NULL),MAKEINTRESOURCEA(IDR_MENU));
			TrackPopupMenu(GetSubMenu(hMenu,0),TPM_LEFTALIGN | TPM_TOPALIGN |TPM_RIGHTBUTTON,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),0,hwnd,NULL);
			DestroyMenu(hMenu); 
			break;
		case WM_SIZE:
			switch(wParam)
			{
				case SIZE_MAXIMIZED:
					/*备份控件位置和大小*/
					posCANCEL=GetClientPOS(hwnd,hIDCANCEL,&rectoldCANCEL);
					posSAVE=GetClientPOS(hwnd,hIDC_SAVE,&rectoldSAVE);
					posRESULT=GetClientPOS(hwnd,hIDC_RESULT,&rectoldRESULT);
					szCANCEL=GetCtrlSize(rectoldCANCEL);
					szSAVE=GetCtrlSize(rectoldSAVE);
					szRESULT=GetCtrlSize(rectoldRESULT);
					/*修改控件位置和大小*/
					GetWindowRect(hwnd,&rect);										
					MoveWindow(hIDCANCEL,rect.right-89,rect.bottom-54,75,23,TRUE);
					MoveWindow(hIDC_SAVE,rect.left+10,rect.bottom-54,102,23,TRUE);
					MoveWindow(hIDC_RESULT,rect.left+10,rect.top+10,rect.right-20,rect.bottom-65,TRUE);
					break;
				case SIZE_RESTORED:
					/*还原控件位置和大小*/
					MoveWindow(hIDCANCEL,posCANCEL.x,posCANCEL.y,szCANCEL.cx,szCANCEL.cy,TRUE);
					MoveWindow(hIDC_SAVE,posSAVE.x,posSAVE.y,szSAVE.cx,szSAVE.cy,TRUE);
					MoveWindow(hIDC_RESULT,posRESULT.x,posRESULT.y,szRESULT.cx,szRESULT.cy,TRUE);
					break;
				default:break;
			}
			break;
		case WM_SYSCOMMAND:
			switch(wParam)
			{
				case SC_CLOSE:
					CloseResultDlg(ExitCode,hwnd);
					break;
				default:return DefWindowProc(hwnd,message,wParam,lParam);
			}
			break;
		case WM_COMMAND:		
			switch(LOWORD(wParam))
			{
				case IDCANCEL:															
					CloseResultDlg(ExitCode,hwnd);
					break;
				case IDC_SAVE:
					WriteResultFile(hwnd);
					break;
				case IDM___1:
					CopyToClickboard(hwnd);
					break;
				default:break;
			}
			break;
		default:return FALSE;		
	}
	return TRUE;
}

/*检查IP地址黑名单*/
char* __stdcall CheckIPAdress(char IPAddress[])
{
	SOCKET sSocket=INVALID_SOCKET;
	SOCKADDR_IN stSvrAddrIn={0}; /* 服务器端地址 */
	char sndBuf[1024]="";
	static char rcvBuf[2048]="";
	char *pRcv=rcvBuf;
	int num=0,nRet=SOCKET_ERROR;
	WSADATA wsaData;
	/*构造请求消息*/
	sprintf(sndBuf, "GET http://www.stopforumspam.com/api?ip=%s\n\r\n",IPAddress);
	/* socket初始化 */
	WSAStartup(MAKEWORD(2, 0), &wsaData);
	stSvrAddrIn.sin_family=AF_INET;
	stSvrAddrIn.sin_port=htons(80); 
	stSvrAddrIn.sin_addr.s_addr=inet_addr("195.20.205.9");
	sSocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/*连接*/
	nRet=connect(sSocket,(SOCKADDR*)&stSvrAddrIn, sizeof(SOCKADDR));
	if(nRet==SOCKET_ERROR)
	{
		MessageBoxA(NULL,"连接spam服务器失败！","警告",MB_ICONERROR|MB_SYSTEMMODAL);
		return NULL;/*连接服务器失败返回*/
	}
	/*发送HTTP请求消息*/
	send(sSocket, (char*)sndBuf, sizeof(sndBuf), 0);
	/*接收HTTP响应消息*/
	while(1)
	{
		num = recv(sSocket, pRcv, 2048, 0);
		pRcv += num;
		if(num==0||num==-1)
		{
			break ;
		}
	}
	/*返回响应消息*/
	return rcvBuf;
}

/*获取IP归属地*/
location __stdcall GetLocation(char IP[21])
{
	location loc={0};  
	openshare();
	getipinfo(IP,&loc);  
	return loc;
}

/*输出结果*/
void __stdcall PrintResult(char IPAddress[],char result[],HWND ResultDlg)
{
	char msg[100]="";
	static unsigned int Error=0;
	location loc=GetLocation(IPAddress);
	DWORD ExitCode=0;
	if(result!=NULL)
	{
		if(result[53]=='y')
		{
			sprintf(msg,"ip %s   在黑名单中   %s %s",IPAddress,loc.p_country,loc.p_area );		
		}
		else if(result[53]=='n')
		{
			sprintf(msg,"ip %s   不在黑名单   %s %s",IPAddress,loc.p_country,loc.p_area );
		}
		if(ResultDlg!=NULL)
		{
			SendDlgItemMessageA(ResultDlg,IDC_RESULT,LB_ADDSTRING,0,(LPARAM)msg);			
		}
	}
	else if(++Error>=2)/*连续错误，终止线程执行*/
	{		
		if(GetExitCodeThread(GetCurrentThread(),&ExitCode))
		{
			ExitThread(ExitCode);
		}
	}
}

/*枚举包含*号的IP的所有可能*/
void __stdcall EnumTPF(IPGROUP IPGroup,HWND ResultDlg)
{
	unsigned int i[4]={0,0,0,0};
	DWORD ExitCode=0;
	unsigned long Number=260,Num=0;
	unsigned int j,j1,k,l,m,n;
	char Buf[4]={0,0,0,0};
	char IPResult[21];
	/*计算IP中包含几个*号以确定枚举后的IP大概数量，便于分配内存空间*/
	if(IPGroup.high[0]=='*')
		i[0]=1;
	if(IPGroup.midhigh[0]=='*')
		i[1]=1;
	if(IPGroup.midlow[0]=='*')
		i[2]=1;
	if(IPGroup.low[0]=='*')
		i[3]=1;
	for(n=1;n<i[0]+i[1]+i[2]+i[3];n++)
	{
		Number=Number*260;
	}
	/*如IP数量过多*/
	if(Number>16581375)
	{
		MessageBoxA(NULL,"无法一次查询这么多IP","警告",MB_ICONERROR);
		if(GetExitCodeThread(GetCurrentThread(),&ExitCode))
		{
			ExitThread(ExitCode);
		}
	}
	/*开始枚举*/
	else
	{
		IPs=(IPGROUP*)GlobalAlloc(GPTR,sizeof(IPGROUP)*Number);
		for(j1=0;j1<4;j1++)
		{
			IPs[0].high[j1]=IPGroup.high[j1];
			IPs[0].midhigh[j1]=IPGroup.midhigh[j1];
			IPs[0].midlow[j1]=IPGroup.midlow[j1];
			IPs[0].low[j1]=IPGroup.low[j1];
		}
		Num++;
		for(j=0;j<=255;j++)
		{
			if(i[0])
			{
				for(j1=0;j1<4;j1++)
				{
					Buf[j1]=0;
				}
				itoa(j,Buf,10);
				for(j1=0;j1<4;j1++)
				{
					IPs[Num].high[j1]=Buf[j1];
				}
				for(j1=0;j1<4;j1++)
				{
					IPs[Num].midhigh[j1]=IPs[Num-1].midhigh[j1];
				}
				for(j1=0;j1<4;j1++)
				{
					IPs[Num].midlow[j1]=IPs[Num-1].midlow[j1];
				}
				for(j1=0;j1<4;j1++)
				{
					IPs[Num].low[j1]=IPs[Num-1].low[j1];
				}
				Num++;
			}
			else{j=255;}
			for(k=0;k<=255;k++)
			{
				if(i[1])
				{
					for(j1=0;j1<4;j1++)
					{
						Buf[j1]=0;
					}
					itoa(k,Buf,10);
					for(j1=0;j1<4;j1++)
					{
						IPs[Num].high[j1]=IPs[Num-1].high[j1];
					}
					for(j1=0;j1<4;j1++)
					{
						IPs[Num].midhigh[j1]=Buf[j1];
					}
					for(j1=0;j1<4;j1++)
					{
						IPs[Num].midlow[j1]=IPs[Num-1].midlow[j1];
					}
					for(j1=0;j1<4;j1++)
					{
						IPs[Num].low[j1]=IPs[Num-1].low[j1];
					}
					Num++;
				}
				else{k=255;}
				for(l=0;l<=255;l++)
				{					
					if(i[2])
					{
						for(j1=0;j1<4;j1++)
						{
							Buf[j1]=0;
						}
						itoa(l,Buf,10);
						for(j1=0;j1<4;j1++)
						{
							IPs[Num].high[j1]=IPs[Num-1].high[j1];
						}
						for(j1=0;j1<4;j1++)
						{
							IPs[Num].midhigh[j1]=IPs[Num-1].midhigh[j1];
						}
						for(j1=0;j1<4;j1++)
						{
							IPs[Num].midlow[j1]=Buf[j1];
						}
						for(j1=0;j1<4;j1++)
						{
							IPs[Num].low[j1]=IPs[Num-1].low[j1];
						}
						Num++;
					}
					else{l=255;}
					for(m=0;m<=255;m++)
					{
						if(i[3])
						{
							for(j1=0;j1<4;j1++)
							{
								Buf[j1]=0;
							}
							itoa(m,Buf,10);
							for(j1=0;j1<4;j1++)
							{
								IPs[Num].high[j1]=IPs[Num-1].high[j1];
							}
							for(j1=0;j1<4;j1++)
							{
								IPs[Num].midhigh[j1]=IPs[Num-1].midhigh[j1];
							}
							for(j1=0;j1<4;j1++)
							{
								IPs[Num].midlow[j1]=IPs[Num-1].midlow[j1];
							}
							for(j1=0;j1<4;j1++)
							{
								IPs[Num].low[j1]=Buf[j1];
							}
							Num++;
						}
						else{m=255;}
					}
				}
			}
		}
		/*过滤掉枚举后包含*号的IP然后提交*/
		for(n=0;n<Num;n++)
		{
			if(IPs[n].high[0]!='*'&&IPs[n].midhigh[0]!='*'&&IPs[n].midlow[0]!='*'&&IPs[n].low[0]!='*')
			{
				sprintf(IPResult,"%s.%s.%s.%s",IPs[n].high,IPs[n].midhigh,IPs[n].midlow,IPs[n].low);
				PrintResult(IPResult,CheckIPAdress(IPResult),ResultDlg);
			}
		}
		/*释放内存*/
		GlobalFree((HGLOBAL)IPs);
		IPs=NULL;
	}
}

/*将一组IP整理成char*.char*.char*.char*的格式*/
void __stdcall trimIP(char IPAddress[],HWND ResultDlg)
{
	static IPGROUP IPGroup={"","","",""};
	unsigned int i=0,j=0,Part=0;
	for(i=0;IPAddress[i]!='\0';i++)
	{
		if(IPAddress[i]=='.')
		{
			Part++;
			j=0;
			continue;
		}
		else if(i<=2&&Part==0)
		{
			IPGroup.high[j]=IPAddress[i];
			j++;
		}
		else if(i<=6&&Part==1)
		{
			IPGroup.midhigh[j]=IPAddress[i];
			j++;
		}
		else if(i<=10&&Part==2)
		{
			IPGroup.midlow[j]=IPAddress[i];
			j++;
		}
		else if(i<=14&&Part==3)
		{
			IPGroup.low[j]=IPAddress[i];
			j++;
		}
	}
	EnumTPF(IPGroup,ResultDlg);
}

/*按行处理用户输入的IP以便于提交*/
void __stdcall ProcessIPData(HWND ResultDlg)
{	
	char IPAddress[21]="";
	unsigned int m=0;
	unsigned int n=0;
	unsigned int i=0;
	while(IPData[m]!='\0')
	{
		for(n=0;IPData[m]!='\n'&&IPData[m]!='\0';n++,m++)
		{
			if(IPData[m]==' ')/*自动忽略空格*/
			{
				n--;
				continue;
			}
			IPAddress[n]=IPData[m];	
		}
		IPAddress[n]='\0';
		for(i=0;IPAddress[i]!='\0';i++)/*如果IP带通配符*/
		{
			if(IPAddress[i]=='*'/*||IPAddress[i]=='?'*/)
			{
				trimIP(IPAddress,ResultDlg);
				break;
			}
		}
		if(IPAddress[i]=='\0')/*如果IP不带通配符*/
		{
			if(IPAddress[i-1]=='\r')/*去除回车符*/
			{
				IPAddress[i-1]='\0';
			}
			PrintResult(IPAddress,CheckIPAdress(IPAddress),ResultDlg);
		}
		m++;		
	}
}

/*获取用户输入的IP地址*/
void __stdcall GetIpData(HWND hwnd)
{
	int len = GetWindowTextLengthA(GetDlgItem(hwnd, IP_LIST));
	static HWND ResultDlg=NULL;
	if(len > 0)
	{
		IPData=(char*)GlobalAlloc(GPTR,len+2);
		/*获取用户输入数据*/
		GetDlgItemTextA(hwnd, IP_LIST, IPData,len+1);
		/*简单检测用户输入数据是否合法*/
		if(((IPData[0]>=48&&IPData[0]<=57)||IPData[0]=='*')&&((IPData[len-1]>=48&&IPData[len-1]<=57)||IPData[len-1]=='*'||IPData[len-1]==13||IPData[len-1]==10||IPData[len-1]==32)&&(IPData[1]=='.'||IPData[2]=='.'||IPData[3]=='.'))
		{
			/*如合法*/						
			EnableWindow(hwnd,FALSE);
			ResultDlg=CreateDialogA(GetModuleHandleA(NULL),MAKEINTRESOURCEA(IDD_RESULT),hwnd,ResultProc);
			if(ResultDlg!=NULL)
			{
				ShowWindow(ResultDlg,SW_SHOW);
			}
			hThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ProcessIPData ,ResultDlg,0,NULL);
		}
		else
		{
			/*如不合法*/
			GlobalFree((HGLOBAL)IPData);
			IPData=NULL;
			MessageBoxA(hwnd,"您输入的不是IP地址！","警告",MB_ICONERROR);
		}
	}
}

/*处理主窗口消息*/
BOOL CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MainWindow=hwnd;
	switch(message)
	{
		case WM_INITDIALOG: 
			SetIcon(hwnd);
			break;		
		case WM_COMMAND:		
			switch(LOWORD(wParam))
			{
				case IDCHECK:
					GetIpData(hwnd);					
					break;
				case IDABOUT:
					MessageBoxA(hwnd,"本软件著作权归Cystc所有，欢迎复制，传播，使用！","版权信息",MB_ICONINFORMATION);
					break;
				case IDCLEAN:
					SetDlgItemTextA(hwnd, IP_LIST,"");
					break;
				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					break;
				default:break;
			}
			break;
		default:return FALSE;		
	}
	return TRUE;
}

/*程序入口*/
int WINAPI WinMain(HINSTANCE hThisInstance,HINSTANCE hPrevInstance,LPSTR lpszArgument,int nFunsterStil)
{
	DialogBoxA(GetModuleHandleA(NULL),MAKEINTRESOURCEA(MAIN_GUI),0,WindowProc);
	closeshare();
	return TRUE;
}