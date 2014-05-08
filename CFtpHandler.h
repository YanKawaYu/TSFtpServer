//
//  CFtpHandler.h
//  TSFtpServer
//
//  Created by zhaoxy on 14-5-7.
//  Copyright (c) 2014年 tsinghua. All rights reserved.
//

#ifndef __TSFtpServer__CFtpHandler__
#define __TSFtpServer__CFtpHandler__

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#define COMMAND_PORT "PORT"
#define COMMAND_PASSIVE "PASV"
#define COMMAND_LIST "LIST"
#define COMMAND_DELETE "DELE"
#define COMMAND_NOOP "NOOP"
#define COMMAND_RETRIEVE "RETR"
#define COMMAND_STORE "STOR"
#define COMMAND_PWD "PWD"
#define COMMAND_CWD "CWD"
#define COMMAND_QUIT "QUIT"
#define COMMAND_USER "USER"
#define COMMAND_PASS "PASS"
#define COMMAND_SYST "SYST"
#define COMMAND_TYPE "TYPE"
#define COMMAND_OPTS "OPTS"
#define COMMAND_SIZE "SIZE"
#define COMMAND_MLSD "MLSD"
#define COMMAND_FEAT "FEAT"
#define COMMAND_CDUP "CDUP"

#define TS_FTP_STATUS_READY                     "220 TS FTP Server ready...\r\n"

#define TS_FTP_STATUS_OPEN_DATA_CHANNEL         "150 Opening data channel for directory list."
#define TS_FTP_STATUS_TRANSFER_START(x)         "150 Data connection accepted; transfer starting for "+x
#define TS_FTP_STATUS_UPLOAD_START              "150 Opening BINARY mode data connection for file transfer"
#define TS_FTP_STATUS_TRAN_TYPE(x)              "200 Type set to "+x
#define TS_FTP_STATUS_PORT_SUCCESS              "200 Port command successful"
#define TS_FTP_STATUS_OK                        "200"
#define TS_FTP_STATUS_FILE_SIZE                 "213 "
#define TS_FTP_STATUS_SYSTEM_TYPE               "215 UNIX Type: L8"
#define TS_FTP_STATUS_BYE                       "221 Goodbye"
#define TS_FTP_STATUS_TRANSFER_OK               "226 Transfer ok."
#define TS_FTP_STATUS_FILE_SENT                 "226 File sent ok."
#define TS_FTP_STATUS_FILE_RECEIVE              "226 File receive ok."
#define TS_FTP_STATUS_PASV                      "227 Entering Passive Mode (127,0,0,1,"
#define TS_FTP_STATUS_LOG_IN(x)                 "230 User "+x+" logged in"
#define TS_FTP_STATUS_CWD_SUCCESS(x)            "250 CWD command successful. \""+x+"\" is current directory."
#define TS_FTP_STATUS_CDUP(x)                   "250 CDUP command successful. \""+x+"\" is current directory."
#define TS_FTP_STATUS_DELETE                    "250 DELE command successful."
#define TS_FTP_STATUS_CUR_DIR(x)                "257 \""+x+"\" is current directory."
#define TS_FTP_STATUS_PWD_REQ(x)                "331 Password required for "+x
#define TS_FTP_STATUS_PWD_ERROR                 "530 Not logged in,password error."
#define TS_FTP_STATUS_FILE_NOT_FOUND            "550 File not found"
#define TS_FTP_STATUS_CWD_FAILED(x)             "550 CWD command failed. \"" +x+ "\": directory not found."
#define TS_FTP_STATUS_DELETE_FAILED             "550 Delete file failed."

int startup(int &);

class CFtpHandler {
private:
    int m_connFd;
    int m_dataFd;
    bool m_isPassive;
    int m_clientIp;
    int m_clientPort;
public:
    std::string username;
    std::string password;
    std::string type;
    std::string fileName;
    std::string currentPath;
    
    CFtpHandler(int);
    int getDataSocket();
    bool handleRequest(char *);
};

#endif /* defined(__TSFtpServer__CFtpHandler__) */
