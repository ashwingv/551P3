#include "common.h"

/* MUST FILED */
extern int port;			// PORT NUMBER OF THE NODE
extern uint32_t location;		// LOCATION OF THE NODE		
extern char *homeDir;		// HOME DIR FOR THE NODE IN WHICH THE NODE CREATED THE TEMPORARY AND PERMANENT FILES

/* OPTIONAL FILEDS */
extern int reset;
extern string logFileName;	// 	LOG FILE WHICH IS CREATED IN THE NODES HOME DIRE
extern int autoShutdown;					// 	TIME AFTER WHICH THE PROGRAM SHUD EXIT
extern int ttl;							// 	TIME TO LIVE FOR THE MSG
extern int msgLifeTime;					// 	LIFE TIME OF A MSG
extern int getMsgLifeTime;				// 
extern int initNeighbors;				//	NUMBER OF NEIGHBORS TO ESTABLISH CONNECTION WITH
extern int joinTimeOut;					//	AMMOUNT OF TIME A NODE MUST WAIT FOR JOIN RESPONSE
extern int keepAliveTimeOut;			// 	TIME AFTER WHICH THE KEEP ALIVE MSG SHUD BE SENT
extern int minNeighbors;					//  MIN NUMBER OF NEIGHBORS THE NODE MUST ESTABLISH CONNECTION WITH WHEN IT COMES UP
extern int noCheck;
extern double cacheProb;				// 	A PROBABILITY BY WHICH A FILE MUST BE CACHED
extern double storeProb;
extern double neighborStoreProb;
extern int cacheSize;
extern int retry;
extern int iAmBeacon;
extern char *nodeId;
extern char *nodeInstanceId;

extern vector<int> beaconPorts;
extern int bcount;

extern int alreadyInNetwork;
extern FILE *INLFile;
extern char *inl;
extern vector<neighborDetails *> neighborVector;

//extern struct conDetails;
extern vector<conDetails *> conVector;
extern pthread_mutex_t *conVectorLock;

extern list<packet *> packetQ;
extern pthread_mutex_t *packetQLock;

extern vector<pthread_t *> threadsList;
extern pthread_mutex_t *threadsListLock;

extern vector<messageCache *> msgCacheVector;
extern pthread_mutex_t *msgCacheLock;
extern pthread_t *oneSecTimerTh;



extern list<logDetails *> logEntryList;		// A list that contails all the log entries
extern pthread_mutex_t *logEntryListLock;		// A lock for it
extern pthread_cond_t *logEntryListEmptyCV;	// Cv on which the log writer thread will wait if the log entry list is empty
extern pthread_t *logWritterTh;

extern vector<nInfo *> nInfoVector;
extern pthread_mutex_t *nInfoVectorLock;

extern FILE *logFile;
extern char *logFilePath;

extern pthread_t *keepAliveTh;

extern FILE *statusFile;
extern char *statusFilePath;
extern sigset_t signals;

extern uint8_t statusTTL;

extern int status;
extern char *statusUoid;
extern pthread_mutex_t *statusLock;

extern vector<namDetails *> namVector;
extern pthread_mutex_t *namVectorLock;

extern pthread_mutex_t *printLock;


extern pthread_mutex_t *threadRefLock;
extern vector<pthread_t> threadRefs;

extern vector<joinDetails *> joinVector;
extern pthread_mutex_t *joinVectorLock;

//extern  joinReplyCount;

extern int toExit;
extern int restart;
extern pthread_mutex_t *systemLock;
extern pthread_cond_t *systemCV;

extern int check;
extern pthread_mutex_t *checkLock;

extern int networkOk;
extern pthread_mutex_t *networkLock;


extern int conCount;
extern pthread_mutex_t *conCountLock;
extern pthread_cond_t *conCountCV;
