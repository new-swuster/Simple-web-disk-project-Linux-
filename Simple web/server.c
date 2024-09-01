#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_HISTORY 100
typedef struct {
    char ip[16];
    char command[10];
    time_t timestamp;
} History;

History history[MAX_HISTORY];
int history_index = 0;

void add_history(const char *ip, const char *command) {
    history[history_index].ip[0] = '\0';
    strcpy(history[history_index].ip, ip);
    strcpy(history[history_index].command, command);
    history[history_index].timestamp = time(NULL);
    history_index = (history_index + 1) % MAX_HISTORY;
}
void processhistory(int rws) {
    for (int i = 0; i < MAX_HISTORY; i++) {
        if (history[i].ip[0] != '\0') {
            char timestamp[20];
            ctime_r(&history[i].timestamp, timestamp);
            timestamp[strlen(timestamp) - 1] = '\0'; // Remove the newline
            write(rws, "IP: ", 4);
            write(rws, history[i].ip, strlen(history[i].ip));
            write(rws, " CMD: ", 6);
            write(rws, history[i].command, strlen(history[i].command));
            write(rws, " Time: ", 7);
            write(rws, timestamp, strlen(timestamp));
            write(rws, "\n", 1);
        }
    }
}
int processlist(int rws) {
    char buf[128] = {0};
    DIR *p = NULL;
    struct dirent *dir;
    p = opendir(".");
    if (p == NULL) {
        perror("opendir");
        close(rws);
        return -1;
    }
    while (dir = readdir(p)) {
        strcat(buf, dir->d_name);
        strcat(buf, "\n");
    }
    buf[strlen(buf) - 1] = '\0';
    write(rws, buf, 128);
    closedir(p);
    return 0;
}

int processget(int rws, char *file_name) {
    int n;
    char buf[128] = {0};
    int fd = open(file_name, O_RDONLY);
    if (-1 == fd) {
        perror("open");
        return -1;
    }
    while (n = read(fd, buf, 128)) {
        write(rws, buf, n);
        memset(buf, 0, 128);
    }
    close(fd);
    return 0;
}

int processput(int rws, char *file_name) {
    int ret;
    char buf[128] = {0};
    int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    while ((ret = read(rws, buf, sizeof(buf))) > 0) {
        ret = write(fd, buf, ret);
        if (ret == -1) {
            perror("write");
            return -1;
        }
    }
    close(fd);
    return 0;
}

int processdelete(int rws, char *file_name) {
    if (remove(file_name) == 0) {
        write(rws, "File deleted successfully\n", 23);
    } else {
        perror("delete");
        write(rws, "Failed to delete file\n", 22);
    }
    return 0;
}

int server_init(char *ipaddr, unsigned short port, int backlog) {
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        perror("socket");
        return -1;
    }
    int on = 1;
    if (0 > setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = (NULL == ipaddr) ? (htonl(INADDR_ANY)) : inet_addr(ipaddr);
    socklen_t slen = sizeof(saddr);
    int ret = bind(sockfd, (struct sockaddr *)&saddr, slen);
    if (ret == -1) {
        perror("bind");
        goto ERR_STEMP;
    }
    printf("Binding successful\n");
    ret = listen(sockfd, backlog);
    if (-1 == ret) {
        perror("listen");
        goto ERR_STEMP;
    }
    return sockfd;
ERR_STEMP:
    close(sockfd);
    return -1;
}

int main() {
    int ret;
    int sockfd = server_init(NULL, 8000, 1024);
    if (-1 == sockfd) {
        perror("server_init fail\n");
        return -1;
    }
    printf("wait......\n");
    struct sockaddr_in caddr;
    memset(&caddr, 0, sizeof(caddr));
    socklen_t clen = sizeof(caddr);
    while (1) {
        int rws = accept(sockfd, (struct sockaddr *)&caddr, &clen);
        if (rws == -1) {
            perror("accept");
            close(sockfd);
            return -1;
        }
        printf("res=%d\n", rws);
        char buf[128];
        while (1) {
            memset(buf, 0, 128);
            ret = recv(rws, buf, sizeof(buf), 0);
            if (ret == -1) {
                perror("recv");
                break;
            } else if (ret == 0) {
                printf("client close\n");
                close(rws);
                break;
            } else {
                fputs(buf, stdout);
                char client_ip[16];
                inet_ntop(AF_INET, &caddr.sin_addr, client_ip, sizeof(client_ip));
                add_history(client_ip, buf);
                if (strncmp(buf, "exit", 4) == 0) {
                    close(rws);
                    break;
                }
                switch (buf[0]) {
                    case 'L':
                        processlist(rws);
                        break;
                    case 'G':
                        processget(rws, buf + 1);
                        break;
                    case 'P':
                        processput(rws, buf + 1);
                        break;
                    case 'D':
                        processdelete(rws, buf + 1);
                        break;
                    case 'H': // 'H' 用于查看历史记录
                        processhistory(rws);
                        break;
                }
                close(rws);
                break;
            }
        }
    }
    close(sockfd);
    return 0;
}
