#include "common.h"


/* MUST FILED */
int port =-1;			// PORT NUMBER OF THE NODE
uint32_t location=0;		// LOCATION OF THE NODE		
char *homeDir=NULL;		// HOME DIR FOR THE NODE IN WHICH THE NODE CREATED THE TEMPORARY AND PERMANENT FILES


/* OPTIONAL FILEDS */
int reset=0;
string logFileName="servant.log";	// 	LOG FILE WHICH IS CREATED IN THE NODES HOME DIRE
int autoShutdown=900;					// 	TIME AFTER WHICH THE PROGRAM SHUD EXIT
uint8_t ttl=4;	// should be 30						// 	TIME TO LIVE FOR THE MSG
int msgLifeTime=10; // shud be 30;					// 	LIFE TIME OF A MSG
int getMsgLifeTime=300;				// 
int initNeighbors=3;				//	NUMBER OF NEIGHBORS TO ESTABLISH CONNECTION WITH
int joinTimeOut=5;	// shoud nbe 15				//	AMMOUNT OF TIME A NODE MUST WAIT FOR JOIN RESPONSE
int keepAliveTimeOut=60;			// 	TIME AFTER WHICH THE KEEP ALIVE MSG SHUD BE SENT
int minNeighbors=2;					//  MIN NUMBER OF NEIGHBORS THE NODE MUST ESTABLISH CONNECTION WITH WHEN IT COMES UP
int noCheck=0;
double cacheProb=0.1;				// 	A PROBABILITY BY WHICH A FILE MUST BE CACHED
double storeProb=0.1;
double neighborStoreProb=0.1;
int cacheSize=500;
int retry=30;
int iAmBeacon=0;
char *nodeId;
char *nodeInstanceId;
vector<int> beaconPorts;
int bcount=0;
 
// Additional details needed for my comfort

int alreadyInNetwork=0; // this flag indicats wether a non beacon node exisit already in network or not
FILE *INLFile;
char *inl;

vector<neighborDetails *> neighborVector;

pthread_t *oneSecTimerTh;
pthread_t *commonWriterThread;
pthread_t *listenThread;
struct sockaddr_in serv_addr; // for server socket to lsiten in well-known port
int listenSock;



vector<conDetails *> conVector; // a vector to hold the details of the connections
pthread_mutex_t *conVectorLock; // lock for ME


list<packet *> packetQ;
pthread_mutex_t *packetQLock;
pthread_cond_t *writerWaiting;

vector<pthread_t *> threadsList;
pthread_mutex_t *threadsListLock;

vector<messageCache *> msgCacheVector;
pthread_mutex_t *msgCacheLock;

list<logDetails *> logEntryList;		// A list that contails all the log entries
pthread_mutex_t *logEntryListLock;		// A lock for it
pthread_cond_t *logEntryListEmptyCV;	// Cv on which the log writer thread will wait if the log entry list is empty
pthread_t *logWritterTh;

vector<nInfo *> nInfoVector;
pthread_mutex_t *nInfoVectorLock;

FILE *logFile;
char *logFilePath;

pthread_t *keepAliveTh;

FILE *statusFile;
char *statusFilePath;
sigset_t signals;

uint8_t statusTTL=0;
int status;
char *statusUoid=NULL;
pthread_mutex_t *statusLock;

vector<namDetails *> namVector;
pthread_mutex_t *namVectorLock;

pthread_mutex_t *printLock;


pthread_mutex_t *threadRefLock;
vector<pthread_t> threadRefs;

vector<joinDetails *> joinVector;
pthread_mutex_t *joinVectorLock;


//int joinReplyCount=0;

int toExit=0;
int restart=0;
pthread_mutex_t *systemLock;
pthread_cond_t *systemCV;

int check=0;
pthread_mutex_t *checkLock;

int networkOk=1;
pthread_mutex_t *networkLock;

int conCount=0;
pthread_mutex_t *conCountLock;
pthread_cond_t *conCountCV;


