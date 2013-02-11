#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "error.h"
#include "processor.h"
#include "processor_utils.h"
#include "utils.h"
#include "debug_console.h"
#include "debug_console_simple.h"
#include "log.h"

bool single_step = false;
const char *logfile = NULL;

volatile bool terminate = false;

void help()
{
	fprintf(stderr, "-d     debug (console) mode\n");
	fprintf(stderr, "-S     enable single step mode\n");
	fprintf(stderr, "-l x   logfile to write to\n");
	fprintf(stderr, "-V     show version & exit\n");
	fprintf(stderr, "-h     this help & exit\n");
}

void version()
{
	printf("miep v" VERSION ", (C) 2013 by folkert@vanheusden.com\n\n");
}

void sig_handler(int sig)
{
        terminate = true;
}

int main(int argc, char *argv[])
{
	int c = -1;
	bool debug = false;

	while((c = getopt(argc, argv, "dSl:")) != -1)
	{
		switch(c)
		{
			case 'd':
				debug = true;
				break;

			case 'S':
				single_step = true;
				break;

			case 'l':
				logfile = optarg;
				break;

			case 'V':
				version();
				return 0;

			case 'h':
				version();
				help();
				return 0;

			default:
				version();
				help();
				return 1;
		}
	}

	dolog("*** START ***");

	signal(SIGHUP , SIG_IGN);
	signal(SIGTERM, sig_handler);
	signal(SIGINT , sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGPIPE, SIG_IGN);

	debug_console *dc = debug ? new debug_console() : new debug_console_simple();

	dc -> init();

	memory_bus *mb = new memory_bus();

	memory *m = new memory(64 * 1024 * 1024, true);
	mb -> register_memory(0, 0xfff, m);

	unsigned char *prom = NULL;
	int prom_len = -1;
	load_file("ip20prom.070-8116-004.BE.bin", &prom, &prom_len);
	memory *m_prom = new memory(prom, prom_len);
	mb -> register_memory(0xffffffffbfc00000, 0x7ffff, m_prom); // IP20, 32bit

	processor *p = new processor(dc, mb);

#ifdef _PROFILE
	int cnt = 0;
	for(;!terminate;)
	{
		p -> reset();

		for(int nr=0; nr<600; nr++)
		{
			// dc -> tick(p);
			p -> tick();
		}

		cnt++;
		if (cnt % 100 == 0)
			printf("%d\r", cnt);
	}

#else

	if (single_step)
	{
		for(;!terminate;)
		{
			dc -> tick(p);
			p -> tick();

			if (single_step)
				getch();
		}
	}
	else
	{
		for(;!terminate;)
		{
			dc -> tick(p);
			p -> tick();
		}
	}
#endif

	delete dc;

	delete p;
	delete mb;
	delete m;

	dolog("--- END ---");

	return 0;
}
