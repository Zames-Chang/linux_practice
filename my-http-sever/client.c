#include "client.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>

struct whatever{
    char path[256];
    char ip[128];
    int port;
};


bool streq(char* str1,char* str2){
    if(strlen(str1) != strlen(str2)) return false;
    for(int i=0;i<strlen(str1);i++){
        if(str1[i] != str2[i]) return false;
    }
    return true;
}

void *foo(void* arg){
    struct whatever *a = (struct whatever *) arg;
    char path[256];
    char ip[128];
    int port;
    strcpy(path,a->path);
    strcpy(ip,a->ip);
    port = a->port;
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    info.sin_addr.s_addr = inet_addr(ip);
    info.sin_port = htons(port);


    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error");
    }

    char message[512] = "GET ";
    strcat(message,path);
    strcat(message," HTTP/1.x\r\nHOST: LOCALHOST:PORT \r\n\r\n");
    char receiveMessage[4096] = {};
    send(sockfd,message,sizeof(message),0);
    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
    printf("%s",receiveMessage);
    close(sockfd);  
}

void DFS(char path[256],char ip[128],int port){
    pthread_t t;
    struct whatever data;
    data.port = port;
    strcpy(data.ip,ip);
    strcpy(data.path,path);
    pthread_create(&t, NULL, foo, (void*) &data);
    pthread_join(t, NULL);
    char new_path[256];
    struct dirent * ptr;
    strcpy(new_path,".");
    strcat(new_path,path);
    DIR* dir = opendir(new_path);
    char mytemp[256];
    if (dir){
        while((ptr = readdir(dir)) != NULL)
        {
            if(DT_DIR == ptr->d_type){
                if(!(streq(ptr->d_name,".") || streq(ptr->d_name,".."))){
                    strcpy(mytemp,path);
                    strcat(path,"/");
                    strcat(path,ptr->d_name);
                    DFS(path,ip,port);
                    strcpy(path,mytemp);
                }
            }
            else if(DT_REG == ptr->d_type){
                strcpy(mytemp,path);
                strcat(path,"/");
                strcat(path,ptr->d_name);
                DFS(path,ip,port);
                strcpy(path,mytemp);
            }
        }
    }
    return;
}


int main(int argc , char *argv[])
{
    char path[256];
    char ip[128];
    int port;
    for(int i=0;i<argc;i++){
        if(argv[i][0] == '-' && argv[i][1] == 't') strcpy(path,argv[i+1]);
        if(argv[i][0] == '-' && argv[i][1] == 'h') strcpy(ip,argv[i+1]);
        if(argv[i][0] == '-' && argv[i][1] == 'p') port = atoi(argv[i+1]);
    }
    DFS(path,ip,port);
    return 0;
}

