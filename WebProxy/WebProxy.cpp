#include "stdafx.h"
#include "proxy.h"

int ACCEPTOR_PORT	= 5060;

int main(int argc, char *argv[])
{	
	CPROXY *MyProxy = new CPROXY;

	MyProxy->Run(ACCEPTOR_PORT);

	// dog-nail
	while(1){}

	return 0;
}

