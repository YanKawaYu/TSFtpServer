//
//  CFtpHandler.cpp
//  TSFtpServer
//
//  Created by zhaoxy on 14-5-7.
//  Copyright (c) 2014年 tsinghua. All rights reserved.
//

#include "CFtpHandler.h"
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include "define.h"
#include <dirent.h>
#include <iomanip>

//is file a directory
bool isDirectory(const char* filename) {
    struct stat s;
    stat(filename, &s);
    return s.st_mode & S_IFDIR;
}

//get file size
std::ifstream::pos_type filesize(const char* filename)
{
    if (isDirectory(filename)) {
        return 0;
    }
    std::ifstream in(filename, std::ifstream::in | std::ifstream::binary);
    in.seekg(0, std::ifstream::end);
    return in.tellg();
}

//create socket and bind
int startup(int &port) {
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    //protocol domain
    servAddr.sin_family = AF_INET;
    //default ip
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //port
    servAddr.sin_port = htons(port);
    int listenFd;
    //create socket
    if ((listenFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    unsigned value = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
    //bind socket to port
    if (bind(listenFd, (struct sockaddr *)&servAddr, sizeof(servAddr))) {
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    //dynamically allocating a port
    if (port == 0) {
        socklen_t namelen = sizeof(servAddr);
        if (getsockname(listenFd, (struct sockaddr *)&servAddr, &namelen) == -1) {
            printf("getsockname error: %s(errno: %d)\n",strerror(errno),errno);
        }
        port = ntohs(servAddr.sin_port);
    }
    //start listen
    if (listen(listenFd, 10) == -1) {
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    return listenFd;
}

//create socket and connect
int connect(int ipAddr, int port) {
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1) {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    //protocol domain
    servAddr.sin_family = AF_INET;
    //server ip address
    servAddr.sin_addr.s_addr = ipAddr;
    //server port
    servAddr.sin_port = htons(port);
    //connect to server
    if (connect(sockfd, (struct sockaddr *)&servAddr, sizeof(struct sockaddr))==-1) {
        printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    return sockfd;
}

using namespace std;

//send response
void sendResponse(int connFd, string msg) {
    msg += "\r\n";
    cout<<msg;
    send(connFd, msg.c_str(), msg.length(), 0);
}

CFtpHandler::CFtpHandler(int connFd) {
    m_connFd = connFd;
    m_dataFd = 0;
    currentPath = "/";
}

//get data socket
int CFtpHandler::getDataSocket() {
    //passive mode
    if (m_isPassive) {
        return accept(m_dataFd, (struct sockaddr *)NULL, NULL);
    }
    //active mode
    else {
        return connect(m_clientIp, m_clientPort);
    }
}

//handle client request
bool CFtpHandler::handleRequest(char *buff) {
    stringstream recvStream;
    recvStream<<buff;

    cout<<buff;
    string command;
    recvStream>>command;

    /* allow client input lowercast commond */
    std::transform(command.begin(), command.end(), command.begin(),::toupper);

    bool isClose = false;
    string msg;
    //username
    if (command == COMMAND_USER) {
        recvStream>>username;
        msg = TS_FTP_STATUS_PWD_REQ(username);
    }
    //password
    else if (command == COMMAND_PASS) {
        recvStream>>password;
        if (username == "zhaoxy" && password == "123") {
            msg = TS_FTP_STATUS_LOG_IN(username);
        }else {
            msg = TS_FTP_STATUS_PWD_ERROR;
        }
    }
    //quit
    else if (command == COMMAND_QUIT) {
        msg = TS_FTP_STATUS_BYE;
        isClose = true;
    }
    //system type
    else if (command == COMMAND_SYST) {
        msg = TS_FTP_STATUS_SYSTEM_TYPE;
    }
    //current directory
    else if (command == COMMAND_PWD) {
        msg = TS_FTP_STATUS_CUR_DIR(currentPath);
    }
    //transmit type
    else if (command == COMMAND_TYPE) {
        recvStream>>type;
        msg = TS_FTP_STATUS_TRAN_TYPE(type);
    }
    //passive mode
    else if (command == COMMAND_PASSIVE) {
        int port = 0;
        if (m_dataFd) {
            close(m_dataFd);
        }
        m_dataFd = startup(port);
        
        stringstream stream;
        stream<<TS_FTP_STATUS_PASV<<port/256<<","<<port%256<<")";
        msg = stream.str();
        
        //active passive mode
        m_isPassive = true;
    }
    //active mode
    else if (command == COMMAND_PORT) {
        string ipStr;
        recvStream>>ipStr;
        
        char ipC[32];
        strcpy(ipC, ipStr.c_str());
        char *ext = strtok(ipC, ",");
        m_clientPort = 0; m_clientIp = 0;
        m_clientIp = atoi(ext);
        int count = 0;
        //convert string to ip address and port number
        //be careful, the ip should be network endianness
        while (1) {
            if ((ext = strtok(NULL, ","))==NULL) {
                break;
            }
            switch (++count) {
                case 1:
                case 2:
                case 3:
                    m_clientIp |= atoi(ext)<<(count*8);
                    break;
                case 4:
                    m_clientPort += atoi(ext)*256;
                    break;
                case 5:
                    m_clientPort += atoi(ext);
                    break;
                default:
                    break;
            }
        }
        msg = TS_FTP_STATUS_PORT_SUCCESS;
    }
    //file size
    else if (command == COMMAND_SIZE) {
        recvStream>>fileName;
        string filePath = ROOT_PATH+currentPath+fileName;
        long fileSize = filesize(filePath.c_str());
        if (fileSize) {
            stringstream stream;
            stream<<TS_FTP_STATUS_FILE_SIZE<<fileSize;
            msg = stream.str();
        }else {
            msg = TS_FTP_STATUS_FILE_NOT_FOUND;
        }
    }
    //change directory
    else if (command == COMMAND_CWD) {
        string tmpPath;
        recvStream>>tmpPath;
        string dirPath = ROOT_PATH+tmpPath;
        if (isDirectory(dirPath.c_str())) {
            currentPath = tmpPath;
            msg = TS_FTP_STATUS_CWD_SUCCESS(currentPath);
        }else {
            msg = TS_FTP_STATUS_CWD_FAILED(currentPath);
        }
    }
    //show file list
    else if (command == COMMAND_LIST || command == COMMAND_MLSD) {
        string param;
        recvStream>>param;
        
        msg = TS_FTP_STATUS_OPEN_DATA_CHANNEL;
        sendResponse(m_connFd, msg);
        int newFd = getDataSocket();
        //get files in directory
        string dirPath = ROOT_PATH+currentPath;
        DIR *dir = opendir(dirPath.c_str());
        struct dirent *ent;
        struct stat s;
        stringstream stream;
        while ((ent = readdir(dir))!=NULL) {
            string filePath = dirPath + ent->d_name;
            stat(filePath.c_str(), &s);
            struct tm tm = *gmtime(&s.st_mtime);
            //list with -l param
            if (param == "-l") {
                stream<<s.st_mode<<" "<<s.st_nlink<<" "<<s.st_uid<<" "<<s.st_gid<<" "<<setw(10)<<s.st_size<<" "<<tm.tm_mon<<" "<<tm.tm_mday<<" "<<tm.tm_year<<" "<<ent->d_name<<endl;
            }else {
                stream<<ent->d_name<<endl;
            }
        }
        closedir(dir);
        //send file info
        string fileInfo = stream.str();
        cout<<fileInfo;
        send(newFd, fileInfo.c_str(), fileInfo.size(), 0);
        //close client
        close(newFd);
        //send transfer ok
        msg = TS_FTP_STATUS_TRANSFER_OK;
    }
    //send file
    else if (command == COMMAND_RETRIEVE) {
        recvStream>>fileName;
        msg = TS_FTP_STATUS_TRANSFER_START(fileName);
        sendResponse(m_connFd, msg);
        int newFd = getDataSocket();
        //send file
        std::ifstream file((ROOT_PATH+currentPath+fileName).c_str(),std::ifstream::in);

        file.seekg(0, std::ifstream::beg);
        while(file.tellg() != -1)
        {
            char *p = new char[1024];
            bzero(p, 1024);
            file.read(p, 1024);
            int n = (int)send(newFd, p, 1024, 0);
            if (n < 0) {
                cout<<"ERROR writing to socket"<<endl;
                break;
            }
            delete p;
        }
        file.close();
        //close client
        close(newFd);
        //send transfer ok
        msg = TS_FTP_STATUS_FILE_SENT;
    }
    //receive file
    else if (command == COMMAND_STORE) {
        recvStream>>fileName;
        msg = TS_FTP_STATUS_UPLOAD_START;
        sendResponse(m_connFd, msg);
        int newFd = getDataSocket();
        //receive file
        ofstream file;
        file.open((ROOT_PATH+currentPath+fileName).c_str(), ios::out | ios::binary);
        char buff[1024];
        while (1) {
            int n = (int)recv(newFd, buff, sizeof(buff), 0);
            if (n<=0) break;
            file.write(buff, n);
        }
        file.close();
        //close client
        close(newFd);
        //send transfer ok
        msg = TS_FTP_STATUS_FILE_RECEIVE;
    }
    //get support command
    else if (command == COMMAND_FEAT) {
        stringstream stream;
        stream<<"211-Extension supported"<<endl;
        stream<<COMMAND_SIZE<<endl;
        stream<<"211 End"<<endl;;
        msg = stream.str();
    }
    //get parent directory
    else if (command == COMMAND_CDUP) {
        if (currentPath != "/") {
            char path[256];
            strcpy(path, currentPath.c_str());
            char *ext = strtok(path, "/");
            char *lastExt = ext;
            while (ext!=NULL) {
                ext = strtok(NULL, "/");
                if (ext) lastExt = ext;
            }
            currentPath = currentPath.substr(0, currentPath.length()-strlen(lastExt)-1);
        }
        msg = TS_FTP_STATUS_CDUP(currentPath);
    }
    //delete file
    else if (command == COMMAND_DELETE) {
        recvStream>>fileName;
        //delete file
        if (remove((ROOT_PATH+currentPath+fileName).c_str()) == 0) {
            msg = TS_FTP_STATUS_DELETE;
        }else {
            printf("delete error: %s(errno: %d)\n",strerror(errno),errno);
            msg = TS_FTP_STATUS_DELETE_FAILED;
        }
    }
    //other
    else if (command == COMMAND_NOOP || command == COMMAND_OPTS){
        msg = TS_FTP_STATUS_OK;
    }
    else
    	msg = TS_FTP_STATUS_CMD_ERROR;

    
    sendResponse(m_connFd, msg);
    return isClose;
}
