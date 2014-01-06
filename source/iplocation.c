/***** iplocation.c   
���빦�ܣ���qq����ip���ݿ��ļ����ص������ڴ��ͨ���������ҳ���Ӧ��������ip�Σ��͵���λ�ã�ʹ�ù����ڴ����ʹ��ѯһ�������뼶��  
qq����ip���ݿ��ļ���ʽ���Բ鿴��http://lumaqq.linuxsir.org/article/qqwry_format_detail.html  
qq����ip���ݿ�������ص�ַ��http://www.cz88.net/fox/ipdat.shtml,��Ҫ��װ����װ����qqwry.dat�������ɣ�Ҳ�ɴ������ҡ�  
 
���ļ��в��ִ�����yifangyou�ṩ  
*******/ 

#include "Batchspamchecker.h"
#define SHARE_MEMORY_FILE "qqwry.dat" //�����ڴ�·��.ip��·��  
#define UNKNOWN "Unknown"  
#define SHARE_MEMORY_SIZE 10485760 //�����ip���ļ���    
#define RECORD_LEN 7 //������¼����  
//�����ڴ�ָ��  
char *p_share=NULL;  
//��һ����¼ָ��  
char *p_begin;  
char *p_end;  
//�ܼ�¼��  
long total_record;  
 
//��4�ֽ�תΪ����  
unsigned long getlong4(char *pos) //����ȡ��4���ֽ�ת��Ϊ��������  
{  
    unsigned long result=(((unsigned char )(*(pos+3)))<<24)  
     +(((unsigned char )(*(pos+2)))<<16)  
     +(((unsigned char )(*(pos+1)))<<8)  
     +((unsigned char )(*(pos)));  
    return result;  
}  
//��3�ֽ�תΪ����  
unsigned long getlong3(char *pos) //����ȡ��3���ֽ�ת��Ϊ��������  
{  
    unsigned long result=(((unsigned char )(*(pos+2)))<<16)  
     +(((unsigned char )(*(pos+1)))<<8)  
     +((unsigned char )(*(pos)));  
    return result;  
}  
 
/**  
 * ���������ڴ棬������ip���ȥ  
 *  
 * @return void  
 */ 
void createshare()  
{    
     long filesize=0;  
     FILE *fp=fopen(SHARE_MEMORY_FILE,"rb");  
     //��ȡ�ļ�����
	 if(fp!=NULL)
	 {
		 fseek(fp,0,SEEK_END);  
		 filesize=ftell(fp);  
		 //����  
		 fseek(fp,0,SEEK_SET);  
		 p_share = (char*)calloc(SHARE_MEMORY_SIZE,sizeof(char));     
		 //���ļ����ݶ��빲���ڴ�  
		 fread(p_share,filesize,1,fp);  
		 fclose(fp);
	 }
}  
 
/**  
 * �򿪹����ڴ�ָ��  
 *  
 * @return void  
 */ 
void openshare() // map a normal file as shared mem:  
{  
	if(p_share==NULL)  
	{  
		//���ǲ������򴴽�  
		createshare();      
	}  
	//��һ����¼λ��  
	p_begin=p_share+getlong4(p_share);  
	//���һ����¼λ��  
	p_end=p_share+getlong4(p_share+4);  
	//��¼����  
	total_record=(getlong4(p_share+4)-getlong4(p_share))/RECORD_LEN;  
}  
 
/**  
 * �رչ����ڴ�ָ��  
 *  
 * @return void  
 */ 
void closeshare()  
{  
    free( p_share);      
}  
 
/**  
 * ���ص�����Ϣ  
 *  
 * @char *pos ������ָ��  
 * @return char *  
 */ 
char *getarea(char *pos) {  
        char *byte=pos; // ��־�ֽ�  
        pos++;  
        switch (*byte) {  
            case 0: // û��������Ϣ  
                return UNKNOWN;  
                break;  
            case 1:  
            case 2: // ��־�ֽ�Ϊ1��2����ʾ������Ϣ���ض���  
                return p_share+getlong3(pos);  
                break;  
            default: // ���򣬱�ʾ������Ϣû�б��ض���  
                return byte;  
                break;  
        }  
  }  
//���ip����������Ϣ,isp  
void getipinfo(char *ipstr,__out location *p_loc)  
{  
      char *pos = p_share;  
      //int record_len=10;  
      char *firstip=p_begin; // first record position  
	  char *byte;
      //��ipתΪ����  
      unsigned long ip=htonl(inet_addr(ipstr));  
      long l=0;  
      long u=total_record;  
      long i=0;
	  //unsigned long j;
	  long offset;
      char* findip=firstip;  
      unsigned long beginip=0;  
      unsigned long endip=0;  
      //���ַ�����  
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
      endip= getlong4(pos); // �û�IP���ڷ�Χ�Ľ�����ַ  
      pos+=4;  
 
      //j=ntohl(beginip);  
      //inet_ntop(AF_INET,&j,p_loc->beginip, INET6_ADDRSTRLEN);// ��ÿ�ʼ��ַ��IP�ַ�������  
      //j=ntohl(endip);  
      //inet_ntop(AF_INET,&j,p_loc->endip, INET6_ADDRSTRLEN);// ��ý�����ַ��IP�ַ�������  
        
      byte = pos; // ��־�ֽ�  
      pos++;  
      switch (*byte) {  
            case 1:{ // ��־�ֽ�Ϊ1����ʾ���Һ�������Ϣ����ͬʱ�ض���  
                long countryOffset = getlong3(pos); // �ض����ַ  
                pos+=3;  
                pos=p_share+countryOffset;  
                byte = pos; // ��־�ֽ�  
                pos++;  
                switch (*byte) {  
                    case 2: // ��־�ֽ�Ϊ2����ʾ������Ϣ�ֱ��ض���  
                    {  
                            p_loc->p_country=p_share+getlong3(pos);  
                            pos=p_share+countryOffset+4;  
							p_loc->p_area = getarea(pos);  
                    }  
                    break;  
                    default: // ���򣬱�ʾ������Ϣû�б��ض���  
                    {  
						p_loc->p_country=byte;  
						p_loc->p_area = getarea(p_loc->p_country+strlen(p_loc->p_country)+1);  
                    }  
                    break;  
                }  
            }  
            break;  
            case 2: // ��־�ֽ�Ϊ2����ʾ������Ϣ���ض���  
            {  
                p_loc->p_country=p_share+getlong3(pos);  
                p_loc->p_area=p_share+offset+8;  
            }  
            break;  
            default:{ // ���򣬱�ʾ������Ϣû�б��ض���  
                p_loc->p_country=byte;  
                p_loc->p_area=getarea(p_loc->p_country+strlen(p_loc->p_country)+1);  
            }  
            break;  
      }   
}