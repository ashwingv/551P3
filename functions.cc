#include "extern.h"
#ifndef min
#define min(A,B) (((A)>(B)) ? (B) : (A))
#endif 


void printJoinDetails()
{

pthread_mutex_lock(joinVectorLock);
	for(unsigned int i=0;i<joinVector.size();i++)
	{
		//joinDetails *jd=joinVector[i];

		//cout<<"Port is "<<jd->peerPort<<endl;
		
		//cout<<" SOkFD is "<<jd->sockFd<<endl;
	
	}
	pthread_mutex_unlock(joinVectorLock);
}
/*
This function is called to set the keep alive value for a
connected node
*/
void setKeepAlive(int sockFd)
{
	pthread_mutex_lock(conVectorLock);
	for(unsigned int i=0;i<conVector.size();i++)
	{
		conDetails *con=conVector[i];
		if(con->sockFd==sockFd)
		{
			con->keepAlive=keepAliveTimeOut;
			break;
		}
	}
	pthread_mutex_unlock(conVectorLock);
}

/*
This function is called to get the keepAlive value of a conection
*/
int getKeepAlive(int sockFd)
{
	pthread_mutex_lock(conVectorLock);
	for(unsigned int i=0;i<conVector.size();i++)
	{
		conDetails *con=conVector[i];
		if(con->sockFd==sockFd)
		{
			pthread_mutex_unlock(conVectorLock);
			return con->keepAlive;
		}
		
	}
	cout<<"!!!!!!!!!!!!!!!!!!!!!!!!! THIS CANNOT HAPPEN !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
	pthread_mutex_unlock(conVectorLock);
	return -1;
}


/*
This function when called return the node id given the sock fd
*/

char *getNodeId(int sockFd)
{
	pthread_mutex_lock(nInfoVectorLock);
	for(unsigned int i=0;i<nInfoVector.size();i++)
	{
		nInfo *nei=nInfoVector[i];
		if(nei->sockFd==sockFd)
		{
			pthread_mutex_unlock(nInfoVectorLock);
			//cout<<"############### Returning -NEI ##################"<<endl;
			return nei->nodeId;
		}
	}
	pthread_mutex_unlock(nInfoVectorLock);
	/*pthread_mutex_lock(conVectorLock);
	for(unsigned int i=0;i<conVector.size();i++)
	{
		conDetails *con=conVector[i];
		if(con->sockFd==sockFd)
		{
			char *nodeId=new char[25];
			memset(nodeId,'\0',25);
			sprintf(nodeId,"nunki.usc.edu_%d",con->peerPort);
			pthread_mutex_unlock(conVectorLock);
			return nodeId;
		}
	}
	pthread_mutex_unlock(conVectorLock);*/
	//cout<<"############### Returning NULL-NEI ##################"<<endl;
	return NULL;
}

char *getJoinNodeId(int sockFd)
{
	pthread_mutex_lock(joinVectorLock);
	for(unsigned int i=0;i<joinVector.size();i++)
	{
		joinDetails *jd=joinVector[i];
		if(jd->sockFd==sockFd)
		{
			char *nodeId=new char[25];
			memset(nodeId,'\0',25);
			sprintf(nodeId,"nunki.usc.edu_%d",jd->peerPort);
			pthread_mutex_unlock(joinVectorLock);
			//cout<<"############### Returning -JOIN ##################"<<endl;
			return nodeId;
		}
	}
	pthread_mutex_unlock(joinVectorLock);
	//cout<<"############### Returning NULL-JOIN ##################"<<endl;
	return NULL;

}

/*
This function is called to create a log entry 

sturct logDetails
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

*/

logDetails *createLogEntry(unsigned char *totalMsg,int sockFd,char action)
{
	logDetails *logEntry=new logDetails();
	logEntry->action=action;
	gettimeofday(&logEntry->msgTime,NULL);
	logEntry->fromTo=getJoinNodeId(sockFd);
	if(logEntry->fromTo==NULL)
	{
		logEntry->fromTo=getNodeId(sockFd);
		
	}
	if(logEntry->fromTo==NULL)
	{
		//cout<<"NULLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"<<endl;
	}
	memcpy(&logEntry->type,(uint8_t *)&totalMsg[0],sizeof(uint8_t));
	memcpy(&logEntry->size,(uint32_t *)&totalMsg[23],sizeof(uint32_t));
	memcpy(&logEntry->ttl,(uint8_t *)&totalMsg[21],sizeof(uint8_t));
	logEntry->msgId=new char[4];
	memset(logEntry->msgId,'\0',4);
	memcpy(logEntry->msgId,&totalMsg[17],4);
	switch(logEntry->type)
	{
		case HLLO:
		{
			//cout<<"Creating log entry for HLLO msg"<<endl;
		
			logEntry->data=new char[logEntry->size];
			memcpy(logEntry->data,&totalMsg[27],logEntry->size);
			break;
		}
		case JNRQ:
		{	
			logEntry->data=new char[logEntry->size];
			memcpy(logEntry->data,&totalMsg[27],logEntry->size);
			break;
		}
		case JNRS:
		{
			logEntry->data=new char[logEntry->size];
			memcpy(logEntry->data,&totalMsg[27],logEntry->size);
			break;
		}
		case KPAV:
		{
			logEntry->data=NULL;
			break;
		}
		case STRQ:
		{
			logEntry->data=new char[logEntry->size];
			memcpy(logEntry->data,&totalMsg[27],sizeof(uint8_t));
			break;
		}
		case STRS:
		{
			logEntry->data=new char[4];
			memcpy(logEntry->data,&totalMsg[43],4);
			break;
		}
		case NTFY:
		{
			logEntry->data=new char[1];
			memcpy(logEntry->data,&totalMsg[27],1);
			break;
		}
		case CKRQ:
		{
			logEntry->data=NULL;
			break;
		}
		case CKRS:
		{
			logEntry->data=new char[4];
			memcpy(logEntry->data,&totalMsg[43],4);
			break;
		}
	}
	
	return logEntry;
}
/*
This function is called during system setup to create and set a node id
*/
void setNodeId()
{
	nodeId=new char[21];
	memset(nodeId,'\0',21);
	if(gethostname(nodeId,21)<0)
	{
		cout<<"Error in get host by name"<<endl;
		exit(0);
	}
	char *tempPort=new char[6];
	memset(tempPort,'\0',6);
	sprintf(tempPort,"_%d",port);
	memcpy((nodeId+strlen(nodeId)),tempPort,strlen(tempPort));
	//cout<<"node id is "<<nodeId<<endl;
	delete(tempPort);
}

/*
This function is called dutin the system setup to create and set a node instance id
*/
void setNodeInstanceId()
{
	time_t timeNow;
	timeNow=time(NULL);
	
	char *tempTime=new char[100];
	memset(tempTime,'\0',100);
	sprintf(tempTime,"_%ld",timeNow);
	
	nodeInstanceId=new char[32];
	memset(nodeInstanceId,'\0',32);
	memcpy(nodeInstanceId,nodeId,strlen(nodeId));
	
	memcpy((nodeInstanceId+strlen(nodeInstanceId)),tempTime,strlen(tempTime));
	//cout<<"The node instance id is "<<nodeInstanceId<<endl;
	delete(tempTime);
}



/*
 This function returns a uniue object id for the passed in object
*/
char *GetUOID(char *node_inst_id, char *obj_type, int uoid_buf_sz)
  {
	  char *uoid_buf=new char[SHA_DIGEST_LENGTH+1];
	  memset(uoid_buf,'\0',SHA_DIGEST_LENGTH+1);
	  static unsigned long seq_no=(unsigned long)1;
      char sha1_buf[SHA_DIGEST_LENGTH];
      memset(sha1_buf,'\0',SHA_DIGEST_LENGTH);
	  char str_buf[104];
      snprintf(str_buf, sizeof(str_buf), "%s_%s_%1ld", node_inst_id, obj_type, (long)seq_no++);
      SHA1((const unsigned char *)str_buf, strlen(str_buf), (unsigned char *)sha1_buf);
      memset(uoid_buf, 0, uoid_buf_sz);
      memcpy(uoid_buf, sha1_buf, min(uoid_buf_sz,(int)sizeof(sha1_buf)));
	  //cout<<"UOID Buffer :"<<uoid_buf<<endl;
	  //for (int i =0;i<SHA_DIGEST_LENGTH;i++)
	   //printf("%02x", (unsigned char)uoid_buf[i]);
	 // printf("\n");
	 // cout<<"Strlen "<<strlen(uoid_buf)<<endl;
	  return uoid_buf;
  }


/*
Prints all the connection in the connection vector
*/
void printCons()
{
	pthread_mutex_lock(conVectorLock);
	//cout<<"My connections are "<<endl;
	for(unsigned int i=0;i<conVector.size();i++)
	{
		//conDetails *con=conVector[i];
		//cout<<" Port :"<<con->peerPort<<endl;
	}
	pthread_mutex_unlock(conVectorLock);
}

/*
dummy function: pritns the data in the msg cache
*/
void printMsgCache()
{
	pthread_mutex_lock(msgCacheLock);
	for(unsigned int i=0;i<msgCacheVector.size();i++)
	{
		messageCache *msgInfo=msgCacheVector[i];
		switch(msgInfo->msgType)
		{
			case HLLO:
			{
				//cout<<" The msg type is HLLO"<<endl;
				break;
			}
			case JNRQ:
			{
				//cout<<" the msg type is JNRQ"<<endl;
				break;
			}
		}
		//for (int i =0;i<SHA_DIGEST_LENGTH;i++)
		//	printf("%02x", (unsigned char)msgInfo->uoid[i]);
		//cout<<endl;
		//cout<<"the msg life is "<<msgInfo->lifeTime<<endl;
		//cout<<"the msg is from node "<<msgInfo->peerPort<<endl;
	}
	pthread_mutex_unlock(msgCacheLock);
}

/*
This functions is called to check if the given msg is already there in the msg cache or not
return 1 if one exist or else returns zero
*/
int checkMsgCache(char *uoid)
{
	pthread_mutex_lock(msgCacheLock);
	for(unsigned int i=0;i<msgCacheVector.size();i++)
	{
		messageCache *msg=msgCacheVector[i];
		if(strncmp(uoid,msg->uoid,20)==0)
		{
			//cout<<"---------------------------------------------------------->Msg aready there in cache "<<endl;
			pthread_mutex_unlock(msgCacheLock);
			return 1;
		}
	}
	pthread_mutex_unlock(msgCacheLock);
	return 0;
	
}
/*
This function when given a sockfd return the welknown port number of the node it is connected to 
*/
uint16_t getWellKnownPort(int sockFd)
{
	pthread_mutex_lock(conVectorLock);
	for(unsigned int i=0;i<conVector.size();i++)
	{
		conDetails *con=conVector[i];
		if(con->sockFd==sockFd)
		{
			pthread_mutex_unlock(conVectorLock);
			return con->peerPort;
		}
	}
	pthread_mutex_unlock(conVectorLock);
	return 0;
}

/*
This function is called to decrement the ttl of the receieved msg 
*/
void decTTL(unsigned char *msg)
{
	uint8_t ttl;
	memcpy(&ttl,&(msg[21]),sizeof(ttl)); // getting the ttl value
	//cout<<"TTL before decrement is "<<(int )ttl<<endl;
	ttl=ttl-1;								// decrementing the ttl value
	//cout<<"TTL after decrement is "<<(int )ttl<<endl;
	memcpy(&(msg[21]),&ttl,sizeof(ttl));// putting the decremented value back in the msg
	return;
}

/*
This function is called to get the routing sockfd
returns -1 if it cannot find a route
*/

int getRoute(unsigned char *msg)
{
	char *joinUoid=new char[20];
	memcpy(joinUoid,&msg[27],20); // copying the joind uoid
	pthread_mutex_lock(msgCacheLock);
	for(unsigned int i=0;i<msgCacheVector.size();i++)
	{
		messageCache *msg=msgCacheVector[i];
		//cout<<"join uoid"<<endl;
		//for (int i =0;i<SHA_DIGEST_LENGTH;i++)
		//	printf("%02x", (unsigned char)joinUoid[i]);
		//printf("\n");
		//cout<<"uoid in cache"<<endl;
		//for (int i =0;i<SHA_DIGEST_LENGTH;i++)
		//	printf("%02x", (unsigned char)msg->uoid[i]);
		//printf("\n");
		
		
		if(strncmp(joinUoid,msg->uoid,20)==0)
		{
			//cout<<"---------------------------------------------------------->There is mactch got the routing information "<<endl;
			pthread_mutex_unlock(msgCacheLock);
			delete(joinUoid);
			return (msg->fromSock);
		}

	}
	delete(joinUoid);
	pthread_mutex_unlock(msgCacheLock);
	return -1;
}

/*
This function is called to create a reply msg for a particular request msg
*/
unsigned char *createReplyMsg(uint8_t type,unsigned char *totalMsg)
{
		unsigned char *msgBuf;
		char *obj=new char[3];
		//string obj="msg";
		sprintf(obj,"%s","msg");
		switch (type)
		{
			case JNRS:
			{
					header *head=new header();
					head->type=JNRS; // setting the msg type to join response
					head->uoid=GetUOID(nodeInstanceId,obj,20); // setting a new uoid
					head->ttl=ttl;	// setting the ttl to the nodes ttl
					head->reserved=0;	// setting the reserved to zero
					int headerSize=sizeof(head->type)+strlen(head->uoid)+sizeof(head->ttl)+sizeof(head->reserved)+sizeof(head->dataLen);

					joinRply *msg=new joinRply();
					char *Name=new char[15];
					memset(Name,'\0',15);
					gethostname(Name,15);
					
					msg->hostPort=port;		// setting the host port in the msg 
					msg->hostName=new char[strlen(Name)+1]; // setting the host name
					memset(msg->hostName,'\0',strlen(Name)+1);
					memcpy(msg->hostName,Name,strlen(Name)+1);
					

					msg->joinUoid=new char[20];
					memcpy(msg->joinUoid,&totalMsg[1],20); // putting the join reply uoid

					//cout<<"------------------------------------------->MSG join uoid"<<endl;
					//for (int i =0;i<SHA_DIGEST_LENGTH;i++)
						//printf("%02x", (unsigned char)msg->joinUoid[i]);
					//cout<<endl;
					//cout<<"------------------------------------------------>total uoid"<<endl;
					//for (int i =0;i<SHA_DIGEST_LENGTH;i++)
						//printf("%02x", (unsigned char)totalMsg[1+i]);
					//printf("\n");
					

					uint32_t otherLoc;
					memcpy(&otherLoc,&totalMsg[27],sizeof(otherLoc));
					uint32_t dist;
					//dist=abs((int)otherLoc-(int)location);
					if(otherLoc>=location)
					{
						dist=otherLoc-location;
					}
					else
					{
						dist=location-otherLoc;
					}
	
					msg->distance=dist; // setting the distance
					head->dataLen=sizeof(msg->hostPort)+strlen(Name)+20/*added for uoid*/+sizeof(msg->distance); // setting the datalen in the header

					

					// msg created copying the contents into the a char buffer
					msgBuf=new unsigned char[HSIZE+head->dataLen];
					memset(msgBuf,'\0',headerSize+head->dataLen);
					memcpy(msgBuf,&(head->type),sizeof(head->type));
					memcpy(&msgBuf[1],(head->uoid),strlen(head->uoid));
					memcpy(&msgBuf[21],&(head->ttl),sizeof(head->ttl));
					memcpy(&msgBuf[22],&(head->reserved),sizeof(head->reserved));
					memcpy(&msgBuf[23],&(head->dataLen),sizeof(head->dataLen));
					memcpy(&msgBuf[27],msg->joinUoid,20/*for uoid len */);
					memcpy(&msgBuf[47],&(msg->distance),sizeof(msg->distance));
					memcpy(&msgBuf[51],&(msg->hostPort),sizeof(msg->hostPort));
					memcpy(&msgBuf[53],msg->hostName,strlen(msg->hostName));
					//cout<<"------------------------------------------------>final buf uoid"<<endl;
					//for (int i =0;i<SHA_DIGEST_LENGTH;i++)
						//printf("%02x", (unsigned char)msgBuf[27+i]);
					//printf("\n");
					delete(head);
					delete(msg->joinUoid);
					delete(msg->hostName);
					delete(msg);
					delete(Name);
				break;
			}
			case STRS:
			{
					header *head=new header();
					head->type=STRS; // setting the msg type to join response
					head->uoid=GetUOID(nodeInstanceId,obj,20); // setting a new uoid
					head->ttl=ttl;	// setting the ttl to the nodes ttl
					head->reserved=0;	// setting the reserved to zero
					//int headerSize=sizeof(head->type)+strlen(head->uoid)+sizeof(head->ttl)+sizeof(head->reserved)+sizeof(head->dataLen);

					statusReply *msg=new statusReply();
					msg->statusReqUoid=new char[20];
					memcpy(msg->statusReqUoid,&totalMsg[1],20);

					msg->hostPort=port;
					
					char *Name=new char[15];
					memset(Name,'\0',15);
					gethostname(Name,15);
					
					msg->hostName=new char[strlen(Name)+1];
					memset(msg->hostName,'\0',strlen(Name)+1);
					memcpy(msg->hostName,Name,strlen(Name));

					msg->hostInfoLength=sizeof(msg->hostPort)+strlen(msg->hostName);

					head->dataLen=20+sizeof(msg->hostInfoLength)+msg->hostInfoLength;

					/*
						
						struct statusRecord
							{
								uint32_t recordLen;
								char *recordData;
							};

						
							struct conDetails
								{
									uint16_t peerPort;
									//char *peerName;
									int sockFd;
									int init;
								};

					*/

					vector<statusRecord *> eachRecord;
					vector<int> eachRecordLen;

					pthread_mutex_lock(conVectorLock);
					for(unsigned int i=0;i<conVector.size();i++)
					{
						conDetails *con=conVector[i];
						uint16_t tempPort=con->peerPort;
						char *cName=new char[15];
						memset(cName,'\0',15);
						sprintf(cName,"nunki.usc.edu");
						int recordLen=sizeof(tempPort)+strlen(cName);
						statusRecord *newRecord=new statusRecord();
						newRecord->recordLen=recordLen;
						newRecord->recordData=new char[newRecord->recordLen];
						memcpy(newRecord->recordData,&tempPort,sizeof(tempPort));
						memcpy(&newRecord->recordData[sizeof(tempPort)],cName,strlen(cName));

						eachRecord.push_back(newRecord);
						eachRecordLen.push_back(recordLen);
					}
					pthread_mutex_unlock(conVectorLock);
					for(unsigned int i=0;i<eachRecordLen.size();i++)
					{
						head->dataLen=head->dataLen+eachRecordLen[i]+sizeof(uint32_t);
					}

					msgBuf=new unsigned char[HSIZE+head->dataLen];
					//memset(msgBuf,'\0',HSIZE+head->dataLen);
					memcpy(&msgBuf[0],&(head->type),sizeof(head->type));
					memcpy(&msgBuf[1],(head->uoid),strlen(head->uoid));
					memcpy(&msgBuf[21],&(head->ttl),sizeof(head->ttl));
					memcpy(&msgBuf[22],&(head->reserved),sizeof(head->reserved));
					memcpy(&msgBuf[23],&(head->dataLen),sizeof(head->dataLen));

					memcpy(&msgBuf[27],msg->statusReqUoid,20/*for uoid len */);
					memcpy(&msgBuf[47],&(msg->hostInfoLength),sizeof(uint16_t));
					memcpy(&msgBuf[49],&(msg->hostPort),sizeof(uint16_t));
					memcpy(&msgBuf[51],(msg->hostName),strlen(msg->hostName));

					int pos=51+strlen(msg->hostName);
					//cout<<"Pos is "<<pos<<endl;
					
					for(unsigned int i=0;i<eachRecord.size();i++)
					{
						statusRecord *currentRecord=eachRecord[i];
						memcpy(&msgBuf[pos],&currentRecord->recordLen,sizeof(currentRecord->recordLen));
						pos=pos+sizeof(currentRecord->recordLen);
						//cout<<"Pos is "<<pos<<endl;
						memcpy(&msgBuf[pos],currentRecord->recordData,currentRecord->recordLen);
						pos=pos+currentRecord->recordLen;
					}

				//cout<<"Total Len is "<<HSIZE+head->dataLen<<endl;
				break;
					

			}
			case CKRS:
			{
				header *head=new header();
				head->type=CKRS; // setting the msg type to join response
				head->uoid=GetUOID(nodeInstanceId,obj,20); // setting a new uoid
				head->ttl=ttl;	// setting the ttl to the nodes ttl
				head->reserved=0;	// setting the reserved to zero
				int headerSize=sizeof(head->type)+strlen(head->uoid)+sizeof(head->ttl)+sizeof(head->reserved)+sizeof(head->dataLen);
				
				checkReply *msg=new checkReply();
				msg->checkUoid=new char[20];
				memcpy(msg->checkUoid,&totalMsg[1],20);
				head->dataLen=20;

				msgBuf=new unsigned char[HSIZE+head->dataLen];
				memset(msgBuf,'\0',headerSize+head->dataLen);
				memcpy(msgBuf,&(head->type),sizeof(head->type));
				memcpy(&msgBuf[1],(head->uoid),strlen(head->uoid));
				memcpy(&msgBuf[21],&(head->ttl),sizeof(head->ttl));
				memcpy(&msgBuf[22],&(head->reserved),sizeof(head->reserved));
				memcpy(&msgBuf[23],&(head->dataLen),sizeof(head->dataLen));
				memcpy(&msgBuf[27],msg->checkUoid,20/*for uoid len */);
				break;
			}
		}
		return msgBuf;
}
/*
This is a helper function which created a msg of the request type and returns a character pointer
*/
unsigned char *createMsg(uint8_t type,uint8_t ec)
{
		unsigned char *msgBuf;
		char *obj=new char[3];
		sprintf(obj,"%s","msg");
		switch (type)
		{
			case HLLO:
			{
					// Creating header
					header *helloHeader=new header();
					helloHeader->type=HLLO;	// setting the msg type to hello 
					
					helloHeader->uoid=GetUOID(nodeInstanceId,obj,20); // setting the uoid in the header
					helloHeader->ttl=1;	// setting the ttl to 1 in the header since this is a hello msg 
					helloHeader->reserved=0; // setting the reserved in the header
					int headerSize=sizeof(helloHeader->type)+strlen(helloHeader->uoid)+sizeof(helloHeader->ttl)+sizeof(helloHeader->reserved)+sizeof(helloHeader->dataLen);
					//cout<<" Header size is "<<headerSize<<endl;
					
					// Creating msg 
					hello *msg=new hello();
					char *Name=new char[15];
					memset(Name,'\0',15);
					gethostname(Name,15);
					msg->hostPort=port; // seeting the host port
					msg->hostName=new char[strlen(Name)+1]; // setting he host name
					memset(msg->hostName,'\0',strlen(Name)+1);
					memcpy(msg->hostName,Name,strlen(Name)+1);
					helloHeader->dataLen=sizeof(msg->hostPort)+strlen(Name); // setting the msg size in the header
					
					
					// msg created copying the contents into the a char buffer
					msgBuf=new unsigned char[HSIZE+helloHeader->dataLen];
					memset(msgBuf,'\0',headerSize+helloHeader->dataLen);
					memcpy(msgBuf,&(helloHeader->type),sizeof(helloHeader->type));
					memcpy(&msgBuf[1],(helloHeader->uoid),strlen(helloHeader->uoid));
					memcpy(&msgBuf[21],&(helloHeader->ttl),sizeof(helloHeader->ttl));
					memcpy(&msgBuf[22],&(helloHeader->reserved),sizeof(helloHeader->reserved));
					memcpy(&msgBuf[23],&(helloHeader->dataLen),sizeof(helloHeader->dataLen));
					memcpy(&msgBuf[27],&(msg->hostPort),sizeof(msg->hostPort));
					memcpy(&msgBuf[29],msg->hostName,strlen(msg->hostName));
					
					delete(Name);
					delete(helloHeader->uoid);
					delete(helloHeader);

					delete(msg->hostName);
					delete(msg);
				break;
			}
			case JNRQ:
			{
					header *joinHeader=new header();
					joinHeader->type=JNRQ; // setting the msg type to join req
					joinHeader->uoid=GetUOID(nodeInstanceId,obj,20);
					joinHeader->ttl=ttl;	// setting the ttl to the nodes ttl
					joinHeader->reserved=0;	// setting the reserved to zero
					int headerSize=sizeof(joinHeader->type)+strlen(joinHeader->uoid)+sizeof(joinHeader->ttl)+sizeof(joinHeader->reserved)+sizeof(joinHeader->dataLen);

					joinMsg *msg=new joinMsg();
					char *Name=new char[15];
					memset(Name,'\0',15);
					gethostname(Name,15);
					msg->hostLoc=location;	// setting the location in the header
					msg->hostPort=port;		// setting the host port in the msg 
					//cout<<"The PORT NUBER IS +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++>>>>>>>>>>>>>>>>>>"<<msg->hostPort<<endl;
					msg->hostName=new char[strlen(Name)+1]; // setting the host name
					memset(msg->hostName,'\0',strlen(Name)+1);
					memcpy(msg->hostName,Name,strlen(Name)+1);
					joinHeader->dataLen=sizeof(msg->hostPort)+strlen(Name)+sizeof(msg->hostLoc); // setting the datalen in the header
					//cout<<"______________"<<HSIZE+joinHeader->dataLen<<endl;
					

					// msg created copying the contents into the a char buffer
					msgBuf=new unsigned char[HSIZE+joinHeader->dataLen];
					memset(msgBuf,'\0',headerSize+joinHeader->dataLen);
					memcpy(msgBuf,&(joinHeader->type),sizeof(joinHeader->type));
					memcpy(&msgBuf[1],(joinHeader->uoid),strlen(joinHeader->uoid));
					memcpy(&msgBuf[21],&(joinHeader->ttl),sizeof(joinHeader->ttl));
					memcpy(&msgBuf[22],&(joinHeader->reserved),sizeof(joinHeader->reserved));
					memcpy(&msgBuf[23],&(joinHeader->dataLen),sizeof(joinHeader->dataLen));
					memcpy(&msgBuf[27],&(msg->hostLoc),sizeof(msg->hostLoc));
					memcpy(&msgBuf[31],&(msg->hostPort),sizeof(msg->hostPort));
					memcpy(&msgBuf[33],msg->hostName,strlen(msg->hostName));
					
					delete(Name);
					delete(joinHeader->uoid);
					delete(joinHeader);

					delete(msg->hostName);
					delete(msg);
					
				break;	
			}
			case KPAV:
			{
					// Creating header
					header *helloHeader=new header();
					helloHeader->type=KPAV;	// setting the msg type to hello 
					
					helloHeader->uoid=GetUOID(nodeInstanceId,obj,20); // setting the uoid in the header
					helloHeader->ttl=1;	// setting the ttl to 1 in the header since this is a hello msg 
					helloHeader->reserved=0; // setting the reserved in the header
					int headerSize=sizeof(helloHeader->type)+strlen(helloHeader->uoid)+sizeof(helloHeader->ttl)+sizeof(helloHeader->reserved)+sizeof(helloHeader->dataLen);
					//cout<<" Header size is "<<headerSize<<endl;				
					helloHeader->dataLen=0;
					
					
					// msg created copying the contents into the a char buffer
					msgBuf=new unsigned char[HSIZE];
					memset(msgBuf,'\0',headerSize+helloHeader->dataLen);
					memcpy(msgBuf,&(helloHeader->type),sizeof(helloHeader->type));
					memcpy(&msgBuf[1],(helloHeader->uoid),strlen(helloHeader->uoid));
					memcpy(&msgBuf[21],&(helloHeader->ttl),sizeof(helloHeader->ttl));
					memcpy(&msgBuf[22],&(helloHeader->reserved),sizeof(helloHeader->reserved));
					memcpy(&msgBuf[23],&(helloHeader->dataLen),sizeof(helloHeader->dataLen));
					
					delete(helloHeader->uoid);
					delete(helloHeader);
				break;
			}
			case STRQ:
			{
				

				header *head=new header();
				head->type=STRQ;	// setting the msg type to hello 
					
				head->uoid=GetUOID(nodeInstanceId,obj,20); // setting the uoid in the header
				head->ttl=statusTTL;	// setting the ttl to 1 in the header since this is a hello msg 
				head->reserved=0; // setting the reserved in the header
				int headerSize=sizeof(head->type)+strlen(head->uoid)+sizeof(head->ttl)+sizeof(head->reserved)+sizeof(head->dataLen);

				statusRequest *stReq=new statusRequest();
				stReq->type=STN;

				head->dataLen=sizeof(stReq->type);
					
				msgBuf=new unsigned char[HSIZE+head->dataLen];
				memset(msgBuf,'\0',headerSize+head->dataLen);
				memcpy(msgBuf,&(head->type),sizeof(head->type));
				memcpy(&msgBuf[1],(head->uoid),strlen(head->uoid));
				memcpy(&msgBuf[21],&(head->ttl),sizeof(head->ttl));
				memcpy(&msgBuf[22],&(head->reserved),sizeof(head->reserved));
				memcpy(&msgBuf[23],&(head->dataLen),sizeof(head->dataLen));
				memcpy(&msgBuf[27],&(stReq->type),sizeof(stReq->type));

				delete(head->uoid);
				delete(head);
				delete(stReq);

				break;
			}
			case CKRQ:
			{
					//cout<<"\n creating check msg"<<endl;

					header *helloHeader=new header();
					helloHeader->type=CKRQ;	// setting the msg type to hello 
					
					helloHeader->uoid=GetUOID(nodeInstanceId,obj,20); // setting the uoid in the header
					helloHeader->ttl=ttl;	// setting the ttl to 1 in the header since this is a hello msg 
					helloHeader->reserved=0; // setting the reserved in the header
					int headerSize=sizeof(helloHeader->type)+strlen(helloHeader->uoid)+sizeof(helloHeader->ttl)+sizeof(helloHeader->reserved)+sizeof(helloHeader->dataLen);
					//cout<<" Header size is "<<headerSize<<endl;				
					helloHeader->dataLen=0;
					
					
					// msg created copying the contents into the a char buffer
					msgBuf=new unsigned char[HSIZE+helloHeader->dataLen];
					memset(msgBuf,'\0',headerSize+helloHeader->dataLen);
					memcpy(msgBuf,&(helloHeader->type),sizeof(helloHeader->type));
					memcpy(&msgBuf[1],(helloHeader->uoid),strlen(helloHeader->uoid));
					memcpy(&msgBuf[21],&(helloHeader->ttl),sizeof(helloHeader->ttl));
					memcpy(&msgBuf[22],&(helloHeader->reserved),sizeof(helloHeader->reserved));
					memcpy(&msgBuf[23],&(helloHeader->dataLen),sizeof(helloHeader->dataLen));
					
					delete(helloHeader->uoid);
					delete(helloHeader);
				break;

			}
			case NTFY:
			{

					header *helloHeader=new header();
					helloHeader->type=NTFY;	// setting the msg type to hello 
					
					helloHeader->uoid=GetUOID(nodeInstanceId,obj,20); // setting the uoid in the header
					helloHeader->ttl=1;	// setting the ttl to 1 in the header since this is a hello msg 
					helloHeader->reserved=0; // setting the reserved in the header
					int headerSize=sizeof(helloHeader->type)+strlen(helloHeader->uoid)+sizeof(helloHeader->ttl)+sizeof(helloHeader->reserved)+sizeof(helloHeader->dataLen);
					//cout<<" Header size is "<<headerSize<<endl;				
					helloHeader->dataLen=1;
					
					
					// msg created copying the contents into the a char buffer
					msgBuf=new unsigned char[HSIZE+helloHeader->dataLen];
					memset(msgBuf,'\0',headerSize+helloHeader->dataLen);
					memcpy(msgBuf,&(helloHeader->type),sizeof(helloHeader->type));
					memcpy(&msgBuf[1],(helloHeader->uoid),strlen(helloHeader->uoid));
					memcpy(&msgBuf[21],&(helloHeader->ttl),sizeof(helloHeader->ttl));
					memcpy(&msgBuf[22],&(helloHeader->reserved),sizeof(helloHeader->reserved));
					memcpy(&msgBuf[23],&(helloHeader->dataLen),sizeof(helloHeader->dataLen));

					notify *msg=new notify();
					msg->errorCode=ec;
					memcpy(&msgBuf[27],&(msg->errorCode),sizeof(uint8_t));
					delete(helloHeader->uoid);
					delete(helloHeader);

			}
		}

	return msgBuf;
}

/*
This function print the nam vector
struct namDetails
{
	uint16_t node;
	vector<uint16_t> connectedNodes;
};

*/

void printNamVector()
{
	pthread_mutex_lock(namVectorLock);
	for (unsigned int i=0;i<namVector.size() ; i++)
	{
		namDetails *nam=namVector[i];
		//cout<<"Node is "<<nam->node<<endl;
		for(unsigned int j=0;j<nam->connectedNodes.size();j++)
		{
			//cout<<"\t\t connected node is "<<nam->connectedNodes[j]<<endl;
		}

	}
	pthread_mutex_unlock(namVectorLock);
}

void printNeighbors()
{
	for(unsigned int i=0;i<neighborVector.size();i++)
	{
		//neighborDetails *neighbor=neighborVector[i];
		//cout<<"N-Name is "<<neighbor->nName<<endl;
		//cout<<"N-Port is "<<neighbor->nPort<<endl;
	}
	return;
}
/*
This function is called to parse the inl file 
*/
void parseINLFile()
{
	//cout<<"------------------>parsing INL"<<endl;
	INLFile=fopen(inl,"r");
	int tempPort;
	char *peerName;
	char *line=new char[25];
	memset(line,'\0',25);
	int i=0;
	while(i<initNeighbors) // i am not reading tikll eof bcos im getting sgh fault
	{
		memset(line,'\0',25);
		fgets (line, 25, INLFile );
		char *values=strtok(line,":");
		peerName=values;
		//cout<<"_________________________"<<endl;
		//cout<<"Peer Name is "<<peerName<<endl;
		
		values = strtok (NULL, ":");
		//char *end;
		tempPort=atoi(values);
		//cout<<"Peer port is "<<tempPort<<endl;
		i++;
		neighborDetails *neighbor=new neighborDetails();
		char *tmp=new char[25];
		memset(tmp,'\0',25);
		memcpy(tmp,peerName,strlen(peerName));
		neighbor->nName=tmp;
		//cout<<"N-Name is "<<neighbor->nName<<endl;
		neighbor->nPort=tempPort;
		neighborVector.push_back(neighbor);
	}
	
	
//	while(fgets ( line, 25 , INLFile ) != NULL)
//	{
//		//line=new char[255];
//		//fscanf(INLFile,"%[^:]:%d\n",peerName,&tempPort); 
//		//cout<<"Peer Name is "<<peerName<<endl;
//		//cout<<"Peer port is "<<tempPort<<endl;
////		neighborDetails *neighbor=new neighborDetails();
////		char *tmp=new char[21];
////		memset(tmp,'\0',21);
////		memcpy(tmp,peerName,strlen(peerName));
////		neighbor->nName=tmp;
////		//cout<<"N-Name is "<<neighbor->nName<<endl;
////		neighbor->nPort=tempPort;
////		neighborVector.push_back(neighbor);
//		cout<<line<<endl;
//		sscanf (line,"%s:%d",peerName,&tempPort);
//		//cout<<"Peer Name is "<<peerName<<endl;
//		//cout<<"Peer port is "<<tempPort<<endl;
//
//		delete(line);
//		line=new char[25];
//		memset(line,'\0',25);
//	}
	delete(line);
	//printNeighbors();
	return;
}

