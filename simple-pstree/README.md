# HW1-simple-pstree
```c
struct messager {
    int white_space[128];
    int array_len;
    char name[128][64];
    int pid[128];
    int isNULL;
};
```
messager 是一個我回傳kernel data 的資料格式
因為我的name 只宣告給他128個位子，所以如果kernel process > 128的時候就會有一些資料沒有被傳回來
# linux_practice
