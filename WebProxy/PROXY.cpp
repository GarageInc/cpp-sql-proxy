#include "PROXY.h"

ACE_OSTREAM_TYPE *CPROXY::output;
std::list<ACE_SOCK_Stream> CPROXY::queue;


CPROXY::CPROXY(void)
{
	InitializeCriticalSection(&guard);
	wait = CreateEvent(NULL, FALSE, FALSE, NULL);

	output = new std::ofstream("webproxy.log");
}

CPROXY::~CPROXY(void)
{
	ACE_LOG_MSG->clr_flags(ACE_Log_Msg::OSTREAM);
	delete output;
}

int CPROXY::Run(int nPort)
{
	ACE_DEBUG((LM_DEBUG, ACE_TEXT("%t: Running!\n")));

	DWORD thid;
	HANDLE hMotherThread = CreateThread(NULL, 0, MotherThread, this, 0, &thid);
	if (!hMotherThread)
		return -1;

	ACE_SOCK_Stream client_stream;
	ACE_INET_Addr addr;
	addr.set(nPort, addr.get_ip_address());

	int e = client_acceptor.open(addr);
	if (e == INVALID_SOCKET)
		return -1;

	while(true)
	{
		int e = client_acceptor.accept(client_stream);
		if (e == INVALID_SOCKET)
			continue;

		//Store in a buffer.
		EnterCriticalSection(&guard);
		queue.push_back(client_stream);
		LeaveCriticalSection(&guard);
	}
	
	return 0;
}

DWORD WINAPI CPROXY::MotherThread(LPVOID param)
{
	CPROXY *newobj = (CPROXY *)param;

	newobj->MotherThreadWorker();

	return 0;
}

int CPROXY::MotherThreadWorker()
{
	isMotherThreadRunning = true;

	while (isMotherThreadRunning)
	{
		EnterCriticalSection(&guard);
		bool isEmpty = queue.empty();
		LeaveCriticalSection(&guard);

		if (!isEmpty){
			DWORD thid;
			HANDLE hDaughterThread = CreateThread(NULL, 0, DaughterThread, this, 0, &thid);
			if (!hDaughterThread)
				continue;

			WaitForSingleObject(wait, INFINITE);
		}
	}
	return 0;
}

DWORD WINAPI CPROXY::DaughterThread(LPVOID param)
{
	ACE_LOG_MSG->msg_ostream (output, 0);
	ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
	CPROXY *newobj = (CPROXY *)param;

	newobj->DaughterThreadWorker();

	ACE_DEBUG((LM_DEBUG, ACE_TEXT("%t: Thread exited!\n")));

	return 0;
}

int CPROXY::DaughterThreadWorker()
{
	char buf1[BUFSIZE];
	char cServerAddress[256];
	int  nServerPort;

	EnterCriticalSection(&guard);
	ACE_SOCK_Stream client_stream = queue.front();
	queue.pop_front();
	LeaveCriticalSection(&guard);

	SetEvent(wait);

	const ACE_Time_Value t(2,0);

	size_t nLen = client_stream.recv(buf1, sizeof(buf1), &t);
	if(sizeof(buf1) > nLen)
		buf1[nLen+1] = '\0';
	if (nLen <= 0){
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("%t: Connection closed by browswer client while receiving requests.\n")));
		return -1;
	}

	//Parse the received packet from client.
	GetAddressAndPort(buf1, cServerAddress, &nServerPort);
	
	//Attempt to connect to remote server
	ACE_INET_Addr addr;
	addr.set(nServerPort, cServerAddress);
	ACE_SOCK_Stream server_stream;

	if (server_connector.connect(server_stream, addr) == INVALID_SOCKET){
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("%t: Connection closed by remote server while connecting.\n")));
		return -1;
	}

	//Send request to server.
	if (server_stream.send(buf1, nLen) == INVALID_SOCKET){
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("%t: Connection closed by remote server while sending requests.\n")));
		return -1;
	}

	//Retrieveing responses from server and send to client.
	char buf2[BUFSIZE];
	while(1){
		nLen =server_stream.recv(buf2, sizeof(buf2), &t);
		if (nLen <= 0){
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("%t: Connection closed by remote server while receiving responses.\n")));
			break;
		}

		if (client_stream.send((buf2), nLen) == SOCKET_ERROR){
			ACE_DEBUG((LM_DEBUG, ACE_TEXT("%t: Connection closed by browser client while sending responses.\n")));
			break;
		}

	}

	server_stream.close();
	client_stream.close();

	return 0;
}

int CPROXY::GetAddressAndPort(char *cStr, char *cAddress, int *nPort)
{
	//////////////////////////////////////////////////////////////////
	// This part is written by HU Zhongshan   
	// e-mail huzhongshan@hotmail.com OR yangjy@mail.njust.edu.cn
	// 1999-4-18
	//////////////////////////////////////////////////////////////////

	char buf[BUFSIZE], command[512], proto[128], *p;
	unsigned j;
	sscanf(cStr, "%s%s%s", command, buf, proto);
	p = strstr(buf, HTTP);
	
	if(p)  //HTTP
	{
		p += strlen(HTTP);
		unsigned i = 0;
		for(i = 0; i < strlen(p); i++)
			if(*(p + i) == '/') break;
		*(p+i)= 0;
		strcpy(cAddress, p);
		p=strstr(cStr, HTTP);
		for(j = 0; j < i + strlen(HTTP); j++)
			*(p + j) = ' ';  // to remove the host name: GET http://www.njust.edu.cn/ HTTP1.1  ==> GET / HTTP1.1
		*nPort = 80;         // default http port 
	}
	/*
	else
	{
		p = strstr(buf, FTP);    //FTP, Not supported, and following code can be omitted.
		if(!p) return 0;
		p += strlen(FTP);
		unsigned i = 0;
		for(i = 0; i < strlen(p); i++)
			if(*(p + i) == '/') break;        // Get The Remote Host
		*(p + i)=0;
		for(j = 0; j < strlen(p); j++)
			if(*(p + j) == ':') 
				{*nPort = atoi(p + j + 1);    // Get The Port
				 *(p + j) = 0;
				}
			 else *nPort = 21;		 			              

		strcpy(cAddress, p);
		p = strstr(cStr, FTP);
		for(j = 0; j < i + strlen(FTP); j++)
			*(p + j) = ' ';		
	}
	*/
	return 1; 
}
