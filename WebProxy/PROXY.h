#pragma once

#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Stream.h"
#include "ace/synch.h"
#include "ace/Log_Msg.h"
#include "ace/streams.h"

#include <list>

#define BUFSIZE 1024
#define HTTP  "http://"
//#define FTP   "ftp://"

class CPROXY
{
public:
			
	CPROXY(void);
	~CPROXY(void);

	int Run(int nPort);

	static DWORD WINAPI MotherThread(LPVOID param);
	static DWORD WINAPI DaughterThread(LPVOID param);
	int MotherThreadWorker();
	int DaughterThreadWorker();

	int GetAddressAndPort(char * cStr, char *cAddress, int * nPort);


private:

	ACE_SOCK_Acceptor					client_acceptor;
	ACE_SOCK_Connector					server_connector;

	CRITICAL_SECTION					guard;
	HANDLE								wait;

	bool								isMotherThreadRunning;
	bool								isDaughterThreadRunning;

	static std::list<ACE_SOCK_Stream>	queue;
	static ACE_OSTREAM_TYPE				*output;
};
