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
        /* 子进程 */
        char pp[5]="pong";
        char name[5];
        
        read(ping[0],name,5);
        printf("%d: received %s\n",getpid(),name);
        //close(ping[0]); // 读取完成，关闭读端

        //close(pong[0]);
        write(pong[1],pp,5);
        //close(pong[1]);
        
        
    } else if (ret>0) { 
        /* 父进程 */
        char pp[5] ="ping";
        char name[5];
        //close(pong[1]); // 关闭写端
        write(ping[1],pp,5);
        read(pong[0],name,5);
        printf("%d: received %s\n",getpid(),name);
        //close(pong[0]); // 读取完成，关闭读端

        //close(ping[0]); // 关闭读端
        
        //close(ping[1]); // 写入完成，关闭写端
    }
    exit(0);
}