#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NETLINK_USER 31
struct messager {
    int white_space[128];
    int array_len;
    char name[128][64];
    int pid[128];
    int isNULL;
};
struct send {
    pid_t id;
    int flag;
};
#define MAX_PAYLOAD 16258 /* maximum payload size*/
struct sockaddr_nl src, dest;
struct nlmsghdr *msgData = NULL;
struct iovec iov;
int mySocket;
struct msghdr msg;

int main(int argc,char* argv[])
{
    mySocket=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    memset(&src, 0, sizeof(src));
    src.nl_family = AF_NETLINK;
    src.nl_pid = getpid(); /* pid */
    int ptr;
    struct send searchPid;
    searchPid.flag = -1;
    searchPid.id = -1;
    if(argc == 1) {
        searchPid.flag = 1;
        searchPid.id = 1;
    } else {
        for(int i=0; i<argc; i++) {
            if(*(argv[i]) == '-') {
                if(*(argv[i] + 1) == 'p') {
                    searchPid.flag = 0;
                    if(*(argv[i]+2) == '\0') searchPid.id = src.nl_pid;
                    else  searchPid.id = atoi(argv[i] + 2);
                } else if(*(argv[i] + 1) == 'c') {
                    searchPid.flag = 1;
                    if(*(argv[i]+2) == '\0') searchPid.id = 1;
                    else  searchPid.id = atoi(argv[i] + 2);
                } else if(*(argv[i] + 1) == 's') {
                    searchPid.flag = 2;
                    if(*(argv[i]+2) == '\0') searchPid.id = src.nl_pid;
                    else  searchPid.id = atoi(argv[i] + 2);
                }
            }
        }
    }
    if(searchPid.flag < 0 || searchPid.id < 0) return 0;

    bind(mySocket, (struct sockaddr*)&src, sizeof(src));

    memset(&dest, 0, sizeof(dest));
    memset(&dest, 0, sizeof(dest));
    dest.nl_family = AF_NETLINK;
    dest.nl_groups = 0;
    dest.nl_pid = 0;


    msgData = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(msgData, 0, NLMSG_SPACE(MAX_PAYLOAD));
    msgData->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    msgData->nlmsg_pid = getpid();
    msgData->nlmsg_flags = 0;
    int msg_size = sizeof(searchPid);
    memcpy(NLMSG_DATA(msgData),&searchPid,msg_size);

    iov.iov_base = (void *)msgData;
    iov.iov_len = msgData->nlmsg_len;
    msg.msg_name = (void *)&dest;
    msg.msg_namelen = sizeof(dest);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sendmsg(mySocket,&msg,0);


    /* Read message from kernel */
    recvmsg(mySocket, &msg, 0);
    struct messager* x = (struct messager *)NLMSG_DATA(msgData);
    if(!(x->isNULL)) {
        if(searchPid.flag == 0) {
            for(int i=0; i<x->array_len; i++) {
                for(int j=0; j<i; j++) printf("    ");
                printf("%s(%d)\n",x->name[i],x->pid[i]);
            }
        } else if (searchPid.flag == 1) {
            for(int i=0; i<x->array_len; i++) {
                for(int j=0; j<x->white_space[i]; j++) printf(" ");
                printf("%s(%d)\n",x->name[i],x->pid[i]);
            }
        } else if (searchPid.flag == 2) {
            for(int i=0; i<x->array_len; i++) {
                printf("%s(%d)\n",x->name[i],x->pid[i]);
            }
        }
    }
    close(mySocket);
}