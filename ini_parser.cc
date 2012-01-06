#include "extern.h"

const char *TrimBlanks(char *buf)
{
	string buffer(buf);
	remove(buffer.begin(),buffer.end(),' ');
	//printf("this trimmed string ---- %s\n",buffer.c_str());
	return buffer.c_str();
}

char *line;
int init=0;
int beacon=0;

void parseIniFile(char *fileName)
{
	line=new char[512];
	//cout<<"File name is "<<fileName<<endl;
	fstream iniFile(fileName,ios::in);
	while(!iniFile.eof())
	{
		memset(line,'\0',512);
		iniFile.getline(line,512);
		//cout<<"\n the line is :"<<line<<endl;
		if(!strncmp(line,"\0",1))
		{
			//cout<<"line is empty"<<endl;
			continue;
		}
		if(beacon==1)
		{
			char *values=strtok(line,"=");
			if (!strncmp(values,"Retry",5))
			{
				values = strtok (NULL, "=");
				char *end;
				retry=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The retry is :"<<retry<<endl;
			}
			else
			{
				values = strtok (line, ":");
				values = strtok (NULL, ":");
				char *end;
				int bport=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"beacon port is "<<bport<<endl;
				beaconPorts.push_back(bport);	
				bcount++;
				if(bport==port)
				{
					iAmBeacon=1;
					//cout<<"I am a beacon"<<endl;
				}
				
			}
			
		}
		char *values=strtok(line,"=");
		while(values!=NULL)
		{
			//cout<<" Values "<<values<<endl;
			if(!strncmp(line,"[init]",6))
			{
				//cout<<"Init section"<<endl;
				init=1;
				break;
			}
			if(!strncmp(line,"[beacons]",9))
			{
				//cout<<"Beacon section"<<endl;
				beacon=1;
				break;
			}
			if (!strncmp(values,"Port",4))
			{
				values = strtok (NULL, "=");
				char *end;
				port=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The port number is :"<<port<<endl;
			}
			if (!strncmp(values,"Location",8))
			{
				values = strtok (NULL, "=");
				char *end;
				location=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The location is :"<<location<<endl;
			}
			if (!strncmp(values,"AutoShutdown",12))
			{
				values = strtok (NULL, "=");
				char *end;
				autoShutdown=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The autoShutdown is :"<<autoShutdown<<endl;
			}
			if (!strncmp(values,"CacheSize",9))
			{
				values = strtok (NULL, "=");
				char *end;
				cacheSize=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The cacheSize is :"<<cacheSize<<endl;
			}
			if (!strncmp(values,"NeighborStoreProb",17))
			{
				values = strtok (NULL, "=");
				char *end;
				neighborStoreProb=(int)strtod (values,&end);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The neighborStoreProb is :"<<neighborStoreProb<<endl;
			}
			if (!strncmp(values,"StoreProb",9))
			{
				values = strtok (NULL, "=");
				char *end;
				storeProb=(int)strtod (values,&end);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The storeProb is :"<<storeProb<<endl;
			}
			if (!strncmp(values,"CacheProb",9))
			{
				values = strtok (NULL, "=");
				char *end;
				cacheProb=(int)strtod (values,&end);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The cacheProb is :"<<cacheProb<<endl;
			}

			if (!strncmp(values,"NoCheck",7))
			{
				values = strtok (NULL, "=");
				char *end;
				noCheck=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The noCheck is :"<<noCheck<<endl;
			}
			if (!strncmp(values,"MinNeighbors",12))
			{
				values = strtok (NULL, "=");
				char *end;
				minNeighbors=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The minNeighbors is :"<<minNeighbors<<endl;
			}
			if (!strncmp(values,"KeepAliveTimeout",16))
			{
				values = strtok (NULL, "=");
				char *end;
				keepAliveTimeOut=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++The keepAliveTimeOut is :"<<keepAliveTimeOut<<endl;
			}
			if (!strncmp(values,"JoinTimeOut",11))
			{
				values = strtok (NULL, "=");
				char *end;
				joinTimeOut=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The joinTimeOut is :"<<joinTimeOut<<endl;
			}
			if (!strncmp(values,"MsgLifeTime",11))
			{
				values = strtok (NULL, "=");
				char *end;
				msgLifeTime=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The msgLifeTime is :"<<msgLifeTime<<endl;
			}
			if (!strncmp(values,"TTL",3))
			{
				values = strtok (NULL, "=");
				char *end;
				ttl=(uint8_t)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The ttl is :"<<ttl<<endl;
			}
			if (!strncmp(values,"GetMsgLifeTime",14))
			{
				values = strtok (NULL, "=");
				char *end;
				getMsgLifeTime=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The getMsgLifeTime is :"<<getMsgLifeTime<<endl;
			}
			if (!strncmp(values,"InitNeighbors",13))
			{
				values = strtok (NULL, "=");
				char *end;
				initNeighbors=(int)strtol (values,&end,10);
				if(strcmp(end,""))
				{
					cout<<"Error in iniFile"<<endl;
					exit(1);
				}
				//cout<<"The initNeighbors is :"<<initNeighbors<<endl;
			}
			if (!strncmp(values,"HomeDir",7))
			{
				values = strtok (NULL, "=");
				homeDir=values;
				//cout<<"The homeDir is :"<<homeDir<<endl;
				//cout<<"StrLen of home Dir is "<<strlen(homeDir)<<endl;
				char *tmpDir=new char[strlen(homeDir)+1];
				memset(tmpDir,'\0',strlen(homeDir)+1);
				memcpy(tmpDir,homeDir,strlen(homeDir));
				homeDir=tmpDir;
				//cout<<"The homeDir is :"<<homeDir<<endl;
				//cout<<"StrLen of home Dir is "<<strlen(homeDir)<<endl;
			}

			values = strtok (NULL, "=");

		}
	}
	
	//cout<<"Total count of beacons is "<<bcount<<endl;
	
}

