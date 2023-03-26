#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[])
{
    int ping[2];
    int pong[2];
    pipe(ping);
    pipe(pong);
    int ret=fork();
    
    
    if (ret == 0) { 
        /* �ӽ��� */
        char pp[5]="pong";
        char name[5];
        
        read(ping[0],name,5);
        printf("%d: received %s\n",getpid(),name);
        //close(ping[0]); // ��ȡ��ɣ��رն���

        //close(pong[0]);
        write(pong[1],pp,5);
        //close(pong[1]);
        
        
    } else if (ret>0) { 
        /* ������ */
        char pp[5] ="ping";
        char name[5];
        //close(pong[1]); // �ر�д��
        write(ping[1],pp,5);
        read(pong[0],name,5);
        printf("%d: received %s\n",getpid(),name);
        //close(pong[0]); // ��ȡ��ɣ��رն���

        //close(ping[0]); // �رն���
        
        //close(ping[1]); // д����ɣ��ر�д��
    }
    exit(0);
}