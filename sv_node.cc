#include"sv_node.h"

void parseIniFile(char *);
void insertCon(int *port);
int checkCon();
void removeCon();
void setNodeId();
void setNodeInstanceId();
unsigned char *GetUOID(char * , char *  , int );
unsigned char *createMsg(uint8_t type,uint8_t ec);
void printCons();
void decTTL(unsigned char *msg);
void printMsgCache();
uint16_t getWellKnownPort(int sockFd);
int checkMsgCache(char *uoid);
unsigned char *createReplyMsg(uint8_t type,unsigned char *totalMsg);
int getRoute(unsigned char *msg);
void parseINLFile(void);
logDetails *createLogEntry(unsigned char *totalMsg,int sockFd,char action);
void printNamVector();
void setKeepAlive(int sockFd);
void printJoinDetails();
//void shutDown();


int checkNetwork()
{
	//cout<<"SENDING CHECK"<<endl;
	unsigned char *msg=createMsg(CKRQ,0);
	pthread_mutex_lock(checkLock);
	check=1;
	pthread_mutex_unlock(checkLock);
	pthread_mutex_lock(networkLock);
	networkOk=0;
	pthread_mutex_unlock(networkLock);
	packet *packToSend=new packet();
	packToSend->msgToSend=msg;
	packToSend->flood=1;
	packToSend->sockFd=0;
	pthread_mutex_lock(packetQLock);
	packetQ.push_back(packToSend);
	pthread_cond_signal(writerWaiting); // singalling the writer thread
	pthread_mutex_unlock(packetQLock);
	int checkTimeOut=joinTimeOut;
	while(checkTimeOut)
	{
		sleep(1);
		checkTimeOut--;
	}
	pthread_mutex_lock(networkLock);
	pthread_mutex_lock(checkLock);
	check=0;
	pthread_mutex_unlock(checkLock);
	if(networkOk==1)
	{
		pthread_mutex_unlock(networkLock);
		//cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ Network is OK "<<endl;
		return 1;
	}
	else
	{
		//cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ Network is ***NOT**** OK "<<endl;
		pthread_mutex_unlock(networkLock);
		return -1;

	}
}


/*
Function to parse the command line argument
*/
char *iniFilename;
void parseCmdLine(int argc, char **args)
{
	
	if(argc>3 || argc < 2)
	{
		//  IF THE COMMAND LINE ARGUMENTS ARE GREATER THEN 3 THEN ITS A MALFORMED COMMAND
		cout<<" Malformed Command "<<endl;
		exit(0);
	}
	else
	{
		// COMMAND IS OK
		for(int i=1;i<argc;i++)
		{
			if(!strncmp(args[argc-1],"-reset",strlen(args[argc-1])))
			{
				cout<<"Malformed Command"<<endl;
				exit(0);
			}
			iniFilename=new char[(strlen(args[argc-1])+1)];
			memset(iniFilename,'\0',strlen(args[argc-1])+1);
			strncpy(iniFilename,args[argc-1],strlen(args[argc-1]));
			if(!strncmp(args[i],"-reset",strlen(args[i])))
			{
				// NODE RESET OPTION IS GIVEN
				//cout<<"Reseting node "<<endl;
				reset=1;
			}
		}
		//cout<<" The ini file name is "<<iniFilename<<endl;
		parseIniFile(iniFilename);
		
	}
 //delete(iniFilename);	
}

/*
A common write function which will write to all the sockets
Forked from the main thread
*/
void *commonWriterFn(void *arg)
{
	while(1)
	{
		pthread_testcancel();
		pthread_mutex_lock(packetQLock);
		while((int)packetQ.size()==0)
		{
			//cout<<"No more msg to send writer waiting"<<endl;
			pthread_cond_wait(writerWaiting,packetQLock);
			pthread_testcancel();
		}
		//cout<<"Writer I have something to send"<<endl;
		packet *pac;
		pac=packetQ.front(); // getting the packet to write
		packetQ.pop_front();
		pthread_mutex_unlock(packetQLock);
		//cout<<"Flood is "<<pac->flood<<endl;
		if(!(pac->flood))
		{
			// This msg should not be flooded
			//cout<<"This msg is not a flood msg "<<endl;
			int toSock=pac->sockFd;
			
			// logging the send msg 
			logDetails *log=createLogEntry(pac->msgToSend,toSock,'s');
			pthread_mutex_lock(logEntryListLock);
			logEntryList.push_back(log);
			pthread_cond_signal(logEntryListEmptyCV);
			pthread_mutex_unlock(logEntryListLock);

			unsigned char *dataToSend=pac->msgToSend;
			uint32_t dataLen=0;
			memcpy(&dataLen,&dataToSend[23],sizeof(uint32_t));
			for(unsigned int i=0;i<HSIZE+dataLen;i++)
			{
				pthread_testcancel();
				send(toSock,&dataToSend[i],1,0); // sending the packet
				pthread_testcancel();
			}
			delete(dataToSend); // deleting the msg after seding it
		}
		else
		{
			//cout<<" the msg should be flooded "<<endl;
			printCons();
			unsigned char *dataToSend=pac->msgToSend;
			pthread_mutex_lock(conVectorLock);
			for(unsigned int i=0;i<conVector.size();i++)
			{
				conDetails *con=conVector[i];
				if(con->sockFd!=pac->sockFd)
				{
					logDetails *log;
					int toSock=con->sockFd;
					if(pac->sockFd==0)
					{
						log=createLogEntry(pac->msgToSend,toSock,'s');
					}
					else
					{
						log=createLogEntry(pac->msgToSend,toSock,'f');
					}
					pthread_mutex_lock(logEntryListLock);
					logEntryList.push_back(log);
					pthread_cond_signal(logEntryListEmptyCV);
					pthread_mutex_unlock(logEntryListLock);

					uint32_t dataLen=0;
					memcpy(&dataLen,&dataToSend[23],sizeof(uint32_t));
					for(unsigned int i=0;i<HSIZE+dataLen;i++)
					{
						pthread_testcancel();
						send(toSock,&dataToSend[i],1,0); // sending the packet
						pthread_testcancel();
					}
				}
			}
			delete(dataToSend); // deleting the memory after using sending the msg 
			pthread_mutex_unlock(conVectorLock);
		}
		
	}
	pthread_exit(NULL);
}

/*
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

This logWriter thread is forked from the main thread
writes data into the logfile
*/

void *logWritter(void *arg)
{
	logFile=fopen(logFilePath,"a+");
	if(logFile==NULL)
	{
		cout<<"ERROR opening file "<<endl;
	}
	else
	{
		//cout<<"Log writer up and runnig "<<endl;
	}
	while(1)
	{
		pthread_testcancel();
		pthread_mutex_lock(logEntryListLock);
		while(logEntryList.size()==0)
		{
			//cout<<"log Writter sleeping "<<endl;
			pthread_cond_wait(logEntryListEmptyCV,logEntryListLock);
			pthread_testcancel();
		}
		logDetails *logRecord=logEntryList.front();
		logEntryList.pop_front();
		pthread_mutex_unlock(logEntryListLock);
		
		fprintf(logFile,"%c ",logRecord->action);
		fflush(logFile);
		fprintf(logFile,"%10ld.%03ld ",logRecord->msgTime.tv_sec,logRecord->msgTime.tv_usec/1000);
		fflush(logFile);
		fprintf(logFile,"%s ",logRecord->fromTo);
		fflush(logFile);
		switch(logRecord->type)
		{
			case HLLO:
			{
				fprintf(logFile,"%s ","HLLO");
				fflush(logFile);
				break;
			}
			case JNRQ:
			{
				fprintf(logFile,"%s ","JNRQ");
				fflush(logFile);
				break;
			}
			case JNRS:
			{
				fprintf(logFile,"%s ","JNRS");
				fflush(logFile);
				break;
			}
			case KPAV:
			{
				fprintf(logFile,"%s ","KPAV");
				fflush(logFile);
				break;
			}
			case STRQ:
			{
				fprintf(logFile,"%s ","STRQ");
				fflush(logFile);
				break;
			}
			case STRS:
			{
				fprintf(logFile,"%s ","STRS");
				fflush(logFile);
				break;
			}
			case NTFY:
			{
				fprintf(logFile,"%s ","NTFY");
				fflush(logFile);
				break;

			}
			case CKRQ:
			{
				fprintf(logFile,"%s ","CKRQ");
				fflush(logFile);
				break;
				break;
			}
			case CKRS:
			{
				fprintf(logFile,"%s ","CKRS");
				fflush(logFile);
				break;
				break;
			}
		}
		fprintf(logFile,"%d ",logRecord->size);
		fflush(logFile);
		fprintf(logFile,"%d ",(int)logRecord->ttl);
		fflush(logFile);
		for (int i =0;i<4;i++)
	   		fprintf(logFile,"%02x", (unsigned char)logRecord->msgId[i]);
	   	fflush(logFile);
	   	switch(logRecord->type)
	   	{
	   		case HLLO:
	   		{
	   			uint16_t tmpPort;
	   			memcpy(&tmpPort,(uint16_t *)&logRecord->data[0],sizeof(uint16_t));
	   			fprintf(logFile," %d ",(int )tmpPort);
	   			fflush(logFile);

	   			char *tmpHost=new char[logRecord->size-sizeof(tmpPort)+1];
	   			memset(tmpHost,'\0',logRecord->size-sizeof(tmpPort)+1);
	   			memcpy(tmpHost,(char *)&logRecord->data[2],logRecord->size-sizeof(tmpPort));
	   			//cout<<" HLLO tmp host is "<<tmpHost<<endl;
	   			fprintf(logFile,"%s",tmpHost);
	   			fflush(logFile);
				delete(tmpHost);
	   			break;
	   		}
			case JNRQ:
			{
				uint16_t tmpPort;
				memcpy(&tmpPort,(uint16_t *)&logRecord->data[4],sizeof(uint16_t));
				fprintf(logFile," %d ",(int )tmpPort);
	   			fflush(logFile);

				char *tmpHost=new char[logRecord->size-sizeof(tmpPort)+1];
	   			memset(tmpHost,'\0',logRecord->size-sizeof(tmpPort)+1);
	   			memcpy(tmpHost,(char *)&logRecord->data[6],logRecord->size-(sizeof(tmpPort)+4));
	   			//cout<<" JNRQ  tmp host is "<<tmpHost<<endl;
	   			fprintf(logFile,"%s",tmpHost);
	   			fflush(logFile);
				delete(tmpHost);
				break;
			}
			case JNRS:
			{
				
				fprintf(logFile," ");
	   			fflush(logFile);
				// printing uoid
				char *tmpUoid=new char[4];
				memcpy(tmpUoid,&logRecord->data[16],4);
				for (int i =0;i<4;i++)
	   				fprintf(logFile,"%02x", (unsigned char)tmpUoid[i]);
	   			fflush(logFile);
				
				// printing distance
				uint32_t tmpDist;
				memcpy(&tmpDist,(uint32_t *)&logRecord->data[20],sizeof(uint32_t));
				fprintf(logFile," %d ",(int )tmpDist);
	   			fflush(logFile);
				
				// printing port
				uint16_t tmpPort;
				memcpy(&tmpPort,(uint16_t *)&logRecord->data[24],sizeof(uint16_t));
				fprintf(logFile,"%d ",(int )tmpPort);
	   			fflush(logFile);
				
				// printing host name
				char *tmpHost=new char[logRecord->size];
	   			memset(tmpHost,'\0',logRecord->size);
	   			memcpy(tmpHost,(char *)&logRecord->data[26],logRecord->size-(sizeof(tmpPort)+sizeof(tmpDist)+20));
	   			//cout<<" JNRS  tmp host is "<<tmpHost<<endl;
	   			fprintf(logFile,"%s",tmpHost);
	   			fflush(logFile);

				delete(tmpUoid);
				delete(tmpHost);


				break;
			}
			case KPAV:
			{
				// nopthing to write for kpav data ois empty
				break;
			}
			case STRQ:
			{
				uint8_t type;
				memcpy(&type,(uint8_t *)&logRecord->data[0],sizeof(uint8_t));
				switch(type)
				{
					case STN:
					{
						fprintf(logFile," %s","STN");
						fflush(logFile);
						break;
					}
					default:
					{
						cout<<"OOOOOOOOOOOOOOOOOOPSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS ERROPR"<<endl;
						break;
					}
				}
				break;
			}
			case STRS:
			{
				fprintf(logFile," ");
				fflush(logFile);
				for (int i =0;i<4;i++)
					fprintf(logFile,"%02x", (unsigned char)logRecord->data[i]);
				fflush(logFile);
				break;
			}
			case NTFY:
			{
				uint8_t ec;
				memcpy(&ec,(uint8_t *)&logRecord->data[0],sizeof(uint8_t));
				fprintf(logFile," %d",ec);
				fflush(logFile);
				break;
			}
			case CKRQ:
			{
				break;
			}
			case CKRS:
			{
				fprintf(logFile," ");
				fflush(logFile);
				for (int i =0;i<4;i++)
					fprintf(logFile,"%02x", (unsigned char)logRecord->data[i]);
				fflush(logFile);
				break;
			}
	   	}
	 	fprintf(logFile,"\n");
		fflush(logFile);
	}
	fclose(logFile);
	pthread_exit(NULL);
}

/*
This function is called to handle a incomming connection
Forked from the handle client thead
*/
void *handleConFn(void *arg)
{
	int *dummyPtr=(int *)arg;
	int sockFd=*dummyPtr;
	delete(dummyPtr);
	int myinit=0;
	uint8_t type;
	do
	{
		//cout<<"Inside Do while"<<endl;
		unsigned char *headerBuf=new unsigned char[HSIZE];
		for(int i=0;i<HSIZE;i++)
		{
			pthread_testcancel();
			if(recv(sockFd,&headerBuf[i],1,0)!=1)// receiving header
			{
				pthread_testcancel();
				//cout<<"------------------------------------------------------------------------ >Recv H Failed "<<endl;
				close(sockFd);
				
						pthread_mutex_lock(joinVectorLock);
						for(unsigned int i=0;i<joinVector.size();i++)
						{
							joinDetails *jd=joinVector[i];
							if(jd->sockFd==sockFd)
							{
								//cout<<"Removing from JoinVector"<<endl;
								joinVector.erase(joinVector.begin()+i);
							}
						}
						pthread_mutex_unlock(joinVectorLock);
						
						pthread_mutex_lock(conVectorLock);
						for(unsigned int i=0;i<conVector.size();i++)
						{
							conDetails *con=conVector[i];
							if(con->sockFd==sockFd)
							{
							conVector.erase(conVector.begin()+i);
							}
						}
						pthread_mutex_unlock(conVectorLock);
				
				
				
				pthread_exit(NULL);
			}
			pthread_testcancel();
		}
		uint8_t type;
		uint32_t msgLen=0;
		memcpy(&msgLen,&headerBuf[23],sizeof(msgLen));
		memcpy(&type,(uint8_t *)&headerBuf[0],sizeof(uint8_t));
		//cout<<"Con : Received msg len is "<<msgLen<<endl;
		unsigned char *data=new unsigned char[msgLen];
		for(unsigned int i=0;i<msgLen;i++)
		{
			pthread_testcancel();
			if(recv(sockFd,&data[i],1,0)!=1)	// receiving data
			{
				//cout<<"------------------------------------------------------------------------ >Recv D Failed"<<endl;
				pthread_testcancel();
				close(sockFd);
				
				
						pthread_mutex_lock(joinVectorLock);
						for(unsigned int i=0;i<joinVector.size();i++)
						{
							joinDetails *jd=joinVector[i];
							if(jd->sockFd==sockFd);
							{
								//cout<<"Removing from JoinVector"<<endl;
								joinVector.erase(joinVector.begin()+i);
							}
						}
						pthread_mutex_unlock(joinVectorLock);
						
						pthread_mutex_lock(conVectorLock);
						for(unsigned int i=0;i<conVector.size();i++)
						{
							conDetails *con=conVector[i];
							if(con->sockFd==sockFd);
							{
							conVector.erase(conVector.begin()+i);
							}
						}
						pthread_mutex_unlock(conVectorLock);
				
				
				
				pthread_exit(NULL);
			}
			pthread_testcancel();
		}

		// combining the header and data 
		unsigned char *totalMsg=new unsigned char[HSIZE+msgLen];
		for(unsigned int i=0,j=0;i<HSIZE+msgLen;i++)
		{
			if(i<HSIZE)
			{
				totalMsg[i]=headerBuf[i];
			}
			else
			{
				totalMsg[i]=data[j];
				j++;
			}
		}
		int totalSize=HSIZE+msgLen;
		setKeepAlive(sockFd);
		switch(type)
		{
			case HLLO:
			{
				// the recevied msg is hello 
				//cout<<"The received msg is hello"<<endl;

				char *uoid=new char[20];
				memcpy(uoid,(char *)&totalMsg[1],20);
				if(checkMsgCache(uoid)==0)
				{

					uint16_t peerPort;
					memcpy(&peerPort,&data[0],sizeof(uint16_t));
					//cout<<"The peer that send hello is : port :"<<peerPort<<endl;

					
					// updating the connection vector
					conDetails *con=new conDetails();
					con->peerPort=peerPort;
					//con->peerName=peerName;
					con->init=myinit;
					con->sockFd=sockFd;
					pthread_mutex_lock(conVectorLock);
					conVector.push_back(con); 
					pthread_mutex_unlock(conVectorLock);
					
					//updating nInfo Vector for logging
					nInfo *nei=new nInfo();
					nei->nodeId=new char[25];
					memset(nei->nodeId,'\0',25);
					sprintf(nei->nodeId,"nunki.usc.edu_%d",peerPort);
					//cout<<"nei node ID is "<<nei->nodeId<<endl;
					nei->sockFd=sockFd;
					
					pthread_mutex_lock(nInfoVectorLock);
					nInfoVector.push_back(nei);
					pthread_mutex_unlock(nInfoVectorLock);

					// creating log entry and then pushing 
					logDetails *log=createLogEntry(totalMsg,sockFd,'r');
					pthread_mutex_lock(logEntryListLock);
					logEntryList.push_back(log);
					pthread_cond_signal(logEntryListEmptyCV);
					pthread_mutex_unlock(logEntryListLock);
				
					// creating a msg cache and populating it
					messageCache *msgInfo=new messageCache();
					msgInfo->msgType=HLLO;
					msgInfo->uoid=new char[21];
					memset(msgInfo->uoid,'\0',21);
					memcpy(msgInfo->uoid,&totalMsg[1],20);
					msgInfo->lifeTime=msgLifeTime;
					msgInfo->fromSock=sockFd;
					msgInfo->peerPort=getWellKnownPort(sockFd);
					if(msgInfo->peerPort==0)
					{
						//cout<<"+++++++++++++++++++++++++++ TEMP CONNECTION JNRS "<<msgInfo->peerPort<<endl;
						memcpy(&msgInfo->peerPort,&totalMsg[31],sizeof(uint16_t));
						//cout<<"+++++++++++++++++++++++++++++ after debug "<<msgInfo->peerPort<<endl;
					}
						
					
					// pushing msg into the message cache vector
					pthread_mutex_lock(msgCacheLock);
					msgCacheVector.push_back(msgInfo);
					pthread_mutex_unlock(msgCacheLock);
					

					
					//printing vector after the push
					printMsgCache();

					unsigned char *msg=createMsg(HLLO,0);
					packet *packToSend=new packet();
					packToSend->msgToSend=msg;
					packToSend->flood=0;
					packToSend->sockFd=sockFd;
					pthread_mutex_lock(packetQLock);
					packetQ.push_back(packToSend);
					pthread_cond_signal(writerWaiting); // singalling the writer thread
					pthread_mutex_unlock(packetQLock);


				}
				delete(uoid);

				// creating the reply hello msg 
				
				
									// creating a log entry and then pushing
			break;
			}
			case JNRQ:
			{
				//cout<<"The received msg is joinreq "<<endl;
				decTTL(totalMsg);
				uint8_t ttl;
				memcpy(&ttl,&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
				//cout<<"TLL is after decrementing is before sending  "<<(int )ttl<<endl;
				// checking for duplicate msg before inerting msg into the msg cache
				
				uint16_t joinPort;
				memcpy(&joinPort,&totalMsg[31],sizeof(uint16_t));
				
				// creating a join detail
				joinDetails *jd=new joinDetails();
				jd->peerPort=(int)joinPort;
				jd->sockFd=sockFd;

				// pushing join detail into join vector
				pthread_mutex_lock(joinVectorLock);
				joinVector.push_back(jd);
				pthread_mutex_unlock(joinVectorLock);
				
				printJoinDetails();
				
				char *uoid=new char[20];
				memcpy(uoid,(char *)&totalMsg[1],20);
				if(checkMsgCache(uoid)==0)
				{
						// msg not there in cache should cache it
						//should send a join reply
						//cout<<"------------------------------------------------------------------------------------------------->should send a join reply"<<endl;
						
						// creating a msg cache and populating it
						messageCache *msgInfo=new messageCache();
						msgInfo->msgType=JNRQ;
						msgInfo->uoid=new char[21];
						memset(msgInfo->uoid,'\0',21);
						memcpy(msgInfo->uoid,&totalMsg[1],20);
						msgInfo->lifeTime=msgLifeTime;
						msgInfo->fromSock=sockFd;
						msgInfo->peerPort=getWellKnownPort(sockFd);
						if(msgInfo->peerPort==0)
						{
							//cout<<"+++++++++++++++++++++++++++ TEMP CONNECTION JNRS "<<msgInfo->peerPort<<endl;
							memcpy(&msgInfo->peerPort,&totalMsg[31],sizeof(uint16_t));
							//cout<<"+++++++++++++++++++++++++++++ after debug "<<msgInfo->peerPort<<endl;
						}
						
						// pushing msg into the message cache vector
						pthread_mutex_lock(msgCacheLock);
						msgCacheVector.push_back(msgInfo);
						pthread_mutex_unlock(msgCacheLock);
						
						nInfo *nei=new nInfo();
						nei->nodeId=new char[25];
						memset(nei->nodeId,'\0',25);
						sprintf(nei->nodeId,"nunki.usc.edu_%d",msgInfo->peerPort);
						//cout<<"nei node ID is "<<nei->nodeId<<endl;
						nei->sockFd=sockFd;
				
						pthread_mutex_lock(nInfoVectorLock);
						nInfoVector.push_back(nei);
						pthread_mutex_unlock(nInfoVectorLock);


						// creating log entry and then pushing 
						logDetails *log=createLogEntry(totalMsg,sockFd,'r');
						pthread_mutex_lock(logEntryListLock);
						logEntryList.push_back(log);
						pthread_cond_signal(logEntryListEmptyCV);
						pthread_mutex_unlock(logEntryListLock);
						
						//printing vector after the push
						printMsgCache();

						// creating a join reply msg
						
						unsigned char *msg=createReplyMsg(JNRS,totalMsg);
						packet *packToSend=new packet();
						packToSend->msgToSend=msg;
						packToSend->flood=0;
						packToSend->sockFd=sockFd;
						pthread_mutex_lock(packetQLock);
						packetQ.push_back(packToSend);
						pthread_cond_signal(writerWaiting); // singalling the writer thread
						pthread_mutex_unlock(packetQLock);

					if(ttl>0)
					{
						// flooding the join req
						packet *packToSend=new packet();
						packToSend->msgToSend=totalMsg;
						packToSend->flood=1;
						packToSend->sockFd=sockFd;
						pthread_mutex_lock(packetQLock);
						packetQ.push_back(packToSend);
						pthread_cond_signal(writerWaiting); // singalling the writer thread
						pthread_mutex_unlock(packetQLock);

					}	
						

				}
				delete(uoid);
				

			
				break;
			}
			case JNRS:
			{
				decTTL(totalMsg);
				uint8_t ttl;
				memcpy(&ttl,&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
				//cout<<"TLL is after decrementing is before sending  "<<(int )ttl<<endl;
										// checking for duplicate msg before inerting msg into the msg cache
				char *uoid=new char[20];
				memcpy(uoid,(char *)&totalMsg[1],20);
				if(checkMsgCache(uoid)==0)
				{
					// This is a new msg putting tht into the msg cache

					messageCache *msgInfo=new messageCache();
					msgInfo->msgType=JNRQ;
					msgInfo->uoid=new char[21];
					memset(msgInfo->uoid,'\0',21);
					memcpy(msgInfo->uoid,&totalMsg[1],20);
					msgInfo->lifeTime=msgLifeTime;
					msgInfo->fromSock=sockFd;
					msgInfo->peerPort=getWellKnownPort(sockFd);
					if(msgInfo->peerPort==0)
					{
							//cout<<"+++++++++++++++++++++++++++ TEMP CONNECTION JNRS "<<msgInfo->peerPort<<endl;
							memcpy(&msgInfo->peerPort,&totalMsg[31],sizeof(uint16_t));
							//cout<<"+++++++++++++++++++++++++++++ after debug "<<msgInfo->peerPort<<endl;
						}
					// pushing msg into the message cache vector
					pthread_mutex_lock(msgCacheLock);
					msgCacheVector.push_back(msgInfo);
					pthread_mutex_unlock(msgCacheLock);


					//logging
					logDetails *log=createLogEntry(totalMsg,sockFd,'r');
					pthread_mutex_lock(logEntryListLock);
					logEntryList.push_back(log);
					pthread_cond_signal(logEntryListEmptyCV);
					pthread_mutex_unlock(logEntryListLock);

					if(ttl>0)
					{
						int toSock=getRoute(totalMsg);
						if(toSock<0)
						{
							//cout<<"------------------------->Route not found"<<endl;
							break;
						}
						else
						{

							// routing the reply msg
							packet *packToSend=new packet();
							packToSend->msgToSend=totalMsg;
							packToSend->flood=0;
							packToSend->sockFd=toSock;
							pthread_mutex_lock(packetQLock);
							packetQ.push_back(packToSend);
							pthread_cond_signal(writerWaiting); // singalling the writer thread
							pthread_mutex_unlock(packetQLock);


						}
					}

				}
				break;
			}
			case KPAV:
			{
				//cout<<"Received a KEEP ALIVE msg"<<endl;
				// caching the KPAV msg 
				messageCache *msgInfo=new messageCache();
				msgInfo->msgType=KPAV;
				msgInfo->uoid=new char[21];
				memset(msgInfo->uoid,'\0',21);
				memcpy(msgInfo->uoid,&totalMsg[1],20);
				msgInfo->lifeTime=msgLifeTime;
				msgInfo->fromSock=sockFd;
				msgInfo->peerPort=getWellKnownPort(sockFd);

				// creating log entry and then pushing 
				logDetails *log=createLogEntry(totalMsg,sockFd,'r');
				pthread_mutex_lock(logEntryListLock);
				logEntryList.push_back(log);
				pthread_cond_signal(logEntryListEmptyCV);
				pthread_mutex_unlock(logEntryListLock);
				break;
			}
			case STRQ:
			{
				//cout<<"The Received Msg is status Request"<<endl;
				
				decTTL(totalMsg);
				uint8_t ttl;
				memcpy(&ttl,&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
				//cout<<"TLL is after decrementing is "<<(int )ttl<<endl;
				
				// checking for duplicate msg before inerting msg into the msg cache
				char *uoid=new char[20];
				memcpy(uoid,(char *)&totalMsg[1],20);
				if(checkMsgCache(uoid)==0) // checking the msg cache
				{
					// shud send a staus reply
					// recvd msg is not in cache

					// creating a msg cache and populating it
					messageCache *msgInfo=new messageCache();
					msgInfo->msgType=STRQ;
					msgInfo->uoid=new char[21];
					memset(msgInfo->uoid,'\0',21);
					memcpy(msgInfo->uoid,&totalMsg[1],20);
					msgInfo->lifeTime=msgLifeTime;
					msgInfo->fromSock=sockFd;
					msgInfo->peerPort=getWellKnownPort(sockFd);

					// pushing msg into the message cache vector
					pthread_mutex_lock(msgCacheLock);
					msgCacheVector.push_back(msgInfo);
					pthread_mutex_unlock(msgCacheLock);


					//logging
					logDetails *log=createLogEntry(totalMsg,sockFd,'r');
					pthread_mutex_lock(logEntryListLock);
					logEntryList.push_back(log);
					pthread_cond_signal(logEntryListEmptyCV);
					pthread_mutex_unlock(logEntryListLock);


					unsigned char *msg=createReplyMsg(STRS,totalMsg);

					//cout<<"Replying Status Msg"<<endl;
					packet *packToSend=new packet();
					packToSend->msgToSend=msg;
					packToSend->flood=0;
					packToSend->sockFd=sockFd;
					pthread_mutex_lock(packetQLock);
					packetQ.push_back(packToSend);
					pthread_cond_signal(writerWaiting); // singalling the writer thread
					pthread_mutex_unlock(packetQLock);
					//cout<<"\n replyin  packet and putting it into the common writer "<<endl;
					if(ttl>0)
					{
						//cout<<"\nFlooding status msg "<<endl;

						packet *packToSend=new packet();
						packToSend->msgToSend=totalMsg;
						packToSend->flood=1;
						packToSend->sockFd=sockFd;
						pthread_mutex_lock(packetQLock);
						packetQ.push_back(packToSend);
						pthread_cond_signal(writerWaiting); // singalling the writer thread
						pthread_mutex_unlock(packetQLock);


					}
					else
					{
						//cout<<"Msg was not flooded"<<endl;
					}
				}

				break;
			}
			case STRS:
			{

				//cout<<"Recvd a status Reply"<<endl;
				//cout<<"\n received Msg is status Response"<<endl;
				decTTL(totalMsg);
				uint8_t ttl;
				memcpy(&ttl,(int *)&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
				char *uoid=new char[20];
				memcpy(uoid,(char *)&totalMsg[1],20);
				if(checkMsgCache(uoid)==0) // checking the msg cache
				{
					// the msg is new and its not in msg cache
					// creating a msg cache and populating it
					messageCache *msgInfo=new messageCache();
					msgInfo->msgType=STRQ;
					msgInfo->uoid=new char[21];
					memset(msgInfo->uoid,'\0',21);
					memcpy(msgInfo->uoid,&totalMsg[1],20);
					msgInfo->lifeTime=msgLifeTime;
					msgInfo->fromSock=sockFd;
					msgInfo->peerPort=getWellKnownPort(sockFd);

					// pushing msg into the message cache vector
					pthread_mutex_lock(msgCacheLock);
					msgCacheVector.push_back(msgInfo);
					pthread_mutex_unlock(msgCacheLock);


					//logging
					logDetails *log=createLogEntry(totalMsg,sockFd,'r');
					pthread_mutex_lock(logEntryListLock);
					logEntryList.push_back(log);
					pthread_cond_signal(logEntryListEmptyCV);
					pthread_mutex_unlock(logEntryListLock);
					
					pthread_mutex_lock(statusLock);
					int statusSent=status;
					pthread_mutex_unlock(statusLock);
					
					char *tempStatusUoid=new char[21];
					memset(tempStatusUoid,'\0',NULL);
					memcpy(tempStatusUoid,&totalMsg[27],20);
					//cout<<"STATUS SENT= "<<statusSent<<endl;
					if(statusSent==1 && !strncmp(statusUoid,tempStatusUoid,20))
					{
						pthread_mutex_unlock(statusLock);
					
						// got the status response shud write to the nam vector
						pthread_mutex_lock(namVectorLock);
						uint16_t tempNode;
						uint16_t tempLen;
						memcpy(&tempLen,(uint16_t *)&totalMsg[47],sizeof(uint16_t));
						memcpy(&tempNode,(uint16_t *)&totalMsg[49],sizeof(uint16_t));
						namDetails *namRecord=new namDetails();
						namRecord->node=tempNode;
						cout<<"Reply from Nam node is "<<tempNode<<endl;
						//cout<<"---------------------------------------------->Temp Len "<<tempLen<<endl;
						int nextConNodePos=HSIZE+20+tempLen+sizeof(tempLen);
						if(nextConNodePos==totalSize)
						{
							cout<<"^^^^^^^^^^^^^^^^^^^ unconnected node not possible I cant beleive it"<<endl;
						}
						else
						{
							//cout<<"Creating a nam record "<<endl;
							uint32_t recordLen;
							memcpy(&recordLen,(uint32_t*)&totalMsg[nextConNodePos],sizeof(uint32_t));
							//cout<<"Pos Record Len is "<<nextConNodePos<<endl;
							uint16_t tempConPort;
							nextConNodePos=nextConNodePos+4;
							memcpy(&tempConPort,(uint16_t *)&totalMsg[nextConNodePos],sizeof(uint16_t));
							//cout<<"Pos Data "<<nextConNodePos<<endl;
							namRecord->connectedNodes.push_back(tempConPort);
							//cout<<"Nam connection "<<tempConPort<<endl;
							nextConNodePos=nextConNodePos+recordLen;
							while(nextConNodePos<totalSize)
							{
										//cout<<"Inseting connections "<<endl;
										memcpy(&recordLen,(uint32_t*)&totalMsg[nextConNodePos],sizeof(uint32_t));
										uint16_t tempConPort;
										nextConNodePos=nextConNodePos+4;
										memcpy(&tempConPort,(uint16_t *)&totalMsg[nextConNodePos],sizeof(uint16_t));
										namRecord->connectedNodes.push_back(tempConPort);
										//cout<<"Nam connection "<<tempConPort<<endl;
										nextConNodePos=nextConNodePos+recordLen;
								
							}	
						}
						namVector.push_back(namRecord);
						pthread_mutex_unlock(namVectorLock);
						printNamVector();

					}
					pthread_mutex_unlock(statusLock);



						if(ttl>0)
						{
							int toSock=getRoute(totalMsg); // Gettign the route
							if(toSock<0)
							{
								//cout<<"------------------------->Route not found"<<endl;
								break;
							}
							else
							{
								// routing the reply msg
								//cout<<"----------------------------->routing status reply msg"<<endl;
								packet *packToSend=new packet();
								packToSend->msgToSend=totalMsg;
								packToSend->flood=0;
								packToSend->sockFd=toSock;
								pthread_mutex_lock(packetQLock);
								packetQ.push_back(packToSend);
								pthread_cond_signal(writerWaiting); // singalling the writer thread
								pthread_mutex_unlock(packetQLock);
							}

						}







					
					
				}
				
				break;
			}
			case CKRQ:
			{
				
				decTTL(totalMsg);
				uint8_t ttl;
				memcpy(&ttl,&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
				//cout<<"TLL is after decrementing is "<<(int )ttl<<endl;
				
				// checking for duplicate msg before inerting msg into the msg cache
				char *uoid=new char[20];
				memcpy(uoid,(char *)&totalMsg[1],20);
				if(checkMsgCache(uoid)==0) // checking the msg cache
				{
					//cout<<"Recvd a CHECK MSG "<<endl;
					if(iAmBeacon)
					{
						//cout<<"Replyin a CHECK MSG "<<endl;
						// shud send check reply
						unsigned char *msg=createReplyMsg(CKRS,totalMsg);
						packet *packToSend=new packet();
						packToSend->msgToSend=msg;
						packToSend->flood=0;
						packToSend->sockFd=sockFd;
						pthread_mutex_lock(packetQLock);
						packetQ.push_back(packToSend);
						pthread_cond_signal(writerWaiting); // singalling the writer thread
						pthread_mutex_unlock(packetQLock);

					}
					else
					{
						if(ttl>0)
						{
							//cout<<"\nFlooding status msg "<<endl;

							packet *packToSend=new packet();
							packToSend->msgToSend=totalMsg;
							packToSend->flood=1;
							packToSend->sockFd=sockFd;
							pthread_mutex_lock(packetQLock);
							packetQ.push_back(packToSend);
							pthread_cond_signal(writerWaiting); // singalling the writer thread
							pthread_mutex_unlock(packetQLock);


						}
					}
				}
			}
			case CKRS:
			{
				decTTL(totalMsg);
				uint8_t ttl;
				memcpy(&ttl,(int *)&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
				char *uoid=new char[20];
				memcpy(uoid,(char *)&totalMsg[1],20);
				if(checkMsgCache(uoid)==0) // checking the msg cache
				{
					//cout<<"Recvd a check response"<<endl;

					messageCache *msgInfo=new messageCache();
					msgInfo->msgType=CKRS;
					msgInfo->uoid=new char[21];
					memset(msgInfo->uoid,'\0',21);
					memcpy(msgInfo->uoid,&totalMsg[1],20);
					msgInfo->lifeTime=msgLifeTime;
					msgInfo->fromSock=sockFd;
					msgInfo->peerPort=getWellKnownPort(sockFd);

					// pushing msg into the message cache vector
					pthread_mutex_lock(msgCacheLock);
					msgCacheVector.push_back(msgInfo);
					pthread_mutex_unlock(msgCacheLock);


					//logging
					logDetails *log=createLogEntry(totalMsg,sockFd,'r');
					pthread_mutex_lock(logEntryListLock);
					logEntryList.push_back(log);
					pthread_cond_signal(logEntryListEmptyCV);
					pthread_mutex_unlock(logEntryListLock);

					pthread_mutex_lock(checkLock);
					int checkSent=check;
					pthread_mutex_unlock(checkLock);
					if(checkSent==1)
					{
						pthread_mutex_lock(networkLock);
						networkOk=1;
						pthread_mutex_unlock(networkLock);
					}

				}

				if(ttl>0)
				{

					int toSock=getRoute(totalMsg); // Gettign the route
					if(toSock<0)
					{
						//cout<<"------------------------->Route not found"<<endl;
						break;
					}
					else
					{
						//cout<<"\n------------------------------> Routing status Response"<<endl;
						// routing the reply msg
						packet *packToSend=new packet();
						packToSend->msgToSend=totalMsg;
						packToSend->flood=0;
						packToSend->sockFd=toSock;
						pthread_mutex_lock(packetQLock);
						packetQ.push_back(packToSend);
						pthread_cond_signal(writerWaiting); // singalling the writer thread
						pthread_mutex_unlock(packetQLock);
					}

				}
				break;
			}
			case NTFY:
			{
				logDetails *log=createLogEntry(totalMsg,sockFd,'r');
				pthread_mutex_lock(logEntryListLock);
				logEntryList.push_back(log);
				pthread_cond_signal(logEntryListEmptyCV);
				pthread_mutex_unlock(logEntryListLock);	
				break;
			}
		}
		delete(data);
		delete(headerBuf);
	}while(type!=NTFY); // while msg type is != notify this loop shud run
	pthread_mutex_lock(conVectorLock);
	for(unsigned int i=0;i<conVector.size();i++)
	{
		conDetails *con=conVector[i];
		if(con->sockFd==sockFd)
		{
			conVector.erase(conVector.begin()+i);
		}
	}
	pthread_mutex_unlock(conVectorLock);
	/*
	int ok=checkNetwork();
	if(ok==-1)
	{
		// shud do soft restart
		pthread_mutex_lock(systemLock);
		restart=1;
		pthread_mutex_lock(threadRefLock);
		for(unsigned int i=0;i<threadRefs.size();i++)
		{
			if(threadRefs[i]!=pthread_self()) //  A THREAD CANNOT CANCEL ITSELF
			{
				pthread_cancel(threadRefs[i]);	// CANCELLING ALL OTHER THREADS
			}
		}
		pthread_mutex_unlock(threadRefLock);
		
		// Clearing the connection vector
		pthread_mutex_lock(conVectorLock);
		conVector.clear();
		pthread_mutex_unlock(conVectorLock);
		
		// Clearing the nam Vector
		pthread_mutex_lock(namVectorLock);
		namVector.clear();
		pthread_mutex_unlock(namVectorLock);

		// clearing the Join Vector;
		pthread_mutex_lock(joinVectorLock);
		joinVector.clear();
		pthread_mutex_unlock(joinVectorLock);

		// Clearing the nInfo Vector
		pthread_mutex_lock(nInfoVectorLock);
		nInfoVector.clear();
		pthread_mutex_unlock(nInfoVectorLock);
	
		// Clearing the msg cache
		pthread_mutex_lock(msgCacheLock);
		msgCacheVector.clear();
		pthread_mutex_unlock(msgCacheLock);

		neighborVector.clear();



		pthread_cond_signal(systemCV);
		pthread_mutex_unlock(systemLock);
		
		*/

	//}
 pthread_exit(NULL);
}

/*
This is the join timer thread that will signal the join thread come out of the recv block
Forked from the join function
*/
void *joinTimer(void *arg)
{
	int *dummyPtr=(int *)arg;
	int sockFd=*dummyPtr;
	int countDown=joinTimeOut;
	while(countDown)
	{
		pthread_testcancel();
		sleep(1);
		pthread_testcancel();
		countDown--;
	}
	//cout<<"join timeout "<<endl;
	close(sockFd);
	pthread_exit(NULL);
}

/*
This function is called to join a network and create an the init_neighhourlist file
return -1 if it cannot join the servant network 
*/
int Join(int port)
{
	//cout<<" Welknown port number of the server to which i must connect to is "<<port<<endl;
	vector<unsigned char*> joinReplys;
	int *cliSock;
	struct sockaddr_in *servAddr;
	struct hostent *serverIp;
	servAddr=new sockaddr_in();
	cliSock=new int();
	serverIp=gethostbyname("nunki.usc.edu");
	*cliSock=socket(AF_INET,SOCK_STREAM,0);
	while(*cliSock<0)
	{
		cout<<"Error : cannot create a socket for communication "<<endl;
		exit(1);
	}
	memset(servAddr,0,sizeof(struct sockaddr_in));
	servAddr->sin_family=AF_INET;
	memcpy(&(servAddr->sin_addr),serverIp->h_addr,serverIp->h_length);
	servAddr->sin_port=htons(port);

	// connecting to the peers well known port number 
	pthread_testcancel();
	if(connect(*cliSock,(struct sockaddr*)servAddr,sizeof(struct sockaddr_in))<0)
	{
		// connect error beacon may not be up
		pthread_testcancel();
		close(*cliSock);
		delete(servAddr);
		return -1;
	}
	else
	{
		// connection successfull

		// creating a join detail
		joinDetails *jd=new joinDetails();
		jd->peerPort=port;
		jd->sockFd=*cliSock;

		// pushing join detail into join vector
		pthread_mutex_lock(joinVectorLock);
		joinVector.push_back(jd);
		pthread_mutex_unlock(joinVectorLock);

		unsigned char *msg=createMsg(JNRQ,0);
		
		// Logging non  beacon JNRQ
		logDetails *log=createLogEntry(msg,*cliSock,'s');
		pthread_mutex_lock(logEntryListLock);
		logEntryList.push_back(log);
		pthread_cond_signal(logEntryListEmptyCV);
		pthread_mutex_unlock(logEntryListLock);

		// creating the packet
		uint32_t dataLen=0;
		memcpy(&dataLen,&msg[23],sizeof(uint32_t));
		for(unsigned int i=0;i<HSIZE+dataLen;i++)
		{
			pthread_testcancel();
			if(send(*cliSock,&msg[i],1,0)!=1)// sending the packet
			{
				pthread_testcancel();
				close(*cliSock);
				return -1;
			}
			pthread_testcancel();
			
		}
		pthread_t *joinTimerTh=new pthread_t();
		pthread_create(joinTimerTh,NULL,joinTimer,cliSock); // creating timer thread
		//pushing it into the threads vector
		pthread_mutex_lock(threadRefLock);
		threadRefs.push_back(*joinTimerTh);
		pthread_mutex_unlock(threadRefLock);
		uint8_t type;
		// goin to wait for the join replys
		//int minWait=minNeighbors;
		while(1)
		{
				char *headerBuf=new char[HSIZE];
				for(int i=0;i<HSIZE;i++)
				{
					pthread_testcancel();
					if(recv(*cliSock,&headerBuf[i],1,0)<0)// receiving header
					{
						pthread_testcancel();
						close(*cliSock);
						delete(headerBuf);
						//cout<<"\n breaking here "<<endl;
						goto outOfWhile;
					}
					pthread_testcancel();
				}
				
				uint32_t dataLen=0;
				memcpy(&dataLen,&headerBuf[23],sizeof(dataLen));
				
				memcpy(&type,(uint8_t *)&headerBuf[0],sizeof(uint8_t));
				//cout<<"Cli : Received msg len is "<<dataLen<<endl;
				
				char *data=new char[dataLen];
				for(unsigned int i=0;i<dataLen;i++)
				{
					pthread_testcancel();
					if(recv(*cliSock,&data[i],1,0)<0)// receiving data
					{
						pthread_testcancel();
						close(*cliSock);
						delete(data);
						goto outOfWhile;
					}
					pthread_testcancel();
				}
				memcpy(&type,(uint8_t *)&headerBuf[0],sizeof(uint8_t));

				// combining the header and the msg for this recvd msg
				unsigned char *totalMsg=new unsigned char[HSIZE+dataLen];
				for(unsigned int i=0,j=0;i<HSIZE+dataLen;i++)
				{
					if(i<HSIZE)
					{
						totalMsg[i]=headerBuf[i];
					}
					else
					{
						totalMsg[i]=data[j];
						j++;
					}
				}
				//joinReplyCount++;
				// Logging non  beacon JNRS
				logDetails *log=createLogEntry(totalMsg,*cliSock,'r');
				pthread_mutex_lock(logEntryListLock);
				logEntryList.push_back(log);
				pthread_cond_signal(logEntryListEmptyCV);
				pthread_mutex_unlock(logEntryListLock);

				//cout<<"\n --------------------> got a join reply "<<endl;
				//uint32_t tempDist;
				//memcpy(&tempDist,&totalMsg[47],sizeof(uint32_t));
				//cout<<"The Distance is "<<tempDist<<endl;
				joinReplys.push_back(totalMsg); // pushing the join replys into a vector
				delete(headerBuf);
				delete(data);

		}
	}

	outOfWhile:
		//cout<<"Out of while reached"<<endl;
	
	// got the join rpelys
	//cout<<"Join socket closed"<<endl;
	list<uint32_t> distList;
	list<uint32_t>::iterator it;
	
	if((int)joinReplys.size()<initNeighbors)
	{
		cout<<"Cannot join servant network, not enough init neighbors"<<endl;
		exit(1);
	}
	uint32_t tempDist;
	for(unsigned int i=0;i<joinReplys.size();i++)
	{
		
		unsigned char *msg=joinReplys[i];
		memcpy(&tempDist,&msg[47],sizeof(uint32_t));
		distList.push_back(tempDist); // pushing the distances in a list for sorting 
	}
						//	cout << "mylist contains before sorting :";
						//	for (it=distList.begin(); it!=distList.end(); ++it)
						//		cout << " " << *it;
						//	cout<<endl;

	distList.sort();	// sorting the list contents

						//	cout << "mylist contains after sorting :";
						//	for (it=distList.begin(); it!=distList.end(); ++it)
						//		cout << " " << *it;
						//	cout<<endl;
	
    //cout<<"init neigbours is "<<initNeighbors<<endl;
	// selecting the hsot names and port as per the sorted distance
	INLFile=fopen(inl,"w+"); // opening the init neigbor list file to write
	for(int i=0;i<initNeighbors;i++)
	{
		tempDist=distList.front();
		distList.pop_front();
		
		for(unsigned  int j=0;j<joinReplys.size();j++) 
		{
			unsigned char *msg=joinReplys[j];
			uint32_t distInMsg;
			memcpy(&distInMsg,&msg[47],sizeof(uint32_t));
			//cout<<"Dist in msg  is "<<distInMsg<<endl;
			//cout<<"tempDist is "<<tempDist<<endl;
			if(distInMsg==tempDist)
			{
				// distance matched 
				uint32_t msgLen;
				memcpy(&msgLen,&msg[23],sizeof(uint32_t));
				char *peerName=new char[(msgLen-(20+sizeof(uint32_t)+sizeof(uint16_t)))+1];
				memset(peerName,'\0',(msgLen-(20+sizeof(uint32_t)+sizeof(uint16_t)))+1);
				memcpy(peerName,&msg[53],(msgLen-(20+sizeof(uint32_t)+sizeof(uint16_t))));
				//cout<<"min peer name is "<<peerName<<endl;
				uint16_t peerPort;
				memcpy(&peerPort,&msg[51],sizeof(uint16_t));
				//cout<<"Min peer port is "<<peerPort<<endl;

				//wirting the init-neibnour_list file
				fprintf (INLFile,"%s:%d\n",peerName,peerPort);
				fflush(INLFile);

			}
			else
			{
				//cout<<"distance does not match "<<endl;
			}
		}
		
	}
	fclose(INLFile);

	// deleted unsed memory to avoid memorty leak 
	for(unsigned int i=0;i<joinReplys.size();i++)
	{
		
		unsigned char *msg=joinReplys[i];
		delete(msg);
	}
	
	//writing to ini file done 
	// joining done
	return 0;	
}



/*
This function is called to create client threads that is going to connect to peers.
Forked from the main function
*/
void *connectToPeer(void *arg)
{
	int myinit=1;
	int *dummyPtr=(int *) arg;
	int sPort=*dummyPtr;
	//delete(dummyPtr);
	//cout<<" Welknown port number of the server to which i must connect to is "<<sPort<<endl;
	int *cliSock;
	struct sockaddr_in *servAddr;
	struct hostent *serverIp;
	while(1)
	{

		servAddr=new sockaddr_in();
		cliSock=new int();
		serverIp=gethostbyname("nunki.usc.edu");
		*cliSock=socket(AF_INET,SOCK_STREAM,0);
		while(*cliSock<0)
		{
			pthread_testcancel();
			cout<<"Error : cannot create a socket for communication "<<endl;
			//sleep(10); // should change
			pthread_testcancel();
		}
		memset(servAddr,0,sizeof(struct sockaddr_in));
		servAddr->sin_family=AF_INET;
		memcpy(&(servAddr->sin_addr),serverIp->h_addr,serverIp->h_length);
		servAddr->sin_port=htons(sPort);
		
		pthread_testcancel();
		// connecting to the peers well known port number 
		if(connect(*cliSock,(struct sockaddr*)servAddr,sizeof(struct sockaddr_in))<0)
		{
			pthread_testcancel();
			//cout<<"Cli : Error : Connection failed.. Trying to Reconnect..."<<endl;
			if(iAmBeacon)
			{
				sleep(retry);
				pthread_testcancel();
				delete(cliSock);
				delete(servAddr);
				delete(serverIp);
				pthread_mutex_lock(conVectorLock);
				for(unsigned int i=0;i<conVector.size();i++)
				{
					conDetails *con;
					con=conVector[i];
					if(sPort==con->peerPort)
					{
						pthread_mutex_unlock(conVectorLock);
						//cout<<"-------------------------------------------->connection already there so exiting "<<endl;
						pthread_exit(NULL);
					}
				}
				pthread_mutex_unlock(conVectorLock);
				pthread_testcancel();
				continue;
			
			}
			else
			{
				pthread_mutex_lock(conCountLock);
				pthread_cond_signal(conCountCV);
				pthread_mutex_unlock(conCountLock);
				pthread_exit(NULL);
			}

		}
		pthread_mutex_lock(conCountLock);
		conCount++;
		pthread_cond_signal(conCountCV);
		pthread_mutex_unlock(conCountLock);
		break;
	}
	
	
					//updating nInfo Vector for logging
				nInfo *nei=new nInfo();
				nei->nodeId=new char[25];
				memset(nei->nodeId,'\0',25);
				sprintf(nei->nodeId,"nunki.usc.edu_%d",sPort);
				//cout<<"nei node ID is "<<nei->nodeId<<endl;
				nei->sockFd=*cliSock;
				
				pthread_mutex_lock(nInfoVectorLock);
				nInfoVector.push_back(nei);
				pthread_mutex_unlock(nInfoVectorLock);
	
		// creatin the hello msg
		unsigned char *msg=createMsg(HLLO,0);
		
							// creating log entry and then pushing 
		//logDetails *log=createLogEntry(msg,*cliSock,'s');
		//pthread_mutex_lock(logEntryListLock);
		//logEntryList.push_back(log);
		//pthread_cond_signal(logEntryListEmptyCV);
		//pthread_mutex_unlock(logEntryListLock);

		// creating the packet
		packet *packToSend=new packet();
		packToSend->msgToSend=msg;
		packToSend->flood=0;
		packToSend->sockFd=*cliSock;
		pthread_mutex_lock(packetQLock);
		packetQ.push_back(packToSend);
		pthread_cond_signal(writerWaiting); // singalling the writer thread
		pthread_mutex_unlock(packetQLock);
		uint8_t type;
		do
		{
					// waiting to recv the reply msg
				char *headerBuf=new char[HSIZE];
				for(int i=0;i<HSIZE;i++)
				{
					pthread_testcancel();
					if(recv(*cliSock,&headerBuf[i],1,0)!=1)// receiving header
					{
						pthread_testcancel();
						
						close(*cliSock);
						pthread_mutex_lock(joinVectorLock);
						for(unsigned int i=0;i<joinVector.size();i++)
						{
							joinDetails *jd=joinVector[i];
							if(jd->sockFd==*cliSock)
							{
								//cout<<"Removing from JoinVector"<<endl;
								joinVector.erase(joinVector.begin()+i);
							}
						}
						pthread_mutex_unlock(joinVectorLock);
						
						pthread_mutex_lock(conVectorLock);
						for(unsigned int i=0;i<conVector.size();i++)
						{
							conDetails *con=conVector[i];
							if(con->sockFd==*cliSock)
							{
							conVector.erase(conVector.begin()+i);
							}
						}
						pthread_mutex_unlock(conVectorLock);
						pthread_exit(NULL);
					}
					pthread_testcancel();
				}
				
				uint32_t dataLen=0;
				memcpy(&dataLen,&headerBuf[23],sizeof(dataLen));
				
				memcpy(&type,(uint8_t *)&headerBuf[0],sizeof(uint8_t));
				//cout<<"Cli : Received msg len is "<<dataLen<<endl;
				
				char *data=new char[dataLen];
				for(unsigned int i=0;i<dataLen;i++)
				{
					pthread_testcancel();
					if(recv(*cliSock,&data[i],1,0)!=1)// receiving data
					{
						pthread_testcancel();
						close(*cliSock);
						
						pthread_mutex_lock(joinVectorLock);
						for(unsigned int i=0;i<joinVector.size();i++)
						{
							joinDetails *jd=joinVector[i];
							if(jd->sockFd==*cliSock);
							{	
								//cout<<"Removing from JoinVector"<<endl;
								joinVector.erase(joinVector.begin()+i);
							}
						}
						pthread_mutex_unlock(joinVectorLock);
						
						pthread_mutex_lock(conVectorLock);
						for(unsigned int i=0;i<conVector.size();i++)
						{
							conDetails *con=conVector[i];
							if(con->sockFd==*cliSock);
							{
							conVector.erase(conVector.begin()+i);
							}
						}
						pthread_mutex_unlock(conVectorLock);
						
						pthread_exit(NULL);
					}
					pthread_testcancel();
				}
				memcpy(&type,(uint8_t *)&headerBuf[0],sizeof(uint8_t));

				// combining the header and the msg for this recvd msg
				unsigned char *totalMsg=new unsigned char[HSIZE+dataLen];
				for(unsigned int i=0,j=0;i<HSIZE+dataLen;i++)
				{
					if(i<HSIZE)
					{
						totalMsg[i]=headerBuf[i];
					}
					else
					{
						totalMsg[i]=data[j];
						j++;
					}
				}
				int totalSize=HSIZE+dataLen;
				setKeepAlive(*cliSock);
				switch(type)
				{
					case HLLO :
					{
							char *uoid=new char[20];
							memcpy(uoid,(char *)&totalMsg[1],20);
							if(checkMsgCache(uoid)==0) // Checking the Msg cache
							{
								// This Msg is new Msg
								uint16_t peerPort;
								memcpy(&peerPort,&data[0],sizeof(uint16_t));
			
								//updating connection vector
								pthread_mutex_lock(conVectorLock);
								conDetails *con=new conDetails();
								con->peerPort=peerPort;
								//con->peerName=peerName;
								con->sockFd=*cliSock;
								con->init=myinit;
								conVector.push_back(con);
								pthread_mutex_unlock(conVectorLock);

								// creating a msg cache and populating it
								messageCache *msgInfo=new messageCache();
								msgInfo->msgType=HLLO;
								msgInfo->uoid=new char[21];
								memset(msgInfo->uoid,'\0',21);
								memcpy(msgInfo->uoid,&totalMsg[1],20);
								msgInfo->lifeTime=msgLifeTime;
								msgInfo->fromSock=*cliSock;
								msgInfo->peerPort=getWellKnownPort(*cliSock);
								
								// pushing msg into the message cache vector
								pthread_mutex_lock(msgCacheLock);
								msgCacheVector.push_back(msgInfo);
								pthread_mutex_unlock(msgCacheLock);
								
								logDetails *log=createLogEntry(totalMsg,*cliSock,'r');
								pthread_mutex_lock(logEntryListLock);
								logEntryList.push_back(log);
								pthread_cond_signal(logEntryListEmptyCV);
								pthread_mutex_unlock(logEntryListLock);	
								
								//printing vector after the push
								printMsgCache();
							}
							delete(uoid);
							break;
					}
					case JNRQ :
					{
						//cout<<"The received msg is joinreq "<<endl;
						decTTL(totalMsg);
						uint8_t ttl;
						memcpy(&ttl,&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
						//cout<<"TLL is after decrementing is "<<(int )ttl<<endl;
						
						
						uint16_t joinPort;
						memcpy(&joinPort,&totalMsg[31],sizeof(uint16_t));
						cout<<"************JOIN PORT IS "<<(int)joinPort<<endl;
				
						// creating a join detail
						joinDetails *jd=new joinDetails();
						jd->peerPort=(int)joinPort;
						jd->sockFd=*cliSock;

						// pushing join detail into join vector
						pthread_mutex_lock(joinVectorLock);
						joinVector.push_back(jd);
						pthread_mutex_unlock(joinVectorLock);	
						
						printJoinDetails();
						
						// checking for duplicate msg before inerting msg into the msg cache
						char *uoid=new char[20];
						memcpy(uoid,(char *)&totalMsg[1],20);
						if(checkMsgCache(uoid)==0)
						{
							//should send a join reply
							// msg not in cache should insert into the msgcache
							//cout<<"------------------------------------------------------------------------------------------------->should send a join reply"<<endl;
							
							// creating a msg cache and populating it
							messageCache *msgInfo=new messageCache();
							msgInfo->msgType=JNRQ;
							msgInfo->uoid=new char[21];
							memset(msgInfo->uoid,'\0',21);
							memcpy(msgInfo->uoid,&totalMsg[1],20);
							msgInfo->lifeTime=msgLifeTime;
							msgInfo->fromSock=*cliSock;
							msgInfo->peerPort=getWellKnownPort(*cliSock);

							if(msgInfo->peerPort==0)
							{
								//cout<<"+++++++++++++++++++++++++++ TEMP CONNECTION JNRS "<<msgInfo->peerPort<<endl;
								memcpy(&msgInfo->peerPort,&totalMsg[31],sizeof(uint16_t));
								//cout<<"+++++++++++++++++++++++++++++ after debug "<<msgInfo->peerPort<<endl;
							}
						
							
							// pushing msg into the message cache vector
							pthread_mutex_lock(msgCacheLock);
							msgCacheVector.push_back(msgInfo);
							pthread_mutex_unlock(msgCacheLock);


							//logging
							logDetails *log=createLogEntry(totalMsg,*cliSock,'r');
							pthread_mutex_lock(logEntryListLock);
							logEntryList.push_back(log);
							pthread_cond_signal(logEntryListEmptyCV);
							pthread_mutex_unlock(logEntryListLock);
							
							//printing vector after the push
							printMsgCache();
							
							// creating a join reply msg

							
							unsigned char *msg=createReplyMsg(JNRS,totalMsg);
							packet *packToSend=new packet();
							packToSend->msgToSend=msg;
							packToSend->flood=0;
							packToSend->sockFd=*cliSock;
							pthread_mutex_lock(packetQLock);
							packetQ.push_back(packToSend);
							pthread_cond_signal(writerWaiting); // singalling the writer thread
							pthread_mutex_unlock(packetQLock);
							if(ttl>0)
							{
								// flooding the join req
								packet *packToSend=new packet();
								packToSend->msgToSend=totalMsg;
								packToSend->flood=1;
								packToSend->sockFd=*cliSock;
								pthread_mutex_lock(packetQLock);
								packetQ.push_back(packToSend);
								pthread_cond_signal(writerWaiting); // singalling the writer thread
								pthread_mutex_unlock(packetQLock);

							}
						}
						delete(uoid);


						break;
					}
					case JNRS :
					{
						//cout<<"The received msg is joinreq "<<endl;
						decTTL(totalMsg); // decrementing the ttl
						uint8_t ttl;
						memcpy(&ttl,&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
						//cout<<"TLL is after decrementing is "<<(int )ttl<<endl;
						
						// checking for duplicate msg before inerting msg into the msg cache
						char *uoid=new char[20];
						memcpy(uoid,(char *)&totalMsg[1],20);
						if(checkMsgCache(uoid)==0) // Checking the Msg Cache
						{

							//creating a msg cache
							messageCache *msgInfo=new messageCache();
							msgInfo->msgType=JNRQ;
							msgInfo->uoid=new char[21];
							memset(msgInfo->uoid,'\0',21);
							memcpy(msgInfo->uoid,&totalMsg[1],20);
							msgInfo->lifeTime=msgLifeTime;
							msgInfo->fromSock=*cliSock;
							msgInfo->peerPort=getWellKnownPort(*cliSock);

							//puttting msg into the msg cache
							pthread_mutex_lock(msgCacheLock);
							msgCacheVector.push_back(msgInfo);
							pthread_mutex_unlock(msgCacheLock);

							//logging
							logDetails *log=createLogEntry(totalMsg,*cliSock,'r');
							pthread_mutex_lock(logEntryListLock);
							logEntryList.push_back(log);
							pthread_cond_signal(logEntryListEmptyCV);
							pthread_mutex_unlock(logEntryListLock);


							if(ttl>0)
							{
								int toSock=getRoute(totalMsg); // Gettign the route
								if(toSock<0)
								{
									//cout<<"------------------------->Route not found"<<endl;
									break;
								}
								else
								{
									// routing the reply msg
									packet *packToSend=new packet();
									packToSend->msgToSend=totalMsg;
									packToSend->flood=0;
									packToSend->sockFd=toSock;
									pthread_mutex_lock(packetQLock);
									packetQ.push_back(packToSend);
									pthread_cond_signal(writerWaiting); // singalling the writer thread
									pthread_mutex_unlock(packetQLock);


								}
							}

						}
						break;

					}
					case KPAV:
					{
						//cout<<"Received a KEEP ALIVE msg"<<endl;
						// creating a msg cache
						messageCache *msgInfo=new messageCache();
						msgInfo->msgType=KPAV;
						msgInfo->uoid=new char[21];
						memset(msgInfo->uoid,'\0',21);
						memcpy(msgInfo->uoid,&totalMsg[1],20);
						msgInfo->lifeTime=msgLifeTime;
						msgInfo->fromSock=*cliSock;
						msgInfo->peerPort=getWellKnownPort(*cliSock);

						// creating log entry and then pushing 
						logDetails *log=createLogEntry(totalMsg,*cliSock,'r');
						pthread_mutex_lock(logEntryListLock);
						logEntryList.push_back(log);
						pthread_cond_signal(logEntryListEmptyCV);
						pthread_mutex_unlock(logEntryListLock);
						break;
					}
					case STRQ:
					{
						//cout<<"The Received Msg is status Request"<<endl;
						decTTL(totalMsg);
						uint8_t ttl;
						memcpy(&ttl,&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
						//cout<<"TLL is after decrementing is "<<(int )ttl<<endl;
						
						// checking for duplicate msg before inerting msg into the msg cache
						char *uoid=new char[20];
						memcpy(uoid,(char *)&totalMsg[1],20);
						if(checkMsgCache(uoid)==0) // checking the msg cache
						{
							// shud send a staus reply
							// recvd msg is not in cache

							// creating a msg cache and populating it
							messageCache *msgInfo=new messageCache();
							msgInfo->msgType=STRQ;
							msgInfo->uoid=new char[21];
							memset(msgInfo->uoid,'\0',21);
							memcpy(msgInfo->uoid,&totalMsg[1],20);
							msgInfo->lifeTime=msgLifeTime;
							msgInfo->fromSock=*cliSock;
							msgInfo->peerPort=getWellKnownPort(*cliSock);

							// pushing msg into the message cache vector
							pthread_mutex_lock(msgCacheLock);
							msgCacheVector.push_back(msgInfo);
							pthread_mutex_unlock(msgCacheLock);


							//logging
							logDetails *log=createLogEntry(totalMsg,*cliSock,'r');
							pthread_mutex_lock(logEntryListLock);
							logEntryList.push_back(log);
							pthread_cond_signal(logEntryListEmptyCV);
							pthread_mutex_unlock(logEntryListLock);


							unsigned char *msg=createReplyMsg(STRS,totalMsg);
							
							//cout<<"Replying status msg"<<endl;

							packet *packToSend=new packet();
							packToSend->msgToSend=msg;
							packToSend->flood=0;
							packToSend->sockFd=*cliSock;
							pthread_mutex_lock(packetQLock);
							packetQ.push_back(packToSend);
							pthread_cond_signal(writerWaiting); // singalling the writer thread
							pthread_mutex_unlock(packetQLock);
							//cout<<"\n replyin  packet and putting it into the common writer "<<endl;
							if(ttl>0)
							{
								//cout<<"\n flooing Status request packet "<<endl;

								packet *packToSend=new packet();
								packToSend->msgToSend=totalMsg;
								packToSend->flood=1;
								packToSend->sockFd=*cliSock;
								pthread_mutex_lock(packetQLock);
								packetQ.push_back(packToSend);
								pthread_cond_signal(writerWaiting); // singalling the writer thread
								pthread_mutex_unlock(packetQLock);


							}
						}


						break;
					}
					case STRS:
					{
						//cout<<"\n received Msg is status Response"<<endl;
						decTTL(totalMsg);
						uint8_t ttl;
						memcpy(&ttl,(int *)&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
						char *uoid=new char[20];
						memcpy(uoid,(char *)&totalMsg[1],20);
						if(checkMsgCache(uoid)==0) // checking the msg cache
						{
							// the msg is new and its not in msg cache
							// creating a msg cache and populating it
							messageCache *msgInfo=new messageCache();
							msgInfo->msgType=STRQ;
							msgInfo->uoid=new char[21];
							memset(msgInfo->uoid,'\0',21);
							memcpy(msgInfo->uoid,&totalMsg[1],20);
							msgInfo->lifeTime=msgLifeTime;
							msgInfo->fromSock=*cliSock;
							msgInfo->peerPort=getWellKnownPort(*cliSock);

							// pushing msg into the message cache vector
							pthread_mutex_lock(msgCacheLock);
							msgCacheVector.push_back(msgInfo);
							pthread_mutex_unlock(msgCacheLock);


							//logging
							logDetails *log=createLogEntry(totalMsg,*cliSock,'r');
							pthread_mutex_lock(logEntryListLock);
							logEntryList.push_back(log);
							pthread_cond_signal(logEntryListEmptyCV);
							pthread_mutex_unlock(logEntryListLock);
							
							pthread_mutex_lock(statusLock);
							int statusSent=status;
							//cout<<"STATUS SENT= "<<statusSent<<endl;
							char *tempStatusUoid=new char[21];
							memset(tempStatusUoid,'\0',NULL);
							memcpy(tempStatusUoid,&totalMsg[27],20);
							if(statusSent==1 && !strncmp(statusUoid,tempStatusUoid,20))
							{
								pthread_mutex_unlock(statusLock);
							
								// got the status response shud write to the nam vector
								pthread_mutex_lock(namVectorLock);
								uint16_t tempNode;
								uint16_t tempLen;
								memcpy(&tempLen,(uint16_t *)&totalMsg[47],sizeof(uint16_t));
								memcpy(&tempNode,(uint16_t *)&totalMsg[49],sizeof(uint16_t));
								namDetails *namRecord=new namDetails();
								namRecord->node=tempNode;
								//cout<<"Reply from Nam node is "<<tempNode<<endl;
								//cout<<"---------------------------------------------->Temp Len "<<tempLen<<endl;
								int nextConNodePos=HSIZE+20+tempLen+sizeof(tempLen);
								if(nextConNodePos==totalSize)
								{
									cout<<"^^^^^^^^^^^^^^^^^^^ unconnected node not possible I cant beleive it"<<endl;
								}
								else
								{
									//cout<<"Creating a nam record "<<endl;
									uint32_t recordLen;
									memcpy(&recordLen,(uint32_t*)&totalMsg[nextConNodePos],sizeof(uint32_t));
									//cout<<"Pos Record Len is "<<nextConNodePos<<endl;
									uint16_t tempConPort;
									nextConNodePos=nextConNodePos+4;
									memcpy(&tempConPort,(uint16_t *)&totalMsg[nextConNodePos],sizeof(uint16_t));
									//cout<<"Pos Data "<<nextConNodePos<<endl;
									namRecord->connectedNodes.push_back(tempConPort);
									//cout<<"Nam connection "<<tempConPort<<endl;
									nextConNodePos=nextConNodePos+recordLen;
									while(nextConNodePos<totalSize)
									{
										//cout<<"Inseting connections "<<endl;
										memcpy(&recordLen,(uint32_t*)&totalMsg[nextConNodePos],sizeof(uint32_t));
										uint16_t tempConPort;
										nextConNodePos=nextConNodePos+4;
										memcpy(&tempConPort,(uint16_t *)&totalMsg[nextConNodePos],sizeof(uint16_t));
										namRecord->connectedNodes.push_back(tempConPort);
										//cout<<"Nam connection "<<tempConPort<<endl;
										nextConNodePos=nextConNodePos+recordLen;
										
									}	
								}
								namVector.push_back(namRecord);
								pthread_mutex_unlock(namVectorLock);
								printNamVector();
							}
							pthread_mutex_unlock(statusLock);
								if(ttl>0)
								{

									int toSock=getRoute(totalMsg); // Gettign the route
									if(toSock<0)
									{
										//cout<<"------------------------->Route not found"<<endl;
										break;
									}
									else
									{
										//cout<<"\n------------------------------> Routing status Response"<<endl;
										// routing the reply msg
										packet *packToSend=new packet();
										packToSend->msgToSend=totalMsg;
										packToSend->flood=0;
										packToSend->sockFd=toSock;
										pthread_mutex_lock(packetQLock);
										packetQ.push_back(packToSend);
										pthread_cond_signal(writerWaiting); // singalling the writer thread
										pthread_mutex_unlock(packetQLock);


									}

								}
							
						}

						break;
					}
					case CKRQ:
					{
						
						decTTL(totalMsg);
						uint8_t ttl;
						memcpy(&ttl,&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
						//cout<<"TLL is after decrementing is "<<(int )ttl<<endl;
						
						// checking for duplicate msg before inerting msg into the msg cache
						char *uoid=new char[20];
						memcpy(uoid,(char *)&totalMsg[1],20);
						if(checkMsgCache(uoid)==0) // checking the msg cache
						{
							//cout<<"Recvd a Check Msg "<<endl;
							if(iAmBeacon)
							{
								//cout<<"Replying a Check Msg "<<endl;
								// shud send check reply
								unsigned char *msg=createReplyMsg(CKRS,totalMsg);
								packet *packToSend=new packet();
								packToSend->msgToSend=msg;
								packToSend->flood=0;
								packToSend->sockFd=*cliSock;
								pthread_mutex_lock(packetQLock);
								packetQ.push_back(packToSend);
								pthread_cond_signal(writerWaiting); // singalling the writer thread
								pthread_mutex_unlock(packetQLock);

							}
							else
							{
								if(ttl>0)
								{
									//cout<<"\nFlooding status msg "<<endl;

									packet *packToSend=new packet();
									packToSend->msgToSend=totalMsg;
									packToSend->flood=1;
									packToSend->sockFd=*cliSock;
									pthread_mutex_lock(packetQLock);
									packetQ.push_back(packToSend);
									pthread_cond_signal(writerWaiting); // singalling the writer thread
									pthread_mutex_unlock(packetQLock);


								}
							}
						}
					}
					case CKRS:
					{
						
						
						decTTL(totalMsg);
						uint8_t ttl;
						memcpy(&ttl,(int *)&(totalMsg[21]),sizeof(ttl)); // getting the ttl value
						char *uoid=new char[20];
						memcpy(uoid,(char *)&totalMsg[1],20);
						if(checkMsgCache(uoid)==0) // checking the msg cache
						{
							//cout<<"Recvd a check response "<<endl;
							pthread_mutex_lock(checkLock);
							int checkSent=check;
							pthread_mutex_unlock(checkLock);
							if(checkSent==1)
							{
								pthread_mutex_lock(networkLock);
								networkOk=1;
								pthread_mutex_unlock(networkLock);
							}

							messageCache *msgInfo=new messageCache();
							msgInfo->msgType=CKRS;
							msgInfo->uoid=new char[21];
							memset(msgInfo->uoid,'\0',21);
							memcpy(msgInfo->uoid,&totalMsg[1],20);
							msgInfo->lifeTime=msgLifeTime;
							msgInfo->fromSock=*cliSock;
							msgInfo->peerPort=getWellKnownPort(*cliSock);

							// pushing msg into the message cache vector
							pthread_mutex_lock(msgCacheLock);
							msgCacheVector.push_back(msgInfo);
							pthread_mutex_unlock(msgCacheLock);


							//logging
							logDetails *log=createLogEntry(totalMsg,*cliSock,'r');
							pthread_mutex_lock(logEntryListLock);
							logEntryList.push_back(log);
							pthread_cond_signal(logEntryListEmptyCV);
							pthread_mutex_unlock(logEntryListLock);

						}

						if(ttl>0)
						{

							int toSock=getRoute(totalMsg); // Gettign the route
							if(toSock<0)
							{
								//cout<<"------------------------->Route not found"<<endl;
								break;
							}
							else
							{
								//cout<<"\n------------------------------> Routing status Response"<<endl;
								// routing the reply msg
								packet *packToSend=new packet();
								packToSend->msgToSend=totalMsg;
								packToSend->flood=0;
								packToSend->sockFd=toSock;
								pthread_mutex_lock(packetQLock);
								packetQ.push_back(packToSend);
								pthread_cond_signal(writerWaiting); // singalling the writer thread
								pthread_mutex_unlock(packetQLock);
							}

						}
					break;
					}
					case NTFY:
					{
						logDetails *log=createLogEntry(totalMsg,*cliSock,'r');
						pthread_mutex_lock(logEntryListLock);
						logEntryList.push_back(log);
						pthread_cond_signal(logEntryListEmptyCV);
						pthread_mutex_unlock(logEntryListLock);	
						break;
					}
	
				}
				delete(headerBuf);
				delete(data);

		}
		while (type!=NTFY);
		pthread_mutex_lock(conVectorLock);
		for(unsigned int i=0;i<conVector.size();i++)
		{
			conDetails *con=conVector[i];
			if(con->sockFd==*cliSock)
			{
				conVector.erase(conVector.begin()+i);
			}
		}
		pthread_mutex_unlock(conVectorLock);
		/*
		int ok=checkNetwork();
		if(ok==-1)
		{
			// shud do soft restart 

			pthread_mutex_lock(systemLock);
			restart=1;
			pthread_mutex_lock(threadRefLock);
			for(unsigned int i=0;i<threadRefs.size();i++)
			{
				if(threadRefs[i]!=pthread_self()) //  A THREAD CANNOT CANCEL ITSELF
				{
					pthread_cancel(threadRefs[i]);	// CANCELLING ALL OTHER THREADS
				}
			}
			// Clearing the connection vector
			pthread_mutex_lock(conVectorLock);
			conVector.clear();
			pthread_mutex_unlock(conVectorLock);
			
			// Clearing the nam Vector
			pthread_mutex_lock(namVectorLock);
			namVector.clear();
			pthread_mutex_unlock(namVectorLock);

			// clearing the Join Vector;
			pthread_mutex_lock(joinVectorLock);
			joinVector.clear();
			pthread_mutex_unlock(joinVectorLock);

			// Clearing the nInfo Vector
			pthread_mutex_lock(nInfoVectorLock);
			nInfoVector.clear();
			pthread_mutex_unlock(nInfoVectorLock);
		
			// Clearing the msg cache
			pthread_mutex_lock(msgCacheLock);
			msgCacheVector.clear();
			pthread_mutex_unlock(msgCacheLock);

			neighborVector.clear();

			pthread_cond_signal(systemCV);
			pthread_mutex_unlock(systemLock);
			*/
		//}
	pthread_exit(NULL);
}

/*
Main server thread that is forked from main thread.
Listens on a well known port number
*/
void *listenThreadFn(void *arg)
{
	// CREATING A LISTEN SOCKET
	int reuseAddr=1;
	//listenSock=new int();
	//serv_addr=new sockaddr_in();
	listenSock=socket(AF_INET,SOCK_STREAM,0);
	if(listenSock<0)
	{
		pthread_testcancel();
		cout<<"Error: creating socket"<<endl;
		exit(1);
	}
	setsockopt(listenSock,SOL_SOCKET,SO_REUSEADDR,(void *)(&reuseAddr),sizeof(int)); // SETTING SOCKET OPTION TO RESUE PORT NUMBERS
    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    
    // BINDING A LISTEN SOCKET
    int rv=bind(listenSock, (struct sockaddr *)&serv_addr,sizeof (struct sockaddr_in) ) ;
    if( rv < 0 )
    {
    	//delete(listenSock);
    	//delete(serv_addr);
    	cout<<"\n Error Binding failed"<<endl;
		exit(1);
    }
    
    // LISTENING
    pthread_testcancel();
	listen(listenSock,CON_MAX);
	pthread_testcancel();
	
	socklen_t cliAddrLen;
    struct sockaddr_in cliAddr;
    cliAddrLen=sizeof(cliAddr);
    int *cliSock;
    
    while(1)
    {

		pthread_t *handleConThread=new pthread_t();     // WORKER THREAD IS CREATED FOR EACH ACCEPTED CONNECTION
		cliSock= new int();
		//cout<<"server waiting to accept connectoin"<<endl;
		pthread_testcancel();
        *cliSock=accept(listenSock,(struct sockaddr *)(&cliAddr),&cliAddrLen);
        if(*cliSock < 0)
        {
        	//cout<<"Deleting listen socket"<<endl;
        	shutdown(listenSock,SHUT_RDWR);
        	close(listenSock);
        	close(*cliSock);
			//cout<<"Error: Accepting connection "<<endl;
			pthread_exit(NULL);
        }
        pthread_testcancel();
        pthread_create(handleConThread,NULL,handleConFn,cliSock); // creating thread to handle incoming connections
        //pushing created thread into the thread vector
        pthread_mutex_lock(threadRefLock);
        threadRefs.push_back(*handleConThread); 
        pthread_mutex_unlock(threadRefLock);
        pthread_testcancel();
        
  }
	pthread_exit(NULL);
}


/*
Common timer thread which scrubs the time realted data.
Forked from main thread
*/

void *oneSecTimerFn(void *arg)
{
	while(1)
	{
		pthread_testcancel();
		sleep(1);
		pthread_testcancel();
		//cout<<"Timmer running "<<endl;
		// sould scrub all time related data

		// scrubing the msg cache
		pthread_mutex_lock(msgCacheLock);
		for(unsigned int i=0;i<msgCacheVector.size();i++)
		{
		
			messageCache *msgInfo=msgCacheVector[i];
			msgInfo->lifeTime=msgInfo->lifeTime-1;

			if(msgInfo->lifeTime<=0) // checking if the life time has reached zero if so decrementing it
			{
				//cout<<"Life time expired removing msg info from vector "<<endl;
				delete(msgInfo->uoid);
				msgCacheVector.erase(msgCacheVector.begin()+i);
			}
		}
		pthread_mutex_unlock(msgCacheLock);
		//printMsgCache();

		// decrementing the keep alive
		pthread_mutex_lock(conVectorLock);
		for(unsigned int i=0;i<conVector.size();i++)
		{
			conDetails *con=conVector[i];
			con->keepAlive=con->keepAlive-1;
			if(con->keepAlive==0)
			{
				// The peer is not alive anymore
				close(con->sockFd); // closing the connection
				conVector.erase(conVector.begin()+i);	// removing connection from connection vector

			}
		}
		pthread_mutex_unlock(conVectorLock);
	}
	pthread_exit(NULL);

}

/*
Keep Alive function that is forked from  main thread
*/
void systemSetup()
{

	setNodeId();	// setting the node id
	setNodeInstanceId();	// setting the node instace id
	
	conVectorLock=new pthread_mutex_t();	// a lock is created to control access to the convector
	pthread_mutex_init(conVectorLock,NULL);
	
	threadsListLock=new pthread_mutex_t();
	pthread_mutex_init(threadsListLock,NULL);

	packetQLock=new pthread_mutex_t();
	pthread_mutex_init(packetQLock,NULL);

	msgCacheLock=new pthread_mutex_t();
	pthread_mutex_init(msgCacheLock,NULL);

	writerWaiting=new pthread_cond_t();
	pthread_cond_init(writerWaiting,NULL);
	
	
	logEntryListLock=new pthread_mutex_t();		// A lock for it
	pthread_mutex_init(logEntryListLock,NULL);
	
	logEntryListEmptyCV=new pthread_cond_t();	// Cv on which the log writer thread will wait if the log entry list is empty
	pthread_cond_init(logEntryListEmptyCV,NULL);
	
	
	nInfoVectorLock=new pthread_mutex_t();
	pthread_mutex_init(nInfoVectorLock,NULL);

	statusLock=new pthread_mutex_t();
	pthread_mutex_init(statusLock,NULL);
	
	namVectorLock=new pthread_mutex_t();
	pthread_mutex_init(namVectorLock,NULL);

	printLock=new pthread_mutex_t();
	pthread_mutex_init(printLock,NULL);

	threadRefLock=new pthread_mutex_t();
	pthread_mutex_init(threadRefLock,NULL);

	joinVectorLock=new pthread_mutex_t();
	pthread_mutex_init(joinVectorLock,NULL);

	systemLock=new pthread_mutex_t();
	pthread_mutex_init(systemLock,NULL);
	
	systemCV=new pthread_cond_t();
	pthread_cond_init(systemCV,NULL);

	checkLock=new pthread_mutex_t();
	pthread_mutex_init(checkLock,NULL);

	networkLock=new pthread_mutex_t();
	pthread_mutex_init(networkLock,NULL);
	

	conCountLock=new pthread_mutex_t();
	pthread_mutex_init(conCountLock,NULL);
	
	conCountCV=new pthread_cond_t();
	pthread_cond_init(conCountCV,NULL);
}

void *keepAliveFn(void *arg)
{
	int keepAliveTime=keepAliveTimeOut/2;
	while(1)
	{
		//cout<<"----------------------------KeepAlive is "<<keepAliveTime<<endl;
		pthread_testcancel();
		if(keepAliveTime==0)
		{
			// shud create a keep alive msg and shud send to all peers
			//cout<<"-----------------------KeepAlive is "<<keepAliveTime<<endl;
			// creating a keep alive msg
			unsigned char *msg=createMsg(KPAV,0);
			packet *packToSend=new packet();
			packToSend->msgToSend=msg;
			packToSend->flood=1;
			packToSend->sockFd=0;
			pthread_mutex_lock(packetQLock);
			packetQ.push_back(packToSend);
			pthread_cond_signal(writerWaiting); // singalling the writer thread
			pthread_mutex_unlock(packetQLock);
			keepAliveTime=keepAliveTimeOut/2;
		}
		pthread_testcancel();
		sleep(1);
		pthread_testcancel();
		keepAliveTime--;
	}
	pthread_exit(NULL);
}

/*
This function is called when ever a staus command is typed in the command line 
interface
*/
void sendStatusRequest()
{
	//cout<<"Going to send status request "<<endl;
	unsigned char *msg=createMsg(STRQ,0);
	pthread_mutex_lock(statusLock);
	status=1;
	statusUoid=new char[21];
	memset(statusUoid,'\0',21);
	memcpy(statusUoid,&msg[1],20);
	pthread_mutex_unlock(statusLock);
	packet *packToSend=new packet();
	packToSend->msgToSend=msg;
	packToSend->flood=1;
	packToSend->sockFd=0;
	pthread_mutex_lock(packetQLock);
	packetQ.push_back(packToSend);
	pthread_cond_signal(writerWaiting); // singalling the writer thread
	pthread_mutex_unlock(packetQLock);
	int sleepTime=msgLifeTime;
	while(sleepTime)
	{
		sleep(1);
		sleepTime--;
	}
	pthread_mutex_lock(statusLock);
	status=0;
	delete(statusUoid);
	statusUoid=NULL;
	pthread_mutex_unlock(statusLock);
	//cout<<"\n status request time out"<<endl;
	pthread_mutex_lock(namVectorLock);
	statusFile=fopen(statusFilePath,"a+");
	fprintf(statusFile,"V -t * -v 1.0a5\n");
	//fprintf(statusFile,"n -t * -s %d -c red -i black\n",port);
	vector<uint16_t >nodes;
	nodes.push_back((uint16_t)port);
	for(unsigned int i=0;i<namVector.size();i++)
	{
		namDetails *nam=namVector[i];
		uint16_t nPort=nam->node;
		nodes.push_back(nPort);
	}
	for(unsigned int i=0;i<namVector.size();i++)
	{
		// Writing the nodes
		namDetails *nam=namVector[i];
		for(unsigned int j=0;j<nam->connectedNodes.size();j++)
		{
			uint16_t tempNode=nam->connectedNodes[j];
			int present=0;
			for(unsigned int k=0;k<nodes.size();k++)
			{
				if(nodes[k]==tempNode)
				{
					present=1;
					break;
				}	
			}
			if(present==0)
			{
				nodes.push_back(tempNode);
			}
		}
		
		
	}
	/*
	for(unsigned int i=0;i<nodes.size();i++)
	{
		cout<<">>>>"<<nodes[i]<<endl;
	}
	for(unsigned int i=0;i<nodes.size();i++)
	{
		for(unsigned int j=0;j<nodes.size();j++)
		{
			if(nodes[i]==nodes[j]&&i!=j&&j>i)
			{
				nodes.erase(nodes.begin()+j);
			}
		}
	}*/
	//cout<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<endl;
	for(unsigned int i=0;i<nodes.size();i++)
	{
		//cout<<">>>>"<<nodes[i]<<endl;
	}
	
	for(unsigned int i=0;i<nodes.size();i++)
	{
		// Writing the nodes
		//namDetails *nam=namVector[i];
		//uint16_t nPort=nam->node;
		fprintf(statusFile,"n -t * -s %d -c red -i black\n",nodes[i]);	
		
	}
	for(unsigned int i=0;i<namVector.size();i++)
	{
		// Writing the nodes
		namDetails *nam=namVector[i];
		uint16_t nPort=nam->node;
		for(unsigned int j=0;j<nam->connectedNodes.size();j++)
		{
			fprintf(statusFile,"l -t * -s %d -d %d -c blue\n",nPort,nam->connectedNodes[j]);
		}
		
	}
	fclose(statusFile);
	pthread_mutex_unlock(namVectorLock);
}

/*
This function is called to shut down the node.
invoked when user enters shutdown at command line
or when auto shutdown timer expires
*/
void shutDown()
{
	unsigned char *msg=createMsg(NTFY,1);
	packet *packToSend=new packet();
	packToSend->msgToSend=msg;
	packToSend->flood=1;
	packToSend->sockFd=0;
	pthread_mutex_lock(packetQLock);
	packetQ.push_back(packToSend);
	pthread_cond_signal(writerWaiting); // singalling the writer thread
	pthread_mutex_unlock(packetQLock);
	sleep(1); // Just to make sure tht the notify msg goes
	pthread_mutex_lock(threadRefLock);
	for(int i=0;i<(int)threadRefs.size();i++)
	{
		if(threadRefs[i]!=pthread_self()) //  A THREAD CANNOT CANCEL ITSELF
		{
			pthread_cancel(threadRefs[i]);	// CANCELLING ALL OTHER THREADS
		}
	}
	pthread_mutex_unlock(threadRefLock);
	pthread_mutex_lock(systemLock);
	toExit=1;
	pthread_cond_signal(systemCV);
	pthread_mutex_unlock(systemLock);
}

/*

user input funcion forked from main thread
*/
void *getUserInput(void *arg)
{
	string usrInput;
	int shutdown=1;
	uint8_t type=0;
	//uint8_t tempTTL;
	while(shutdown)
	{
		pthread_testcancel();
		cout<<"servant:"<<port<<">";
		fflush(stdin);
		//cin>>usrInput;
		pthread_testcancel();
		getline (cin,usrInput);
		pthread_testcancel();
		//cout<<"The user iput entered is "<<usrInput<<endl;
		//cout<<"\n len of user input is "<<strlen(usrInput.c_str())<<endl;
		if(strlen(usrInput.c_str())==0)
		{
			continue;
		}
		char *line=new char[strlen(usrInput.c_str())+1];
		memset(line,'\0',strlen(usrInput.c_str())+1);
		memcpy(line,usrInput.c_str(),strlen(usrInput.c_str()));
		char *values=strtok(line," ");
		if (!strncmp(values,"shutdown",8))
		{
			//cout<<"the user input is shutdown"<<endl;
			shutDown();
			pthread_exit(NULL);
			//exit(1); // shud remove
		}
		else if(!strncmp(values,"status",6))
		{
			
			//cout<<"the user command is status"<<endl;
			values = strtok (NULL, " ");
			if(!strncmp(values,"neighbors",9))
			{
				type=STN;
				//cout<<"the stauts type is "<<values<<endl;
			}
			values = strtok (NULL, " ");
			char *end;
			statusTTL=(uint8_t)strtol (values,&end,10);
			if(strcmp(end,""))
			{
				//cout<<"Error in iniFile"<<endl;
				exit(1);
			}
			//cout<<"TTL is "<<(int )statusTTL<<endl;
			values = strtok (NULL, " ");
			//cout<<"values is "<<values<<endl;
			statusFilePath=new char[strlen(values)+1];
			memset(statusFilePath,'\0',strlen(values)+1);
			memcpy(statusFilePath,values,strlen(values));
			statusFile=fopen(statusFilePath,"a");
			fclose(statusFile);
			if((int)statusTTL==0)
			{
				statusFile=fopen(statusFilePath,"a+");
				fprintf(statusFile,"V -t * -v 1.0a5\n");
				fprintf(statusFile,"n -t * -s %d -c red -i black\n",port);
				pthread_mutex_lock(conVectorLock);
				for(unsigned int i=0;i<conVector.size();i++)
				{
					conDetails *con=conVector[i];
					fprintf(statusFile,"n -t * -s %d -c red -i black\n",(int )con->peerPort);
				}
				for(unsigned int i=0;i<conVector.size();i++)
				{
					conDetails *con=conVector[i];
					fprintf(statusFile,"l -t * -s %d -d %d -c blue\n",port,(int )con->peerPort);
				}
				pthread_mutex_unlock(conVectorLock);
				fclose(statusFile);
			}
			else
			{
				// must call a send status function
				sendStatusRequest();
			}
		}
		else
		{
			cout<<usrInput<<": Command not found."<<endl;
		}
	}
	pthread_exit(NULL);
}

/*
Signal Handler function forked from main thread
*/

void *signalHandlerFn(void *arg)
{
	
	//cout<<"Signal Hander"<<endl;
	int prompt=0;
    int recvSignal;
	
    for(;;)
     {
     	pthread_testcancel();
        sigwait(&signals,&recvSignal);
        pthread_testcancel();
        if(recvSignal == SIGPIPE)
        {
            //cout<<"Connection interuppted with client. Broken pipe handled"<<endl;
        }
        else if( recvSignal == SIGINT)
        {
            //terminateServer();
			//pthread_exit(NULL);
			//cout<<"Termination caught"<<endl;
			// cout<<" Ctrl+c pressed by the user "<<endl;
			if(prompt==0)
			{
				prompt=1;
				//pthread_t *userInputThread=new pthread_t();
				//pthread_create(userInputThread,NULL,getUserInput,NULL);
			}
		 //pthread_exit(NULL);
        }
      }

}



/*
Auto shutdown timer thread froked from the main function
*/
void *autoShutDownFn(void *arg)
{
	int autoShut=autoShutdown;
	while(autoShut)
	{
		pthread_testcancel();
		sleep(1);
		pthread_testcancel();
		autoShut--;
		//cout<<"Auto shutdown is "<<autoShut<<endl;
	}
	shutDown();
	pthread_exit(NULL);
}

/*
Main function program execution begins here
*/
int main(int argc,char *argv[])
{
	parseCmdLine(argc,argv);	// Parsing the command line arguements

	sigemptyset (&signals);			// initializing the signal set
    sigaddset (&signals, SIGINT);	// adding singal to the signal set
    sigaddset (&signals, SIGPIPE);	// adding the signal to the signal set
	pthread_sigmask(SIG_BLOCK, &signals, NULL); // blocking this main thread from singals

	systemSetup();				// setting up the system 

	listenThread=new pthread_t();							// a thread pointer is created to fork the listen function
	pthread_create(listenThread,NULL,listenThreadFn,NULL); // creating a server thread for this node
	// Pushing thead into threadRef
	pthread_mutex_lock(threadRefLock);
	threadRefs.push_back(*listenThread);
	pthread_mutex_unlock(threadRefLock);

	
	commonWriterThread=new pthread_t();
	pthread_create(commonWriterThread,NULL,commonWriterFn,NULL); // creating a common writer thread
	// Pushing thead into threadRef
	pthread_mutex_lock(threadRefLock);
	threadRefs.push_back(*commonWriterThread);
	pthread_mutex_unlock(threadRefLock);

	oneSecTimerTh=new pthread_t();
	pthread_create(oneSecTimerTh,NULL,oneSecTimerFn,NULL); // creating tehe general timer thread
	// Pushing thead into threadRef
	pthread_mutex_lock(threadRefLock);
	threadRefs.push_back(*oneSecTimerTh);
	pthread_mutex_unlock(threadRefLock);

	keepAliveTh=new pthread_t();
	pthread_create(keepAliveTh,NULL,keepAliveFn,NULL); // creating a keep alive th
	
	pthread_mutex_lock(threadRefLock);
	threadRefs.push_back(*keepAliveTh);
	pthread_mutex_unlock(threadRefLock);
	

	pthread_t *userInputThread=new pthread_t();
	pthread_create(userInputThread,NULL,getUserInput,NULL);
	// Pushing thead into threadRef
	pthread_mutex_lock(threadRefLock);
	threadRefs.push_back(*userInputThread);
	pthread_mutex_unlock(threadRefLock);


	pthread_t *singnalHandlerTh=new pthread_t();
	pthread_create(singnalHandlerTh,NULL,signalHandlerFn,NULL);
	// Pushing thead into threadRef
	pthread_mutex_lock(threadRefLock);
	threadRefs.push_back(*singnalHandlerTh);
	pthread_mutex_unlock(threadRefLock);

	
	//cout<<"Creating home dir "<<endl;
	if(mkdir(homeDir,S_IRWXU)==-1)
	{
		 //std::cerr << "Error: " << strerror(errno);
	}

	//cout<<"creating log file"<<endl;
	logFilePath=new char[strlen(homeDir)+20];
	memset(logFilePath,'\0',strlen(homeDir)+20);
	memcpy(logFilePath,homeDir,strlen(homeDir));
	logFilePath[strlen(homeDir)]='/';
	memcpy(&logFilePath[strlen(homeDir)+1],logFileName.c_str(),11);

	string temp="init_neighbor_list";
	inl=new char[strlen(homeDir)+20];
	memset(inl,'\0',strlen(homeDir)+20);
	memcpy(inl,homeDir,strlen(homeDir));
	inl[strlen(homeDir)]='/';
	memcpy(&inl[strlen(homeDir)+1],temp.c_str(),18);

	if(reset) // checking if reset option is given
	{
		if(remove(inl) == -1) // removing the inl file
		{
				//fprintf(stderr,"Remove failed");
		}
		if(remove(logFilePath)==-1) // removeing the servant log file
		{
			//cout<<"Remove failed "<<endl;
		}
	}
	
	
	logFile=fopen(logFilePath,"a");
	fclose(logFile);
	
	logWritterTh=new pthread_t();
	pthread_create(logWritterTh,NULL,logWritter,NULL);
	// Pushing thead into threadRef
	pthread_mutex_lock(threadRefLock);
	threadRefs.push_back(*logWritterTh);
	pthread_mutex_unlock(threadRefLock);


	
	if(iAmBeacon)
	{
		// I am a beacon
		int *bport;
		for(int i=0;i</*bcount*/(int)beaconPorts.size();i++)
		{
			bport=new int();
			pthread_t *cliThread=new pthread_t();
//			*bport=beaconPorts.front();
			*bport=beaconPorts[i];
			//beaconPorts.pop_front();
			if(*bport!=port)
			{
				pthread_create(cliThread,NULL,connectToPeer,bport);
				// Pushing thead into threadRef
				pthread_mutex_lock(threadRefLock);
				threadRefs.push_back(*cliThread);
				pthread_mutex_unlock(threadRefLock);
			}
			//delete(bport);
		}
	}
	else // I am not a beacon
	{
		
		struct stat homeDirStat;
		int homeDirExist;
		homeDirExist = stat(homeDir,&homeDirStat);
		struct stat initFileStat;
		int initFileExist;
		initFileExist = stat(inl,&initFileStat); 
		homeDirExist = stat(homeDir,&homeDirStat);
		initFileExist = stat(inl,&initFileStat); 
		if(homeDirExist==0)
		{
			    // We were able to get the dir attributes
				// so the dir obviously exists.
				// now should check if the file exsist

				if(initFileExist==0)
				{
					alreadyInNetwork=1;
				}
				else
				{
					//cout<<"Init neighbour list does not exist I muts join the network "<<endl;
					//cout<<"inl is "<<inl<<endl;
					INLFile=fopen(inl,"w");
					fclose(INLFile);

				}

		}
		else
		{
			// homedir not there
			// shuold create a dir
			//cout<<"home dir does not exist must create a home dir "<<endl;
			if(mkdir(homeDir,S_IRWXU)==-1)
			{
			 std::cerr << "Error: " << strerror(errno);
			}
			//cout<<"Init neighbour list does not exist I muts join the network "<<endl;
			//cout<<"inl is "<<inl<<endl;
			INLFile=fopen(inl,"w");
			fclose(INLFile);
		}
		if(alreadyInNetwork)
		{
			// already in network can directly send a hello msg
			// after parsing the init neighbor list file
			//cout<<"Init neighbot List file exist so i am participating in the network"<<endl;
			goto takePart;
		}
		else
		{
				if(remove(inl) == -1) // removing the inl file
				{
					//fprintf(stderr,"Remove failed");
				}
				INLFile=fopen(inl,"w");
				fclose(INLFile);
				
				int joined=0; // to indicated whether I joined the network or not
				int *bport;
				for(int i=0;i<(int)beaconPorts.size();i++)
				{
					if(joined==1)
					{
						break;
					}
					bport=new int();
					//*bport=beaconPorts.front();
					*bport=beaconPorts[i];
					//beaconPorts.pop_front();
					if(Join(*bport)<0)
					{
						continue;
					}
					joined=1;
				}
				if(joined==0)
				{
					//cout<<"none of my beacon are alive to exiting "<<endl;
					exit(0);
				}
		}
		// joined successfully

		// parsing the inl file
		takePart:
		parseINLFile();
		int *nPort;
		for(unsigned int i=0;i<neighborVector.size();i++)
		{
			nPort=new int();
			pthread_t *cliThread=new pthread_t();
			neighborDetails *neighbor=neighborVector[i];
			*nPort=neighbor->nPort;
			pthread_create(cliThread,NULL,connectToPeer,nPort);
			// Pushing thead into threadRef
			pthread_mutex_lock(threadRefLock);
			threadRefs.push_back(*cliThread);
			pthread_mutex_unlock(threadRefLock);

		}
		int signalCount=0;
		pthread_mutex_lock(conCountLock);
		while(signalCount<minNeighbors)
		{
			pthread_cond_wait(conCountCV,conCountLock);
			signalCount++;
			
		}
		pthread_mutex_unlock(conCountLock);
		if(conCount<minNeighbors)
		{
				//cout<<"\n Min neighbors not present "<<endl;
				// Should join the network again
				//parseIniFile(iniFilename);
			
				int joined=0; // to indicated whether I joined the network or not
				int *bport;
				for(int i=0;i</*bcount*/(int)beaconPorts.size();i++)
				{
					if(joined==1)
					{
						break;
					}
					bport=new int();
					//*bport=beaconPorts.front();
					*bport=beaconPorts[i];
					//beaconPorts.pop_front();
					if(Join(*bport)<0)
					{
						continue;
					}
					joined=1;
				}
				if(joined==0)
				{
					//cout<<"none of my beacon are alive to exiting "<<endl;
					exit(0);
				}
				
				parseINLFile();
				int *nPort;
				for(unsigned int i=0;i<neighborVector.size();i++)
				{
					nPort=new int();
					pthread_t *cliThread=new pthread_t();
					neighborDetails *neighbor=neighborVector[i];
					*nPort=neighbor->nPort;
					pthread_create(cliThread,NULL,connectToPeer,nPort);
					// Pushing thead into threadRef
					pthread_mutex_lock(threadRefLock);
					threadRefs.push_back(*cliThread);
					pthread_mutex_unlock(threadRefLock);

				}	
		}
	}

	pthread_t *shutDownTh=new pthread_t();
	pthread_create(shutDownTh,NULL,autoShutDownFn,NULL);
	pthread_mutex_lock(threadRefLock);
	threadRefs.push_back(*shutDownTh);
	pthread_mutex_unlock(threadRefLock);

	pthread_mutex_lock(systemLock);
	pthread_mutex_unlock(systemLock);
	//pthread_exit(NULL);
	for(unsigned int i=0;i<threadRefs.size();i++)
	{
		pthread_join(threadRefs[i], NULL);
	}
	//cout<<"Main thread Exiting "<<endl;
	//cout<<endl;
}
