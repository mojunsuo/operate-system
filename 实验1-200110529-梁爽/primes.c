#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[])
{
    int ex[2];
    int pipe2[2];
    int pipe3[2];
    int pipe5[2];
    pipe(ex);
    pipe(pipe2);
    pipe(pipe3);
    pipe(pipe5);
    if(fork()==0)
    {
        int j;
        close(pipe2[1]);
        char buf[sizeof(int)];
        while(read(pipe2[0],buf,sizeof(int)))
        {
            j=*((int*)buf);
            
            if(j%2!=0||j==2)
            {
                write(pipe3[1],(char*)&j,sizeof(int));
            }
        }
        
        close(pipe3[1]);
        if(fork()==0)
        {
            int k;
            char buf2[sizeof(int)];
            while(read(pipe3[0],buf2,sizeof(int)))
            {
                k=*((int*)buf2);
                
                if(k%3!=0||k==3)
                {
                    write(pipe5[1],(char*)&k,sizeof(int));
                }
            }
            close(pipe5[1]);
            if(fork()==0)
            {
                int q;
                char buf3[sizeof(int)];
                while(read(pipe5[0],buf3,sizeof(int)))
                {
                    q=*((int*)buf3);
                    
                    if(q%5!=0||q==5)
                    {
                        printf("prime %d\n",q);
                    }
                    
                }
                write(ex[1],"ok",3);
                
            }
            else{
                
            }
        }
        else{
            close(pipe5[1]);
        }
        close(pipe5[1]);
    }
    else{
        close(pipe5[1]);
        close(pipe3[1]);
        int i=2;
        for(i=2;i<=35;i++)
        {
            write(pipe2[1],(char*)&i,sizeof(int));
        }
        close(pipe2[1]);
        char buf[3];
        read(ex[0],buf,3);
        printf("$\n");
    }
    exit(0);
}