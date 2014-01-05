#define _CRT_SECURE_NO_DEPRECATE 
#include <stdio.h> 
#include <stdlib.h>
#include <WindowsX.h>
#include <fcntl.h>  
#include <sys/types.h>  
#include <sys/stat.h>   
#include <string.h> 
#include <errno.h>
#include <math.h>     
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include "resource.h"
#include "ptresult.h"

typedef struct   
{  
    char *p_country;  
    char *p_area;  
    char beginip[INET6_ADDRSTRLEN]; // 用户IP所在范围的开始地址  
    char endip[INET6_ADDRSTRLEN]; // 用户IP所在范围的结束地址  
}location;

void getipinfo(char *ipstr,location *p_loc);
void openshare();
void closeshare();