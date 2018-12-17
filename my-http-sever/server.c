#include "server.h"
#include "status.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>



struct request_info{
    int status;
    char path[128];
    char *content;
    char return_type[50];
    bool dir_flag;
};

void substring(char s[], char sub[], int p, int l) {
   int c = 0;
   
   while (c < l) {
      sub[c] = s[p+c-1];
      c++;
   }
   sub[c] = '\0';
}
bool streq(char* str1,char* str2){
    if(strlen(str1) != strlen(str2)) return false;
    for(int i=0;i<strlen(str1);i++){
        if(str1[i] != str2[i]) return false;
    }
    return true;
}

int open_file(char* path ,char **buffer){
  FILE * pFile;
  long lSize;
  size_t result;
  pFile = fopen ( path , "rb" );
  if (pFile==NULL) return -1;
  fseek (pFile , 0 , SEEK_END);
  lSize = ftell (pFile);
  rewind (pFile);
  *buffer = (char*) malloc (sizeof(char)*lSize);
  if (*buffer == NULL) return -2;
  result = fread (*buffer,1,lSize,pFile);
  if (result != lSize) return -3;

  fclose (pFile);
  return 0;
}
bool open_dir(char* path,struct request_info* info){
    char new_path[256];
    strcpy(new_path,".");
    strcat(new_path,path);
    DIR* dir = opendir(new_path);
    if (dir){
        closedir(dir);
        return true;  
    }
    return false;
}
void string_paser(char* request,struct request_info* info){
    char *delim = " ";
    char * pch;
    char * path;
    char * pch2;
    char  *pch3 = malloc(128);
    char * file_content;
    int space = 0;
    bool method_flag = false;
    pch = strtok(request,delim);
    if(!(pch[0] == 'G' && pch[1] == 'E' && pch[2] == 'T')) {
        method_flag = true;
    }
    pch = strtok (NULL, delim);
    if(pch[0] != '/'){
        info->status = status_code[BAD_REQUEST];
        return;
    }
    if(method_flag == true){
        info->status = status_code[METHOD_NOT_ALLOWED];
        return; 
    }
    strcpy (info->path,pch);
    info->dir_flag = false;
    if(open_dir(info->path,info)){
        info->status = status_code[OK];
        info->dir_flag = true;
        return;
    }
    strcpy(pch3,pch);
    int check_dir = true;
    for(int i=0;i<strlen(pch3);i++){
        if(pch3[i] == '.') check_dir = false;
    }
    if(check_dir){
        info->status = status_code[NOT_FOUND];
        return;
    }
    while(*pch3 != '.') pch3 = pch3 + 1;
    pch3 = pch3 + 1 ;
    bool flag = false;
    for(int i=0;i<8;i++){
        if(streq(pch3,extensions[i][0])){
            strcpy (info->return_type,extensions[i][1]);
            flag = true;
        }
    }
    if(flag == false){
        info->status = status_code[UNSUPPORT_MEDIA_TYPE];
        return;
    }
    int check = open_file(pch+1 ,&file_content);
    if(check == -1){
        info->status = status_code[NOT_FOUND];
        return;
    }
    info->content = malloc(strlen(file_content)+1);
    strcpy(info->content,file_content);
    info->status = status_code[OK];
    return;  
}

int get_index(struct request_info* info){
    for(int i=0;i<5;i++){
        if(info->status == status_code[i]) return i;
    }
    return -1;
}

void response(struct request_info* info,char** response){
    char* stat[] = {
        "OK" ,
        "BAD_REQUEST",
        "NOT_FOUND",
        "METHOD_NOT_ALLOWED",
        "UNSUPPORT_MEDIA_TYPE"  
    };
    if(info->status != 200){
        int someInt = info->status;
        char int_status[12];
        sprintf(int_status, "%d", someInt);
        char *head  = "HTTP/1.x ";
        int index = get_index(info);
        char *str_status = stat[index];
        char *mid = "\r\nContent-Type: ";
        char* type = info->return_type;
        char* end = "\r\nServer: httpserver/1.x\r\n\r\n";
        *response = malloc(strlen(head)+strlen(int_status)+strlen(str_status)+strlen(mid)+strlen(end)+6);
        strcpy(*response,head);
        strcat(*response,int_status);
        strcat(*response," ");
        strcat(*response,str_status);
        strcat(*response,mid);
        strcat(*response,end);
    }
    else if(info->dir_flag){
        int someInt = info->status;
        char int_status[12];
        sprintf(int_status, "%d", someInt);
        char *head  = "HTTP/1.x ";
        int index = get_index(info);
        char *str_status = stat[index];
        char *mid = "\r\nContent-Type: directioy";
        char* type = info->return_type;
        char* end = "\r\nServer: httpserver/1.x\r\n\r\n";
        *response = malloc(strlen(head)+strlen(int_status)+strlen(str_status)+strlen(mid)+strlen(end)+6);
        strcpy(*response,head);
        strcat(*response,int_status);
        strcat(*response," ");
        strcat(*response,str_status);
        strcat(*response,mid);
        strcat(*response,end);
    }
    else{
        int someInt = info->status;
        char int_status[12];
        sprintf(int_status, "%d", someInt);
        char *head  = "HTTP/1.x ";
        int index = get_index(info);
        char *str_status = stat[index];
        char *mid = "\r\nContent-Type: ";
        char* type = info->return_type;
        char* end = "\r\nServer: httpserver/1.x\r\n\r\n";
        char* content = info->content;
        *response = malloc(strlen(head)+strlen(int_status)+strlen(str_status)+strlen(type)+strlen(mid)+strlen(end)+strlen(content)+8);
        strcpy(*response,head);
        strcat(*response,int_status);
        strcat(*response," ");
        strcat(*response,str_status);
        strcat(*response,mid);
        strcat(*response,type);

        strcat(*response,end);
        strcat(*response,content);
    }
}

bool isDir(char* name){
    for(int i=0;i<strlen(name);i++){
        if(name[i] == '.') return false;
    }
    return true;
}

void child(int forClientSockfd){
    char inputBuffer[512] = {};
    struct request_info info;
    char* res;
    char* res2;
    char* create_path = malloc(256);
    char* pch;
    char temp[256];
    char temp_for_w[512];
    recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0);
    string_paser(inputBuffer,&info);
    response(&info,&res);
    if(info.status == 200){
        DIR* dir = opendir("output");
        if (dir){
            closedir(dir);
        }
            else{
            system("mkdir output");
        }
        if(info.dir_flag){
            strcpy(create_path,"mkdir -p output/");
            strcat(create_path,info.path+1);
            system(create_path);
        }
        else{
            int current = 0;
            strcpy(create_path,"mkdir -p output/");
            for(int i=0;i<strlen(info.path);i++){
                if(info.path[i] == '/') current = i;
            }
            for(int i=1;i<current;i++){
                temp[i-1] = info.path[i];
            }
            temp[current-1] = '\0';
            strcat(create_path,temp);
            system(create_path);
            FILE *pFile;
            strcpy(temp_for_w,"output/");
            strcat(temp_for_w,info.path+1);
            pFile = fopen(temp_for_w,"w" );
            if( NULL == pFile ){
                printf( "open failure" );
            }
            else{
                fwrite(info.content,1,strlen(info.content),pFile);
            }
            fclose(pFile);
        }
    }
    send(forClientSockfd,res,strlen(res),0);
}
int main(int argc , char *argv[])
{
    int port;
    int thread_num;
    for(int i=0;i<argc;i++){
        if(argv[i][0] == '-' && argv[i][1] == 'p') port = atoi(argv[i+1]);
        if(argv[i][0] == '-' && argv[i][1] == 'n') thread_num = atoi(argv[i+1]);
    }
    pthread_t t[thread_num];
    char inputBuffer[256] = {};
    char message[] = {"Hi,this is server.\n"};
    int sockfd = 0,forClientSockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    char *tmp;
    int sta;
    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));
    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(port);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);
    struct request_info info;
    char *res;
    
    while(1){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
        child(forClientSockfd);
    }
    return 0;
}


