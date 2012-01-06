#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<fstream>
#include<string>
#include<list>
#include<vector>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <netdb.h>
#include <time.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <dirent.h>
#include<errno.h>
#include<signal.h>
//#include "hdr_s.h"     
//#include "Format.h"



#define CON_MAX 16

#define JNRQ htons(0xFC)
#define JNRS htons(0xFB)
#define HLLO htons(0xFA)
#define KPAV htons(0xF8)
#define NTFY htons(0xF7)
#define CKRQ htons(0xF6)
#define CKRS htons(0xF5)
#define SHRQ htons(0xEC)
#define SHRS htons(0xEB)
#define GTRQ htons(0xDC)
#define GTRS htons(0xDB)
#define STOR htons(0xCC)
#define DELT htons(0xBC)
#define STRQ htons(0xAC)
#define STRS htons(0xAB)

#define HSIZE 27

#define STN htons(0x01)



using namespace std;


/*
A log details struct which contains all the details thatr goes into the log file
*/
struct logDetails
{
	char action;
	struct timeval msgTime;
	char *fromTo;
	uint8_t type;
	uint32_t size;
	uint8_t ttl;
	char *msgId;
	char *data;
};


/*
A Structure to maintain a neighbor details
*/
struct nInfo
{
	char *nodeId;
	int sockFd;
};

struct neighborDetails
{
	char *nName;
	int nPort;
};


/*
structure to maintain the connection details 
*/
struct conDetails
{
	uint16_t peerPort;
	//char *peerName;
	int sockFd;
	int init;
	int keepAlive;
};

struct joinDetails
{
	int peerPort;
	int sockFd;
};

/* structure to maitaing the msg queue */
struct packet
{
	unsigned char *msgToSend;
	int flood;
	int sockFd;
	int rSockFd;
};

/*
structure to maintain the message chache details
*/
struct messageCache
{
	uint8_t msgType;
	char *uoid;
	int lifeTime;
	int fromSock;
	uint16_t peerPort;
};

struct header
{
	uint8_t type;		// 0
	char *uoid; 		// 1-20
	uint8_t ttl;		// 21
	uint8_t reserved;	// 22
	uint32_t dataLen;   // 23-26	
};

struct hello
{
	uint16_t hostPort;
	char *hostName;
};

struct joinMsg
{
	uint32_t hostLoc;
 	uint16_t hostPort;
 	char * hostName;
 	
};

struct joinRply
{
	char *joinUoid;
	uint32_t distance;
	uint16_t hostPort;
	char *hostName;
};



struct keepAlive
{
 header msgHeader;
};

struct statusRequest
{
	uint8_t type;
};

struct statusReply
{
	char *statusReqUoid;
	uint16_t hostInfoLength;
	uint16_t hostPort;
	char *hostName;
};

/*
This structure holds the data that has the record for the satus reply
*/
struct statusRecord
{
	uint32_t recordLen;
	char *recordData;
};

struct checkReply
{
	char *checkUoid;
};

struct notify
{
	uint8_t errorCode;
};

/*
This structture holds the data that needes to be up into the nam file 
*/

struct namDetails
{
	uint16_t node;
	vector<uint16_t> connectedNodes;
};
