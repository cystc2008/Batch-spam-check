/*
Copyright (c) 2013 ����Ȩ��Cystc���С�����Ȩ�˱���һ��Ȩ����

�����Ȩ�����ʹ���߷��������������������£�����ʹ����ʹ�ü���ɢ����
�����װԭʼ�뼰����λ��ִ����ʽ��Ȩ�������۴˰�װ�Ƿ񾭸�����Ȼ��

���ڱ����Դ�������ɢ�������뱣�������İ�Ȩ���桢�����������У���
������������������
���ڱ��׼�����λ��ִ����ʽ����ɢ���������������ļ��Լ�������������
��ɢ����װ�е�ý�鷽ʽ����������֮��Ȩ���桢�����������У��Լ�����
������������
δ����ǰȡ��������ɣ�����ʹ��Cystc�����������֮���ƣ�
��Ϊ�����֮���������κα�ʾ֧�֡��Ͽɻ��ƹ㡢����֮��Ϊ��

�������������������Cystc�������֮����������״��"as is"���ṩ��
�������װ�����κ���ʾ��Ĭʾ֮�������Σ������������ھ��������Լ��ض�Ŀ
�ĵ�������ΪĬʾ�Ե�����Cystc�������֮�����ߣ������κ�������
���۳�����κ��������塢���۴�����Ϊ���Լ��ϵ���޹�ʧ������������Υ
Լ֮��Ȩ��������ʧ������ԭ��ȣ����𣬶����κ���ʹ�ñ������װ��������
�κ�ֱ���ԡ�����ԡ�ż���ԡ������ԡ��ͷ��Ի��κν�����𺦣�����������
�������Ʒ������֮���á�ʹ����ʧ��������ʧ��������ʧ��ҵ���жϵȵȣ���
�����κ����Σ����ڸ���ʹ���ѻ���ǰ��֪���ܻ���ɴ����𺦵���������Ȼ��
*/

#include "Batchspamchecker.h"

/*���������IP*/
char *IPData=NULL;

/*���������ھ��*/
HWND MainWindow=NULL;

/*�����ѯ�߳̾��*/
HANDLE hThread=NULL;

/*�������ͨ�����һ��IP*/
typedef struct
{
	char high[4],midhigh[4],midlow[4],low[4];
} IPGROUP;

/*�������ͨ�����һ����ٺ��IP*/
IPGROUP *IPs=NULL;

/*���ô���ͼ��*/
void __stdcall SetIcon(HWND hwnd)
{
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON2)));
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON3)));
}

/*��ʼ��ͨ�öԻ���*/
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

/*��ȡ�����ļ�·��*/
char* __stdcall GetSavePath(HWND hwnd)
{
	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn=InitCommonDlg("�ı��ļ���*.txt��\0*.txt\0",hwnd);
	if(GetSaveFileNameA(&ofn))
	{
		return ofn.lpstrFile;
	}
	return "";
}

/*�����д���ļ�*/
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

/*����ѡ�еĽ����������*/
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

/*�رս���Ի���*/
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

/*��ȡ�ؼ��������*/
POINT __stdcall GetClientPOS(HWND hwnd,HWND hCtrl,__out RECT* LPrectctrl)
{	
	POINT pos={0,0};
	GetWindowRect(hCtrl,LPrectctrl);
	pos.x=LPrectctrl->left;
	pos.y=LPrectctrl->top;
	ScreenToClient(hwnd,&pos);
	return pos;
}

/*��ȡ�ؼ��ߴ�*/
SIZE __stdcall GetCtrlSize(RECT rect)
{
	SIZE sz={0,0};
	sz.cx=rect.right-rect.left;
	sz.cy=rect.bottom-rect.top;
	return sz;
}

/*����������Ϣ*/
BOOL CALLBACK ResultProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD ExitCode=0;/*�����ѯ�̳߳�����*/
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
					/*���ݿؼ�λ�úʹ�С*/
					posCANCEL=GetClientPOS(hwnd,hIDCANCEL,&rectoldCANCEL);
					posSAVE=GetClientPOS(hwnd,hIDC_SAVE,&rectoldSAVE);
					posRESULT=GetClientPOS(hwnd,hIDC_RESULT,&rectoldRESULT);
					szCANCEL=GetCtrlSize(rectoldCANCEL);
					szSAVE=GetCtrlSize(rectoldSAVE);
					szRESULT=GetCtrlSize(rectoldRESULT);
					/*�޸Ŀؼ�λ�úʹ�С*/
					GetWindowRect(hwnd,&rect);										
					MoveWindow(hIDCANCEL,rect.right-89,rect.bottom-54,75,23,TRUE);
					MoveWindow(hIDC_SAVE,rect.left+10,rect.bottom-54,102,23,TRUE);
					MoveWindow(hIDC_RESULT,rect.left+10,rect.top+10,rect.right-20,rect.bottom-65,TRUE);
					break;
				case SIZE_RESTORED:
					/*��ԭ�ؼ�λ�úʹ�С*/
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

/*���IP��ַ������*/
char* __stdcall CheckIPAdress(char IPAddress[])
{
	SOCKET sSocket=INVALID_SOCKET;
	SOCKADDR_IN stSvrAddrIn={0}; /* �������˵�ַ */
	char sndBuf[1024]="";
	static char rcvBuf[2048]="";
	char *pRcv=rcvBuf;
	int num=0,nRet=SOCKET_ERROR;
	WSADATA wsaData;
	/*����������Ϣ*/
	sprintf(sndBuf, "GET http://www.stopforumspam.com/api?ip=%s\n\r\n",IPAddress);
	/* socket��ʼ�� */
	WSAStartup(MAKEWORD(2, 0), &wsaData);
	stSvrAddrIn.sin_family=AF_INET;
	stSvrAddrIn.sin_port=htons(80); 
	stSvrAddrIn.sin_addr.s_addr=inet_addr("195.20.205.9");
	sSocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/*����*/
	nRet=connect(sSocket,(SOCKADDR*)&stSvrAddrIn, sizeof(SOCKADDR));
	if(nRet==SOCKET_ERROR)
	{
		MessageBoxA(NULL,"����spam������ʧ�ܣ�","����",MB_ICONERROR|MB_SYSTEMMODAL);
		return NULL;/*���ӷ�����ʧ�ܷ���*/
	}
	/*����HTTP������Ϣ*/
	send(sSocket, (char*)sndBuf, sizeof(sndBuf), 0);
	/*����HTTP��Ӧ��Ϣ*/
	while(1)
	{
		num = recv(sSocket, pRcv, 2048, 0);
		pRcv += num;
		if(num==0||num==-1)
		{
			break ;
		}
	}
	/*������Ӧ��Ϣ*/
	return rcvBuf;
}

/*��ȡIP������*/
location __stdcall GetLocation(char IP[21])
{
	location loc={0};  
	openshare();
	getipinfo(IP,&loc);  
	return loc;
}

/*������*/
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
			sprintf(msg,"ip %s   �ں�������   %s %s",IPAddress,loc.p_country,loc.p_area );		
		}
		else if(result[53]=='n')
		{
			sprintf(msg,"ip %s   ���ں�����   %s %s",IPAddress,loc.p_country,loc.p_area );
		}
		if(ResultDlg!=NULL)
		{
			SendDlgItemMessageA(ResultDlg,IDC_RESULT,LB_ADDSTRING,0,(LPARAM)msg);			
		}
	}
	else if(++Error>=2)/*����������ֹ�߳�ִ��*/
	{		
		if(GetExitCodeThread(GetCurrentThread(),&ExitCode))
		{
			ExitThread(ExitCode);
		}
	}
}

/*ö�ٰ���*�ŵ�IP�����п���*/
void __stdcall EnumTPF(IPGROUP IPGroup,HWND ResultDlg)
{
	unsigned int i[4]={0,0,0,0};
	DWORD ExitCode=0;
	unsigned long Number=260,Num=0;
	unsigned int j,j1,k,l,m,n;
	char Buf[4]={0,0,0,0};
	char IPResult[21];
	/*����IP�а�������*����ȷ��ö�ٺ��IP������������ڷ����ڴ�ռ�*/
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
	/*��IP��������*/
	if(Number>16581375)
	{
		MessageBoxA(NULL,"�޷�һ�β�ѯ��ô��IP","����",MB_ICONERROR);
		if(GetExitCodeThread(GetCurrentThread(),&ExitCode))
		{
			ExitThread(ExitCode);
		}
	}
	/*��ʼö��*/
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
		/*���˵�ö�ٺ����*�ŵ�IPȻ���ύ*/
		for(n=0;n<Num;n++)
		{
			if(IPs[n].high[0]!='*'&&IPs[n].midhigh[0]!='*'&&IPs[n].midlow[0]!='*'&&IPs[n].low[0]!='*')
			{
				sprintf(IPResult,"%s.%s.%s.%s",IPs[n].high,IPs[n].midhigh,IPs[n].midlow,IPs[n].low);
				PrintResult(IPResult,CheckIPAdress(IPResult),ResultDlg);
			}
		}
		/*�ͷ��ڴ�*/
		GlobalFree((HGLOBAL)IPs);
		IPs=NULL;
	}
}

/*��һ��IP�����char*.char*.char*.char*�ĸ�ʽ*/
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

/*���д����û������IP�Ա����ύ*/
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
			if(IPData[m]==' ')/*�Զ����Կո�*/
			{
				n--;
				continue;
			}
			IPAddress[n]=IPData[m];	
		}
		IPAddress[n]='\0';
		for(i=0;IPAddress[i]!='\0';i++)/*���IP��ͨ���*/
		{
			if(IPAddress[i]=='*'/*||IPAddress[i]=='?'*/)
			{
				trimIP(IPAddress,ResultDlg);
				break;
			}
		}
		if(IPAddress[i]=='\0')/*���IP����ͨ���*/
		{
			if(IPAddress[i-1]=='\r')/*ȥ���س���*/
			{
				IPAddress[i-1]='\0';
			}
			PrintResult(IPAddress,CheckIPAdress(IPAddress),ResultDlg);
		}
		m++;		
	}
}

/*��ȡ�û������IP��ַ*/
void __stdcall GetIpData(HWND hwnd)
{
	int len = GetWindowTextLengthA(GetDlgItem(hwnd, IP_LIST));
	static HWND ResultDlg=NULL;
	if(len > 0)
	{
		IPData=(char*)GlobalAlloc(GPTR,len+2);
		/*��ȡ�û���������*/
		GetDlgItemTextA(hwnd, IP_LIST, IPData,len+1);
		/*�򵥼���û����������Ƿ�Ϸ�*/
		if(((IPData[0]>=48&&IPData[0]<=57)||IPData[0]=='*')&&((IPData[len-1]>=48&&IPData[len-1]<=57)||IPData[len-1]=='*'||IPData[len-1]==13||IPData[len-1]==10||IPData[len-1]==32)&&(IPData[1]=='.'||IPData[2]=='.'||IPData[3]=='.'))
		{
			/*��Ϸ�*/						
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
			/*�粻�Ϸ�*/
			GlobalFree((HGLOBAL)IPData);
			IPData=NULL;
			MessageBoxA(hwnd,"������Ĳ���IP��ַ��","����",MB_ICONERROR);
		}
	}
}

/*������������Ϣ*/
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
					MessageBoxA(hwnd,"���������Ȩ��Cystc���У���ӭ���ƣ�������ʹ�ã�","��Ȩ��Ϣ",MB_ICONINFORMATION);
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

/*�������*/
int WINAPI WinMain(HINSTANCE hThisInstance,HINSTANCE hPrevInstance,LPSTR lpszArgument,int nFunsterStil)
{
	DialogBoxA(GetModuleHandleA(NULL),MAKEINTRESOURCEA(MAIN_GUI),0,WindowProc);
	closeshare();
	return TRUE;
}