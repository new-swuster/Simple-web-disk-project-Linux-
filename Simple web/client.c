#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <dirent.h>

// 客户端
// 创建套结字
// 通信
// 关闭

void processhelp() {
    printf("help: 展示帮助信息\n");
    printf("list: 列出储存在服务端的文件名\n");
    printf("get: get + 服务端的文件名（下载服务端文件到客户端）\n");
    printf("put: put + 客户端的文件名（上传客户端文件到服务端）\n");
    printf("delete: delete + 服务端文件名（删除服务端的文件）\n");
    printf("history: history（查看历史）\n");
    printf("exit: exit（退出程序）\n");
}

int processlist(struct sockaddr_in saddr) {
    int ret;
    char buf[128] = { 0 };
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        perror("socket");
        return -1;
    }
    ret = connect(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (-1 == ret) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    ret = write(sockfd, "L", 1);
    if (-1 == ret) {
        perror("write L");
        close(sockfd);
        return -1;
    }
    ret = read(sockfd, buf, 128);
    if (-1 == ret) {
        perror("write L");
        close(sockfd);
        return -1;
    }
    printf("%s\n", buf);
    close(sockfd);
    return 0;
}

int processput(struct sockaddr_in saddr, char* file_name) {
    int n;
    char buf[128] = { 0 };
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        perror("socket");
        return -1;
    }
    int ret = connect(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (-1 == ret) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    int fd = open(file_name, O_RDONLY);
    if (-1 == fd) {
        perror("open");
        close(sockfd);
        return -1;
    }
    // 发送上传命令和文件名
    sprintf(buf, "P%s", file_name);
    ret = write(sockfd, buf, strlen(buf) + 1); // 包括字符串结束符 '\0'
    if (-1 == ret) {
        perror("write put cmd");
        close(sockfd);
        return -1;
    }
    // 读取文件内容并发送
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        ret = write(sockfd, buf, n);
        if (-1 == ret) {
            perror("write");
            close(sockfd);
            return -1;
        }
    }
    close(fd);
    close(sockfd);
    return 0;
}

int processget(struct sockaddr_in saddr, char* file_name) {
    int ret = 0;
    char buf[128] = { 0 };
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        perror("socket");
        return -1;
    }
    ret = connect(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (ret == -1) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (-1 == fd) {
        perror("open");
        close(sockfd);
        return -1;
    }
    sprintf(buf, "G%s", file_name);
    ret = write(sockfd, buf, sizeof(buf));
    if (ret == -1) {
        perror("write get cmd");
        close(sockfd);
        return -1;
    }
    while ((ret = read(sockfd, buf, 128)) > 0) {
        ret = write(fd, buf, ret);
        if (ret == -1) {
            perror("write");
            close(sockfd);
            return -1;
        }
        memset(buf, 0, sizeof(buf));
    }
    close(fd);
    close(sockfd);
    return 0;
}

int processdelete(struct sockaddr_in saddr, char* file_name) {
    char buf[128] = { 0 };
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        perror("socket");
        return -1;
    }
    int ret = connect(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (-1 == ret) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    sprintf(buf, "D%s", file_name);
    ret = write(sockfd, buf, strlen(buf) + 1); // 包括字符串结束符 '\0'
    if (-1 == ret) {
        perror("write delete cmd");
        close(sockfd);
        return -1;
    }
    close(sockfd);
    return 0;
}
// ... 省略部分代码 ...

void processhistory(struct sockaddr_in saddr) {
    char buf[128] = {0};
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        perror("socket");
        return;
    }
    int ret = connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (-1 == ret) {
        perror("connect");
        close(sockfd);
        return;
    }
    write(sockfd, "H", 1); // 发送历史记录请求
    while (read(sockfd, buf, sizeof(buf) - 1) > 0) {
        printf("%s", buf);
        memset(buf, 0, sizeof(buf));
    }
    close(sockfd);
}
int main() {
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // 创建套结字
    if (-1 == sockfd) {
        perror("socket");
        return -1;
    }
    printf("sockfd=%d\n", sockfd);
    struct sockaddr_in saddr = { 0 };
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // 服务器的外网ip
    saddr.sin_port = htons(8000);
    socklen_t slen = sizeof(saddr);
    char buf[128];
    while (1) {
        memset(buf, 0, 128);
        printf("客户端指令：");
        fgets(buf, 127, stdin);
        buf[strlen(buf) - 1] = '\0';
        if (strncmp(buf, "help", 4) == 0) {
            processhelp();
        }
        else if (strncmp(buf, "get ", 4) == 0) {
            processget(saddr, buf + 4);
        }
        else if (strncmp(buf, "put ", 4) == 0) {
            processput(saddr, buf + 4);
        }
        else if (strncmp(buf, "list", 4) == 0) {
            processlist(saddr);
        }
        else if (strncmp(buf, "delete ", 7) == 0) {
            processdelete(saddr, buf + 7);
        }
        else if (strncmp(buf, "history", 7) == 0) {
            processhistory(saddr);
        }
        else if (strncmp(buf, "exit", 4) == 0) {
            printf("下次再见~\n");
            break;
        }
        else {
            printf("下次再见~\n");
            break;
        }
    }

    close(sockfd);

    return 0;
}
