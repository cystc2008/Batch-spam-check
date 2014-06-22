/*
Copyright (c) 2013-2014 著作权由Cystc所有。著作权人保留一切权利。

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

/*判断是否去查询spam*/
unsigned int CheckSpamSW=0;

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

/*将UNICODE转换为ANSI*/
/*
char* UnicodeToAnsi(const wchar_t *Unicode_String)
{
    static char ANSI_String[1000];
    WideCharToMultiByte(CP_ACP,0 ,Unicode_String,-1,ANSI_String,1000,NULL,NULL);
    return ANSI_String;
}*/

/*将ANSI转换为UNICODE*/
/*
wchar_t* AnsiToUnicode(const char *ANSI_String)
{
	static wchar_t Unicode_String[1000];
	MultiByteToWideChar(CP_ACP,0, ANSI_String,-1,Unicode_String,1000);
	return Unicode_String;
}*/

/*初始化通用对话框*/
OPENFILENAMEA __stdcall InitCommonDlg(LPCSTR lpstrFilter,HWND hwnd)
{
	OPENFILENAMEA ofn;
	static char ModuleName[1000]="";
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize =sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;		
	ofn.lpstrFile = ModuleName;
	ofn.nMaxFile = 1000;
	ofn.lpstrFilter =lpstrFilter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_OVERWRITEPROMPT; 
	return ofn;
}

/*初始化菜单*/
MENUITEMINFO __stdcall InitMenu(LPSTR MenuText,unsigned int wID)
{
	MENUITEMINFO Minfo;
	ZeroMemory(&Minfo,sizeof(Minfo));
	Minfo.cbSize=sizeof(MENUITEMINFO);
	Minfo.fMask=MFT_STRING | MIIM_DATA | MIIM_ID | MIIM_TYPE;
	Minfo.fType=MFT_STRING;
	Minfo.wID=wID;
	Minfo.dwItemData=wID;
	Minfo.dwTypeData=MenuText;
	Minfo.cch=strlen(MenuText)+1;
	return Minfo;
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
	char ItemData[1000];
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
				ZeroMemory(ItemData, sizeof(char)*1000);
			}
			fclose(ResultFile);
		}
	}
}

/*从剪贴板中复制数据*/
char* GetCbData(HWND hwnd)
{
	GLOBALHANDLE hGlobal=NULL;
	char *pText=NULL;
	char *pGlobal=NULL;
	if(IsClipboardFormatAvailable(CF_TEXT))
	{
		OpenClipboard(hwnd);
		hGlobal=GetClipboardData(CF_TEXT);
		if(hGlobal!=NULL)
		{
			pText=(char*)GlobalAlloc(GMEM_FIXED,GlobalSize(hGlobal));
			pGlobal =(char*)GlobalLock(hGlobal);
			strcpy(pText,pGlobal);
			GlobalUnlock(hGlobal);     
			CloseClipboard();
			return pText;
		}
	}
	return NULL;
}

/*复制选中的结果到剪贴板*/
void  __stdcall CopyToClickboard(HWND hwnd,int mID)
{
	HWND hList = GetDlgItem(hwnd, IDC_RESULT);	
	int SelCount = SendMessage(hList, LB_GETSELCOUNT, 0, 0);
	int *SelIndex=(int*)GlobalAlloc(GPTR, sizeof(int)*SelCount);
	char SELData[1000];
	GLOBALHANDLE hGlobal =GlobalAlloc(GHND | GMEM_SHARE, sizeof(char)*1000*SelCount);
	char *SELDatas=(char*)GlobalLock(hGlobal);
	int i=0,j;
	SendMessage(hList, LB_GETSELITEMS, (WPARAM)SelCount, (LPARAM)SelIndex);
	for(i=0;i<SelCount;i++)
	{
		ZeroMemory(SELData, sizeof(char)*1000);
		SendMessageA(hList,LB_GETTEXT,(WPARAM)SelIndex[i],(LPARAM)SELData);
		if(mID==IDM_COPYIP)/*如果只需复制IP*/
		{			
			for(j=3;(SELData[j]>=48&&SELData[j]<=57)||SELData[j]=='.';j++)
			{
				SELData[j-3]=SELData[j];
			}
			SELData[j-3]='\0';
		}
		if(i!=SelCount-1)
		{
			strcat(SELData,"\r\n");
		}
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

/*将结果排序*/
void SortResult(HWND hwnd,int mID)
{
	HWND hList = GetDlgItem(hwnd, IDC_RESULT);	
	char temp[1000]="";
	int count=SendMessage(hList,LB_GETCOUNT ,0,0);
	int i=0,j=0;
	int a=0,b=0;
	if(mID==IDM_SORT1)/*前置在黑名单中的IP*/
	{
		a=-44;
		b=-78;
	}
	else if(mID==IDM_SORT2)/*前置不在黑名单中的IP*/
	{
		a=-78;
		b=-44;
	}
	/*开始排序*/
	for(i=j=0;j<count;i++,j++)
	{
		SendMessageA(hList,LB_GETTEXT,(WPARAM)i,(LPARAM)temp);				
		if(temp[21]==a)
		{
			ZeroMemory(temp, sizeof(char)*1000);
			continue;
		}
		else if(temp[21]==b)
		{
			SendMessageA(hList,LB_ADDSTRING,0,(LPARAM)temp);
			SendMessage(hList,LB_DELETESTRING,(WPARAM)i,0);
			ZeroMemory(temp, sizeof(char)*1000);
			i--;
		}
	}
}

/*处理结果窗消息*/
BOOL CALLBACK ResultProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD ExitCode=0;/*保存查询线程出口码*/
	HMENU hMenu=NULL;
	HMENU hSubMenu=NULL;
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
			hSubMenu=GetSubMenu(hMenu,0);
			if(CheckSpamSW==IDC_CHECK)
			{
				EnableMenuItem(hSubMenu,2,MF_BYPOSITION|MF_ENABLED);
			}
			TrackPopupMenu(hSubMenu,TPM_LEFTALIGN | TPM_TOPALIGN |TPM_RIGHTBUTTON,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),0,hwnd,NULL);
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
				case IDM_COPY:
					CopyToClickboard(hwnd,IDM_COPY);
					break;
				case IDM_COPYIP:
					CopyToClickboard(hwnd,IDM_COPYIP);
					break;
				case IDM_SORT1:
					SortResult(hwnd,IDM_SORT1);
					break;
			    case IDM_SORT2:
					SortResult(hwnd,IDM_SORT2);
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
	sprintf(sndBuf, "GET http://www.stopforumspam.com/api?ip=%s\r\n",IPAddress);
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
	location loc={NULL,NULL};  
	openshare();
	getipinfo(IP,&loc);  
	return loc;
}

/*输出结果*/
void __stdcall PrintResult(char IPAddress[],char result[],HWND ResultDlg)
{
	char msg[1000]="";
	static unsigned int Error=0;
	location loc=GetLocation(IPAddress);
	DWORD ExitCode=0;
	if(result!=NULL)
	{
		if(result[53]=='y')
		{
			sprintf(msg,"ip %-18s在黑名单中   %s %s",IPAddress,loc.p_country,loc.p_area);		
		}
		else if(result[53]=='n')
		{
			sprintf(msg,"ip %-18s不在黑名单   %s %s",IPAddress,loc.p_country,loc.p_area);
		}
		else if(result[0]=='N')/*如果仅查询归属地*/
		{
			sprintf(msg,"ip %-18s%s %s",IPAddress,loc.p_country,loc.p_area);
		}
		else
		{
			sprintf(msg,"ip %-18s查询失败   %s %s",IPAddress,loc.p_country,loc.p_area);
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
				_itoa(j,IPs[Num].high,10);
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
						IPs[Num].high[j1]=IPs[Num-1].high[j1];
					}
					_itoa (k,IPs[Num].midhigh,10);
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
							IPs[Num].high[j1]=IPs[Num-1].high[j1];
						}
						for(j1=0;j1<4;j1++)
						{
							IPs[Num].midhigh[j1]=IPs[Num-1].midhigh[j1];
						}
						_itoa (l,IPs[Num].midlow,10);
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
							_itoa (m,IPs[Num].low,10);
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
				if(CheckSpamSW==IDC_CHECK)
				{
					PrintResult(IPResult,CheckIPAdress(IPResult),ResultDlg);
				}
				else if(CheckSpamSW==IDC_CHKLOC)
				{
					PrintResult(IPResult,"NONE",ResultDlg);
				}
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
	IPGROUP IPGroup={"","","",""};
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
			/*提交查询*/
			if(CheckSpamSW==IDC_CHECK)
			{
				PrintResult(IPAddress,CheckIPAdress(IPAddress),ResultDlg);
			}
			else if(CheckSpamSW==IDC_CHKLOC)
			{
				PrintResult(IPAddress,"NONE",ResultDlg);
			}
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

/*从剪贴板中搜索IP数据*/
void __stdcall SearchAllIP(char *Data,HWND hwnd)
{
	/*临时存放IP的结构体*/
	IPGROUP ip={"","","",""};
	/*获取剪贴板内文本的长度*/
	int len=strlen(Data);
	/*获取原本的文本框内容长度*/
	int Origlen=GetWindowTextLengthA(GetDlgItem(hwnd, IP_LIST));
	/*定义所需的其他变量*/
	int i,j,k,l,m,hi,mh,ml,lo,GroupPos=0,ResultNum=0;
	char Result[20]="";
	char *OrigList=NULL;
	/*申请内存空间以保存所有搜索结果*/
	char *ResultAll=(char*)GlobalAlloc(GPTR,sizeof(char)*(len+Origlen+4+(len/3)));
	/*申请内存空间以保存历史搜索结果*/
	char **ResultHistory=(char**)GlobalAlloc(GPTR,sizeof(char*)*(len/7));
	for(m=0;m<len/7;m++)
	{
		ResultHistory[m]=(char*)GlobalAlloc(GPTR,sizeof(char)*20);
	}
	/*开始搜索*/
	for(i=0;i<len+1;i++)
	{
		/*判断第一个字符以及最小长度下最后一个字符是否为数字,小数点或*号*/
		if(((Data[i]>=48&&Data[i]<=57)||Data[i]=='*')&&((Data[i+6]>=48&&Data[i+6]<=57)||Data[i+6]=='*'||Data[i+6]=='.'))
		{
			/*重新初始化相关变量*/
			GroupPos=hi=mh=ml=lo=0;
			/*重新初始化临时存放IP的结构体*/
			for(k=0;k<4;k++)
			{
				ip.high[k]=ip.midhigh[k]=ip.midlow[k]=ip.low[k]='\0';
			}
			/*进一步判断接下来的14个字符以确定是否是IP并记录到临时存放IP的结构体,如不是IP则跳出循环停止记录IP*/
			for(j=i;j<=i+14&&j<len;j++)
			{
				if(Data[j]=='.'&&GroupPos<3)
				{
					GroupPos++;
				}
				else if((Data[j]>=48&&Data[j]<=57)||Data[j]=='*')
				{						
					if(GroupPos==0)
					{
						if(hi<3)
						{
							ip.high[hi++]=Data[j];
							ip.high[hi]='\0';
						}
						else{break;}
						if(atoi(ip.high)>255)
						{
							break;
						}
					}					
					if(GroupPos==1)
					{
						if(mh<3)
						{
							ip.midhigh[mh++]=Data[j];
							ip.midhigh[mh]='\0';
						}
						else{break;}
						if(atoi(ip.midhigh)>255)
						{
							break;
						}
					}
					if(GroupPos==2)
					{
						if(ml<3)
						{
							ip.midlow[ml++]=Data[j];
							ip.midlow[ml]='\0';
						}
						else{break;}
						if(atoi(ip.midlow)>255)
						{
							break;
						}
					}	
					if(GroupPos==3)
					{
						if(lo<3)
						{
							ip.low[lo++]=Data[j];
							ip.low[lo]='\0';
						}
						else{break;}
						if(atoi(ip.low)>255)
						{
							break;
						}
					}
				}
				else{break;}
			}
			/*最后确定是否为IP并保存到结果*/
			if(GroupPos==3&&((ip.low[0]>=48&&ip.low[0]<=57)||ip.low[0]=='*')&&atoi(ip.low)<=255)
			{
				sprintf(Result,"%s.%s.%s.%s\r\n",ip.high,ip.midhigh,ip.midlow,ip.low);
				/*判断与之前记录过的IP是否相同*/
				for(l=0;l<ResultNum;l++)
				{
					/*如相同，则不记录IP*/
					if(strcmp(Result,ResultHistory[l])==0)
					{
						goto same;/*跳过IP记录过程*/
					}
				}
				strcat(ResultAll,Result);
				strcpy(ResultHistory[ResultNum++],Result);
				same:
				i=i+strlen(Result)-3;/*跳过已记录的IP或者已忽略的与之前相同的IP的长度并继续搜索*/
			}
		}
	}
	/*将记录的IP与文本框内原有的内容连接，并重新填入文本框*/
	OrigList=(char*)GlobalAlloc(GPTR,sizeof(char)*(Origlen+2));
	GetDlgItemTextA(hwnd, IP_LIST, OrigList,Origlen+3);
	strcat(ResultAll,OrigList);
	SetDlgItemText(hwnd, IP_LIST, ResultAll);
	/*释放内存*/	
	GlobalFree((HGLOBAL)Data);
	GlobalFree((HGLOBAL)ResultAll);
	GlobalFree((HGLOBAL)OrigList);
	for(m=0;m<len/7;m++)
	{
		GlobalFree((HGLOBAL)ResultHistory[m]);
	}
	GlobalFree((HGLOBAL)ResultHistory);		
}

/*处理主窗口消息*/
BOOL CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HMENU hSysMenu=NULL;
	static MENUITEMINFO MinfoAbout,MinfoReportBug;
	static char *CBData=NULL;
	switch(message)
	{
		case WM_INITDIALOG:
			MainWindow=hwnd;
			hSysMenu=GetSystemMenu(hwnd,FALSE);
			MinfoAbout=InitMenu("关于...",IDSYSM_ABOUT);
			MinfoReportBug=InitMenu("技术支持及Bug报告...",IDSYSM_REPORTBUG);
			InsertMenuItemA(hSysMenu,6,TRUE,&MinfoAbout);
			InsertMenuItemA(hSysMenu,5,TRUE,&MinfoReportBug);
			SetIcon(hwnd);
			break;
		case WM_SYSCOMMAND:
			switch(wParam)
			{
				case IDSYSM_ABOUT:
					MessageBoxA(hwnd,"本软件著作权归Cystc所有，欢迎复制，传播，使用！\n作者博客：http://www.cystc.org/\n技术支持及Bug报告：http://www.cystc.org/?p=2375","版权信息",MB_ICONINFORMATION);
					break;
				case IDSYSM_REPORTBUG:
					ShellExecuteA(hwnd, NULL, "http://www.cystc.org/?p=2375", NULL, NULL, SW_SHOW);
					break;
				default:return DefWindowProc(hwnd,message,wParam,lParam);
			}
			break;
		case WM_COMMAND:		
			switch(LOWORD(wParam))
			{
				case IDC_CHECK:
					CheckSpamSW=IDC_CHECK;
					GetIpData(hwnd);					
					break;
				case IDC_CHKLOC:
					CheckSpamSW=IDC_CHKLOC;
					GetIpData(hwnd);
					break;
				case IDC_CBSEARCH:
					if(CBData=GetCbData(hwnd))
					{
						SearchAllIP(CBData,hwnd);
					}
					break;
				case IDC_CLEAN:
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