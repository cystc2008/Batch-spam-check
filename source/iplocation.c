/***** iplocation.c   
代码功能：把qq纯真ip数据库文件加载到共享内存里，通过参数查找出对应的所属的ip段，和地理位置，使用共享内存可以使查询一次在纳秒级。  
qq纯真ip数据库文件格式可以查看：http://lumaqq.linuxsir.org/article/qqwry_format_detail.html  
qq纯真ip数据库官网下载地址：http://www.cz88.net/fox/ipdat.shtml,需要安装，安装完后把qqwry.dat拷出即可，也可从网上找。  
 
本文件中部分代码由yifangyou提供  
*******/ 

#include "Batchspamchecker.h"
#define SHARE_MEMORY_FILE "qqwry.dat" //共享内存路径.ip库路径  
#define UNKNOWN "Unknown"  
#define SHARE_MEMORY_SIZE 10485760 //必须比ip库文件大    
#define RECORD_LEN 7 //单条记录长度  
//共享内存指针  
char *p_share=NULL;  
//第一条记录指针  
char *p_begin;  
char *p_end;  
//总记录数  
long total_record;  
 
//把4字节转为整数  
unsigned long getlong4(char *pos) //将读取的4个字节转化为长整型数  
{  
    unsigned long result=(((unsigned char )(*(pos+3)))<<24)  
     +(((unsigned char )(*(pos+2)))<<16)  
     +(((unsigned char )(*(pos+1)))<<8)  
     +((unsigned char )(*(pos)));  
    return result;  
}  
//把3字节转为整数  
unsigned long getlong3(char *pos) //将读取的3个字节转化为长整型数  
{  
    unsigned long result=(((unsigned char )(*(pos+2)))<<16)  
     +(((unsigned char )(*(pos+1)))<<8)  
     +((unsigned char )(*(pos)));  
    return result;  
}  
 
/**  
 * 创建共享内存，并加载ip库进去  
 *  
 * @return void  
 */ 
void createshare()  
{    
     long filesize=0;  
     FILE *fp=fopen(SHARE_MEMORY_FILE,"rb");  
     //读取文件长度
	 if(fp!=NULL)
	 {
		 fseek(fp,0,SEEK_END);  
		 filesize=ftell(fp);  
		 //归零  
		 fseek(fp,0,SEEK_SET);  
		 p_share = (char*)calloc(SHARE_MEMORY_SIZE,sizeof(char));     
		 //把文件内容读入共享内存  
		 fread(p_share,filesize,1,fp);  
		 fclose(fp);
	 }
}  
 
/**  
 * 打开共享内存指针  
 *  
 * @return void  
 */ 
void openshare() // map a normal file as shared mem:  
{  
	if(p_share==NULL)  
	{  
		//若是不存在则创建  
		createshare();      
	}  
	//第一条记录位置  
	p_begin=p_share+getlong4(p_share);  
	//最后一条记录位置  
	p_end=p_share+getlong4(p_share+4);  
	//记录总数  
	total_record=(getlong4(p_share+4)-getlong4(p_share))/RECORD_LEN;  
}  
 
/**  
 * 关闭共享内存指针  
 *  
 * @return void  
 */ 
void closeshare()  
{  
    free( p_share);      
}  
 
/**  
 * 返回地区信息  
 *  
 * @char *pos 地区的指针  
 * @return char *  
 */ 
char *getarea(char *pos) {  
        char *byte=pos; // 标志字节  
        pos++;  
        switch (*byte) {  
            case 0: // 没有区域信息  
                return UNKNOWN;  
                break;  
            case 1:  
            case 2: // 标志字节为1或2，表示区域信息被重定向  
                return p_share+getlong3(pos);  
                break;  
            default: // 否则，表示区域信息没有被重定向  
                return byte;  
                break;  
        }  
  }  
//获得ip所属地理信息,isp  
void getipinfo(char *ipstr,__out location *p_loc)  
{  
      char *pos = p_share;  
      //int record_len=10;  
      char *firstip=p_begin; // first record position  
	  char *byte;
      //把ip转为整数  
      unsigned long ip=htonl(inet_addr(ipstr));  
      long l=0;  
      long u=total_record;  
      long i=0;
	  //unsigned long j;
	  long offset;
      char* findip=firstip;  
      unsigned long beginip=0;  
      unsigned long endip=0;  
      //二分法查找  
      while(l <= u)  
      {  
           i=(l+u)/2;  
           pos=firstip+i*RECORD_LEN;  
           beginip = getlong4(pos);  
           pos+=4;  
           if(ip<beginip)  
           {  
				u=i-1;      
           }  
           else 
           {  
                endip=getlong4(p_share+getlong3(pos));  
                if(ip>endip)  
                {  
					l=i+1;          
                }  
                else 
                {  
					findip=firstip+i*RECORD_LEN;  
					break;      
                }  
           }  
      }  
      offset = getlong3(findip+4);  
      pos=p_share+offset;  
      endip= getlong4(pos); // 用户IP所在范围的结束地址  
      pos+=4;  
 
      //j=ntohl(beginip);  
      //inet_ntop(AF_INET,&j,p_loc->beginip, INET6_ADDRSTRLEN);// 获得开始地址的IP字符串类型  
      //j=ntohl(endip);  
      //inet_ntop(AF_INET,&j,p_loc->endip, INET6_ADDRSTRLEN);// 获得结束地址的IP字符串类型  
        
      byte = pos; // 标志字节  
      pos++;  
      switch (*byte) {  
            case 1:{ // 标志字节为1，表示国家和区域信息都被同时重定向  
                long countryOffset = getlong3(pos); // 重定向地址  
                pos+=3;  
                pos=p_share+countryOffset;  
                byte = pos; // 标志字节  
                pos++;  
                switch (*byte) {  
                    case 2: // 标志字节为2，表示国家信息又被重定向  
                    {  
                            p_loc->p_country=p_share+getlong3(pos);  
                            pos=p_share+countryOffset+4;  
							p_loc->p_area = getarea(pos);  
                    }  
                    break;  
                    default: // 否则，表示国家信息没有被重定向  
                    {  
						p_loc->p_country=byte;  
						p_loc->p_area = getarea(p_loc->p_country+strlen(p_loc->p_country)+1);  
                    }  
                    break;  
                }  
            }  
            break;  
            case 2: // 标志字节为2，表示国家信息被重定向  
            {  
                p_loc->p_country=p_share+getlong3(pos);  
                p_loc->p_area=p_share+offset+8;  
            }  
            break;  
            default:{ // 否则，表示国家信息没有被重定向  
                p_loc->p_country=byte;  
                p_loc->p_area=getarea(p_loc->p_country+strlen(p_loc->p_country)+1);  
            }  
            break;  
      }   
}