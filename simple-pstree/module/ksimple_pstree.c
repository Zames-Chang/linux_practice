//Taken from https://stackoverflow.com/questions/15215865/netlink-sockets-in-c-using-the-3-x-linux-kernel?lq=1

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/kernel.h>
#include <linux/init.h>
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
struct sock *myNetlinkSocket = NULL;

void parentDFS(struct task_struct* task,struct messager* result,int *i)
{
    struct task_struct *parent;
    int mylen = strlen(task->comm);
    int j;
    char temp;
    if(task->pid == 1) {
        for(j=0; j<mylen; j++) {
            temp = *(task->comm + j);
            result->name[*i][j] = temp;
        }
        result->name[*i][mylen] = '\0';
        result->pid[*i] = task->pid;
    } else {
        parentDFS(task->real_parent,result,i);
        (*i)++;
        for(j=0; j<mylen; j++) {
            temp = *(task->comm + j);
            result->name[*i][j] = temp;
        }
        result->name[*i][mylen] = '\0';
        result->pid[*i] = task->pid;
    }
}
void child(struct task_struct* task,struct messager* result,int *i)
{
    struct task_struct *child;
    struct list_head *list;
    list_for_each(list, &task->children) {
        child = list_entry(list, struct task_struct, sibling);
        int mylen = strlen(child->comm);
        int j;
        char temp;
        for(j=0; j<mylen; j++) {
            temp = *(child->comm + j);
            result->name[*i][j] = temp;
        }
        result->name[*i][mylen] = '\0';
        result->pid[*i] = child->pid;
        (*i)++;
    }
}
void childDFS(struct task_struct* task,struct messager* result ,int white_space,int* i)
{
    struct task_struct *child;
    struct list_head *list;
    result->white_space[*i] = white_space;
    int mylen = strlen(task->comm);
    int j;
    char temp;
    for(j=0; j<mylen; j++) {
        temp = *(task->comm + j);
        result->name[*i][j] = temp;
    }
    result->name[*i][mylen] = '\0';
    result->pid[*i] = task->pid;
    (*i)++;
    list_for_each(list, &task->children) {
        child = list_entry(list, struct task_struct, sibling);
        childDFS(child,result,white_space+4,i);
    }
}

static void kSimpleTree(struct sk_buff *mySocket)
{

    struct nlmsghdr *msgData;
    int pid;
    struct sk_buff *mySocket_out;
    int msg_size;
    struct messager msg;
    int count_len = 0;
    int res;
    msgData=(struct nlmsghdr*)mySocket->data;
    // mycode process pid
    struct pid *pid_struct;
    struct task_struct* current_task;
    char* buffer;
    pid_t tmp;
    pid_t client_send;
    struct send * mymsg= (struct send*) nlmsg_data(msgData);
    client_send = mymsg->id;
    pid_struct = find_get_pid(client_send);
    if(!pid_struct) {
        msg.isNULL = 1;
    } else {
        msg.isNULL = 0;
        current_task = pid_task(pid_struct,PIDTYPE_PID);
        if(mymsg->flag == 0) {
            parentDFS(current_task,&msg,&count_len);
            msg.array_len = count_len+1;
        } else if (mymsg->flag == 1) {
            childDFS(current_task,&msg,0,&count_len);
            msg.array_len = count_len;
        } else if(mymsg->flag == 2) {
            child(current_task,&msg,&count_len);
            msg.array_len = count_len;
        }
    }

    msg_size = sizeof(msg);
    pid = msgData->nlmsg_pid; /*pid of sending process */

    mySocket_out = nlmsg_new(msg_size,0);

    if(!mySocket_out) {

        printk(KERN_ERR "Failed\n");
        return;

    }
    msgData=nlmsg_put(mySocket_out,0,0,NLMSG_DONE,msg_size,0);
    NETLINK_CB(mySocket_out).dst_group = 0;
    memcpy(nlmsg_data(msgData),&msg,msg_size);

    res=nlmsg_unicast(myNetlinkSocket,mySocket_out,pid);

    if(res<0)
        printk(KERN_INFO "Error sending\n");
}

static int __init myInit(void)
{

    struct netlink_kernel_cfg cfg = {
        .input = kSimpleTree,
    };

    myNetlinkSocket = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if(!myNetlinkSocket) {

        printk(KERN_ALERT "Error creating\n");
        return -10;

    }

    return 0;
}

static void __exit myExit(void)
{

    netlink_kernel_release(myNetlinkSocket);
}

module_init(myInit);
module_exit(myExit);

MODULE_LICENSE("GPL");