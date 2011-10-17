/*
 GNU GPL Notice
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define VERSION "0.3"
#define VDATE "17oct11"

#define CODENAME "Candied Bacon"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

// All collected data
struct datastore {
	float 				load[3]; //loadavg
	int 				rqueue, pqueue; //loadavg
	unsigned long int 	cpu_usermode, cpu_low, cpu_sys, cpu_idle, cpu_wait,
				ctxt, btime, processes, procs_run, procs_block; //stat
	unsigned long int 	memtotal, memfree, buffers, cached, swapcached, dirty; //meminfo
	unsigned long int	vz_kmemsize[4], vz_lockedpages[4], vz_privvmpages[4], vz_shmpages[4], vz_numproc[4], vz_physpages[4], vz_vmguarpages[4], vz_oomguarpages[4],
				vz_numtcpsock[4], vz_numflock[4], vz_numpty[4], vz_numsiginfo[4], vz_tcpsndbuf[4], vz_tcprcvbuf[4], vz_othersockbuf[4], vz_dgramrcvbuf[4],
				vz_numothersock[4], vz_dcachesize[4], vz_numfile[4], vz_numiptent[4]; //user_beancounter

};

// Program configuration settings
struct config {
	float 	pause, jiffy; //How long to wait
	char	opt, optsub; //command line options
	char 	defDate[45], delimiter[5], ld_delimiter[5];
	int		showDate, count, showCounter, showLoad, corecount, showIOwait, showMemory, showVZ;
	short int	vz_ALL, vz_kmemsize, vz_lockedpages, vz_privvmpages, vz_shmpages, vz_numproc, vz_physpages, vz_vmguarpages, vz_oomguarpages,
				vz_numtcpsock, vz_numflock, vz_numpty, vz_numsiginfo, vz_tcpsndbuf, vz_tcprcvbuf, vz_othersockbuf, vz_dgramrcvbuf,
				vz_numothersock, vz_dcachesize, vz_numfile, vz_numiptent;
	char	clr_low[12], clr_med[12], clr_high[12], clr_vhigh[12], clr_norm[12];
	short int	isvz;
};

FILE *fptr; // for passing to signal()

void init(struct config *c)
{
	c->pause=2;
	c->showDate=0;
	c->showCounter=0;
	c->showLoad=1;
	c->showIOwait=0;
	c->showMemory=0;
	c->showVZ=0;
	strcpy(c->delimiter, "//");
	strcpy(c->ld_delimiter, ", ");
	strcpy(c->defDate, "%b %d %X %Y");
	strcpy(c->clr_low, "\e[1m");
	strcpy(c->clr_med, "\e[1;33m");
	strcpy(c->clr_high, "\e[1;31m");
	strcpy(c->clr_vhigh, "\e[0;30;41m");
	strcpy(c->clr_norm, "\e[0;m");
    c->jiffy=sysconf(_SC_CLK_TCK);
	if(access("/proc/user_beancounters", R_OK)!=-1)
		c->isvz=1;
	c->count=-1;
}


// Function fail() - Generic error
void fail()
{
				fprintf(stderr, "Error gathering data");
				exit(1);
}


// Function: showHelp - how to use
void showHelp()
{
	printf("\n");
	printf("	-d	time		delay, in seconds\n");
	printf("	-D	[format]	date, with optional formatting\n");
	printf("	-h				this help screen\n");
	printf("	-v	vz_type		get VZ beans of vz_type\n");
	printf("		A			all; display only on VZ bean\n");
	printf("		f			numfile\n");
	printf("		k			kmemsize\n");
	printf("		p			privvmpages\n");
	printf("		P			numproc\n");
	printf("		s			shmpages\n");
	printf("		S			numothersock\n");
	printf("	-V				version information\n");
	printf("\n");
}


// Function: showVersion() - some stuffs about the program
//     Note: Need to break it up.
void showVersion()
{
	printf("progname: Loadout\n  author: Dusten Barker\n version: %s\n written: %s\ncodename: %s\n", VERSION, VDATE, CODENAME);
}


// Function isInt - Test if cstring is all ints
//    Note: This is fucking amazing.
//      In: cstring to test
// Returns: true if cstring only contains numbers, else false
int isInt(const char *s)
{
	if((s==NULL)||(*s=='\0')||(*s==' ')) return 0;
	char *p;
	if(!strtol(s, &p, 10)) return 0; // Test if function succeeds
	return(*p=='\0'); // <-- The magic; Test if it made it to the end
}


// Function isFloat - Test if cstring is a float
//    Note: This is fucking amazing #2.
//      In: cstring to test
// Returns: true if cstring only contains float, else false
int isFloat(const char *s)
{
	if((s==NULL)||(*s=='\0')||(*s==' ')) return 0;
	char *p;
	if(!strtof(s, &p)) return 0; // Test if function succeeds
	return(*p=='\0'); // <-- The magic; Test if it made it to the end
}


// Function: bye - Graceful exit
//     Note: Don't really need, but I was bored. Thats why I do most things.
//       In: Signal catch
void bye()
{
	printf("\nBoom: Headshot.\n\n");
	if(ftell(fptr)>0) fclose(fptr);
	(void)signal(SIGINT, SIG_DFL);
	exit(0);
}


// Function: getData() - for collecting system data
//     Note: Pimpshit
//   In/Out: Pointer to datastore to reference against
void getData(struct datastore *d, struct config *c)
{
	char str[15], *pch;

	// Get stat
	fptr=fopen("/proc/stat", "r");
	while(fscanf(fptr, "%s", str)!=EOF)
	{
		if(!strcmp(str, "cpu"))
			if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->cpu_usermode, &d->cpu_low, &d->cpu_sys, &d->cpu_idle, &d->cpu_wait))
				fail();
		if(!strcmp(str, "ctxt"))
			if(!fscanf(fptr, "%lu", &d->ctxt))
				fail();
		if(!strcmp(str, "btime"))
			if(!fscanf(fptr, "%lu", &d->btime))
				fail();
		if(!strcmp(str, "processes"))
			if(!fscanf(fptr, "%lu", &d->processes))
				fail();
		if(!strcmp(str, "procs_running"))
			if(!fscanf(fptr, "%lu", &d->procs_run))
				fail();
		if(!strcmp(str, "procs_blocked"))
			if(!fscanf(fptr, "%lu", &d->procs_block))
				fail();
	}
	fclose(fptr);

	// Get load
	fptr=fopen("/proc/loadavg", "r");
	if(!fscanf(fptr, "%f %f %f %s", &d->load[0], &d->load[1], &d->load[2], str))
		fail();
	pch=strtok(str,"/ ");
	d->rqueue=(atoi(pch));
	pch=strtok(NULL, "/ ");
	d->pqueue=(atoi(pch));
	fclose(fptr);

	// Get memory
	fptr=fopen("/proc/meminfo", "r");
	while(fscanf(fptr, "%s", str)!=EOF)
	{
		if (!strcmp(str, "MemTotal:"))
			if(!fscanf(fptr, "%lu", &d->memtotal))
				fail();
		if (!strcmp(str, "MemFree:"))
			if(!fscanf(fptr, "%lu", &d->memfree))
				fail();
		if (!strcmp(str, "Buffers:"))
			if(!fscanf(fptr, "%lu", &d->buffers))
				fail();
		if (!strcmp(str, "Cached:"))
			if(!fscanf(fptr, "%lu", &d->cached))
				fail();
		if (!strcmp(str, "SwapCached:"))
			if(!fscanf(fptr, "%lu", &d->swapcached))
				fail();
		if (!strcmp(str, "Dirty:"))
			if(!fscanf(fptr, "%lu", &d->dirty))
				fail();
	}
	fclose(fptr);

	// Get VZ beans
	if(c->isvz)
	{
		fptr=fopen("/proc/user_beancounters", "r");
		while((fscanf(fptr, "%s", str)!=EOF))
		{
			if(!strcmp(str, "kmemsize"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_kmemsize[0], &d->vz_kmemsize[1],&d->vz_kmemsize[2],&d->vz_kmemsize[3],&d->vz_kmemsize[4]))
					fail();
			if(!strcmp(str, "lockedpages"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_lockedpages[0], &d->vz_lockedpages[1],&d->vz_lockedpages[2],&d->vz_lockedpages[3],&d->vz_lockedpages[4]))
					fail();
			if(!strcmp(str, "privvmpages"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_privvmpages[0], &d->vz_privvmpages[1],&d->vz_privvmpages[2],&d->vz_privvmpages[3],&d->vz_privvmpages[4]))
					fail();
			if(!strcmp(str, "shmpages"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_shmpages[0], &d->vz_shmpages[1],&d->vz_shmpages[2],&d->vz_shmpages[3],&d->vz_shmpages[4]))
					fail();
			if(!strcmp(str, "numproc"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_numproc[0], &d->vz_numproc[1],&d->vz_numproc[2],&d->vz_numproc[3],&d->vz_numproc[4]))
					fail();
			if(!strcmp(str, "physpages"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_physpages[0], &d->vz_physpages[1],&d->vz_physpages[2],&d->vz_physpages[3],&d->vz_physpages[4]))
					fail();
			if(!strcmp(str, "vmguarpages"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_vmguarpages[0], &d->vz_vmguarpages[1],&d->vz_vmguarpages[2],&d->vz_vmguarpages[3],&d->vz_vmguarpages[4]))
					fail();
			if(!strcmp(str, "oomguarpages"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_oomguarpages[0], &d->vz_oomguarpages[1],&d->vz_oomguarpages[2],&d->vz_oomguarpages[3],&d->vz_oomguarpages[4]))
					fail();
			if(!strcmp(str, "numtcpsock"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_numtcpsock[0], &d->vz_numtcpsock[1],&d->vz_numtcpsock[2],&d->vz_numtcpsock[3],&d->vz_numtcpsock[4]))
					fail();
			if(!strcmp(str, "numflock"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_numflock[0], &d->vz_numflock[1],&d->vz_numflock[2],&d->vz_numflock[3],&d->vz_numflock[4]))
					fail();
			if(!strcmp(str, "numpty"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_numpty[0], &d->vz_numpty[1],&d->vz_numpty[2],&d->vz_numpty[3],&d->vz_numpty[4]))
					fail();
			if(!strcmp(str, "numsiginfo"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_numsiginfo[0], &d->vz_numsiginfo[1],&d->vz_numsiginfo[2],&d->vz_numsiginfo[3],&d->vz_numsiginfo[4]))
					fail();
			if(!strcmp(str, "tcpsndbuf"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_tcpsndbuf[0], &d->vz_tcpsndbuf[1],&d->vz_tcpsndbuf[2],&d->vz_tcpsndbuf[3],&d->vz_tcpsndbuf[4]))
					fail();
			if(!strcmp(str, "tcprcvbuf"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_tcprcvbuf[0], &d->vz_tcprcvbuf[1],&d->vz_tcprcvbuf[2],&d->vz_tcprcvbuf[3],&d->vz_tcprcvbuf [4]))
					fail();
			if(!strcmp(str, "othersockbuf"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_othersockbuf[0], &d->vz_othersockbuf[1],&d->vz_othersockbuf[2],&d->vz_othersockbuf[3],&d->vz_othersockbuf[4]))
					fail();
			if(!strcmp(str, "dgramrcvbuf"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_dgramrcvbuf[0], &d->vz_dgramrcvbuf[1],&d->vz_dgramrcvbuf[2],&d->vz_dgramrcvbuf[3],&d->vz_dgramrcvbuf[4]))
					fail();
			if(!strcmp(str, "numothersock"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_numothersock[0], &d->vz_numothersock[1],&d->vz_numothersock[2],&d->vz_numothersock[3],&d->vz_numothersock[4]))
					fail();
			if(!strcmp(str, "dcachesize"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_dcachesize[0], &d->vz_dcachesize[1],&d->vz_dcachesize[2],&d->vz_dcachesize[3],&d->vz_dcachesize[4]))
					fail();
			if(!strcmp(str, "numfile"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_numfile[0], &d->vz_numfile[1],&d->vz_numfile[2],&d->vz_numfile[3],&d->vz_numfile[4]))
					fail();
			if(!strcmp(str, "numiptent"))
				if(!fscanf(fptr, "%lu %lu %lu %lu %lu", &d->vz_numiptent[0], &d->vz_numiptent[1],&d->vz_numiptent[2],&d->vz_numiptent[3],&d->vz_numiptent[4]))
					fail();
		}
		fclose(fptr);
	}
	return;
}

// Function: doOnce - Gather startup info
//   In/Out: configuration struct
void doOnce(struct config *conf)
{
	char str[15];
	fptr=fopen("/proc/cpuinfo", "r");
	while(fscanf(fptr, "%s", str)!=EOF)
	if(!strcmp(str, "processor")) conf->corecount++;
	fclose(fptr);

	printf("C:%i", conf->corecount);
	printf(" - isVZ:%i", conf->isvz);

	printf("\n");


}

//Function: doCounter - run counter
//  In/Out: config struct
void doCounter(struct config *conf)
{
}

// Function: doResults - calculate and assemble output string
//   In/Out: datastore structs to compare and configuration struct
void doResults(struct datastore *d1, struct datastore *d2, struct config *conf)
{
	time_t curtime;
	struct tm *loctime;
	char str[45], color[12];
	short int i,i2=0,i3=0;
	float tmp;



	// Date
	if(conf->showDate)
	{
		if(i2) printf("%s", conf->delimiter);
		curtime=time(NULL);
		loctime=localtime(&curtime);
		strftime (str, 45, conf->defDate, loctime);
		printf("%s", str);
		i2=1;
	}

	// CPU Load
	if(conf->showLoad)
	{
		if(i2) printf("%s", conf->delimiter);
		for(i=0; i<=2; i++)
		{
			if(d2->load[i]>=conf->corecount*2) strcpy(color, conf->clr_vhigh);
			else if(d1->load[i]>=conf->corecount*1.05) strcpy(color, conf->clr_high);
			else if((d1->load[i]>conf->corecount-(conf->corecount*0.05))&&(d1->load[i]<conf->corecount*1.05)) strcpy(color, conf->clr_med);
			else strcpy(color, conf->clr_low);
			printf("%s%.2f%s", color, d1->load[i], conf->clr_norm);
			if(i<2) printf("%s", conf->ld_delimiter);
		}
		i2=1;
	}

	// IO Wait
	if(conf->showIOwait)
	{
		if(i2) printf("%s", conf->delimiter);
		tmp=((((float)(d2->cpu_wait-d1->cpu_wait)*100*conf->pause)/conf->jiffy)/conf->corecount)/conf->pause;
		if(tmp>=25) strcpy(color, conf->clr_vhigh);
		else if(tmp>=20) strcpy(color, conf->clr_high);
		else if(tmp>15) strcpy(color, conf->clr_med);
		else strcpy(color, conf->clr_norm);
		printf("%s%.2f%s%%", color, tmp, conf->clr_norm);
		i2=1;
	}

	// Memory
	if(conf->showMemory)
	{
		if(i2) printf("%s", conf->delimiter);
		tmp=((float)(d2->memfree-d2->cached)/d2->memtotal)*100;
		if(tmp>=99) strcpy(color, conf->clr_vhigh);
		else if(tmp>=95) strcpy(color, conf->clr_high);
		else if(tmp>90) strcpy(color, conf->clr_med);
		else strcpy(color, conf->clr_norm);
		printf("%s%.2f%s%%", color, tmp, conf->clr_norm);
		i2=1;
	}

	// VZ  -  Need to put these tests into its own function, also to add output to a string; this will help with delimiter detection.
	if(conf->showVZ)
	{
		if(conf->vz_ALL)
		{
			printf(" ");
			i3=d2->vz_kmemsize[4]-d1->vz_kmemsize[4];
			if(i3) printf("%i kmemsize ", i3);
			i3=d2->vz_lockedpages[4]-d1->vz_lockedpages[4];
			if(i3) printf("%i lockedpages ", i3);
			i3=d2->vz_privvmpages[4]-d1->vz_privvmpages[4];
			if(i3) printf("%i privvmpages ", i3);
			i3=d2->vz_shmpages[4]-d1->vz_shmpages[4];
			if(i3) printf("%i shmpages ", i3);
			i3=d2->vz_numproc[4]-d1->vz_numproc[4];
			if(i3) printf("%i numproc ", i3);
			i3=d2->vz_physpages[4]-d1->vz_physpages[4];
			if(i3) printf("%i physpages ", i3);
			i3=d2->vz_vmguarpages[4]-d1->vz_vmguarpages[4];
			if(i3) printf("%i vmguarpages ", i3);
			i3=d2->vz_oomguarpages[4]-d1->vz_oomguarpages[4];
			if(i3) printf("%i oomguarpages ", i3);
			i3=d2->vz_numtcpsock[4]-d1->vz_numtcpsock[4];
			if(i3) printf("%i numtcpsock ", i3);
			i3=d2->vz_numflock[4]-d1->vz_numflock[4];
			if(i3) printf("%i numflock ", i3);
			i3=d2->vz_numpty[4]-d1->vz_numpty[4];
			if(i3) printf("%i numpty ", i3);
			i3=d2->vz_numsiginfo[4]-d1->vz_numsiginfo[4];
			if(i3) printf("%i numsiginfo ", i3);
			i3=d2->vz_tcpsndbuf[4]-d1->vz_tcpsndbuf[4];
			if(i3) printf("%i tcpsndbuf ", i3);
			i3=d2->vz_tcprcvbuf[4]-d1->vz_tcprcvbuf[4];
			if(i3) printf("%i tcprcvbuf ", i3);
			i3=d2->vz_othersockbuf[4]-d1->vz_othersockbuf[4];
			if(i3) printf("%i othersockbuf ", i3);
			i3=d2->vz_dgramrcvbuf[4]-d1->vz_dgramrcvbuf[4];
			if(i3) printf("%i dgramrcvbuf ", i3);
			i3=d2->vz_numothersock[4]-d1->vz_numothersock[4];
			if(i3) printf("%i numothersock ", i3);
			i3=d2->vz_dcachesize[4]-d1->vz_dcachesize[4];
			if(i3) printf("%i dcachesize ", i3);
			i3=d2->vz_numfile[4]-d1->vz_numfile[4];
			if(i3) printf("%i numfile ", i3);
			i3=d2->vz_numiptent[4]-d1->vz_numiptent[4];
			if(i3) printf("%i numiptent ", i3);
		}
	}



	printf("\n");
}



// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [  Da Main  ] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int main(int argc, char *argv[])
{
	struct datastore *data1, *data2;
	struct config *conf;
	//Allocate memory
	if((data1=calloc(1, sizeof(struct datastore)))==NULL||(data2=calloc(1, sizeof(struct datastore)))==NULL)
	{
		fprintf(stderr, "Failed to allocated %lu bytes.\n", sizeof(struct datastore));
		return 1;
	}
	if((conf=calloc(1, sizeof(struct config)))==NULL)
	{
		fprintf(stderr, "Failed to allocated %lu bytes.\n", sizeof(struct datastore));
		return 1;
	}

	(void)signal(SIGINT, bye); //tc shuts up cw
	opterr=0;
	setvbuf(stdout, NULL, _IOLBF,0);

	init(conf);

	while((conf->opt=getopt(argc, argv, "+wmhv:VD::d:Cc:"))!=-1)
		switch(conf->opt)
		{
			case 'h':
				showHelp();
				exit(0);
				break;
			case 'v':
				if(strpbrk(optarg, "A"))
					conf->vz_ALL=1;
				if(strpbrk(optarg, "k"))
					conf->vz_kmemsize=1;
				if(strpbrk(optarg, "p"))
					conf->vz_privvmpages=1;
				if(strpbrk(optarg, "s"))
					conf->vz_shmpages=1;
				if(strpbrk(optarg, "P"))
					conf->vz_numproc=1;
				if(strpbrk(optarg, "f"))
					conf->vz_numfile=1;
				if(strpbrk(optarg, "S"))
					conf->vz_numothersock=1;
				break;
			case 'V':
				showVersion();
				exit(0);
				break;
			case 'd':
				// Base case - Delay 0 makes this consume 20-25% CPU on my 4-core Desktop. 0% for delay>=1.
				if(!strcmp(optarg,"0")) fprintf(stderr, "Argument 0 is not allowed.\n");
				else if(isFloat(optarg)) conf->pause=atof(optarg);	// Test = bye-bye segfaults
				else fprintf(stderr, "Argument %s is invalid. Must be an integer value.\n", optarg);
				if(conf->pause < 0.1)
				{
					fprintf(stderr, "Delay cannot be smaller than 0.1 seconds.\n");
					exit(1);
				}
				break;
			case 'D':
				if(optarg) strcpy(conf->defDate, optarg);
				conf->showDate=1;
				break;
			case 'm':
				conf->showMemory=1;
				break;
			case 'w':
				conf->showIOwait=1;
				break;
			/*case '?':
				if (isprint(optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;
				break;
			default:
				abort();;*/
		}
	if(optind<argc)
		if(!strcmp(argv[optind], "moo"))
		{
			printf("ZOMG WOW!!!11!111one1!!! Super cow powers! KTHXBAI!\n");
			exit(0);
		}
	
	doOnce(conf);

	while(1)
	{

		getData(data1,conf);
		usleep(1000000*conf->pause);
		if(conf->count!=-1) doCounter(conf);
		getData(data2,conf);
		doResults(data1, data2, conf);


	}

/*
	getData(data1);
	usleep(1000000*conf.pause);
	getData(data2);
	printf("%lu %lu %lu %lu %lu\n", data1->cpu_usermode, data1->cpu_low, data1->cpu_sys, data1->cpu_idle, data1->cpu_wait);
	printf("%lu %lu %lu %lu %lu\n", data2->cpu_usermode, data2->cpu_low, data2->cpu_sys, data2->cpu_idle, data2->cpu_wait);
	printf("%f %f %f %i %i \n", data2->load[0], data2->load[1], data2->load[2], data2->rqueue, data2->pqueue);
	printf("Context Switches: %lu\n", data2->ctxt);
	if(conf.vz_kmemsize) printf("kememsize ");
	if(conf.vz_privvmpages) printf("privvmpages ");
	if(conf.vz_numfile) printf("numfile ");
	printf("\n");
*/
	return 0;
}


/* Structured test output
[root@the ~]# cat /proc/user_beancounters 
Version: 2.5
       uid  resource                     held              maxheld              barrier                limit              failcnt
      209:  kmemsize                 25719073             26533137             52428800             52428800                    0
            lockedpages                     0                    0  9223372036854775807  9223372036854775807                    0
            privvmpages                247891               249098              1000000              1000000                    0
            shmpages                    10427                10779               150000               150000                    0
            dummy                           0                    0  9223372036854775807  9223372036854775807                    0
            numproc                       115                  118                  600                  600                    0
            physpages                   90533                90721  9223372036854775807  9223372036854775807                    0
            vmguarpages                     0                    0         206158430208         206158430208                    0
            oomguarpages                90535                90723  9223372036854775807         164926744166                    0
            numtcpsock                     34                   38                  800                  800                    0
            numflock                       15                   16  9223372036854775807  9223372036854775807                    0
            numpty                          1                    1                   30                   30                    0
            numsiginfo                      0                    2  9223372036854775807  9223372036854775807                    0
            tcpsndbuf                  601840               601840  9223372036854775807  9223372036854775807                    0
            tcprcvbuf                  557056               540672  9223372036854775807  9223372036854775807                    0
            othersockbuf               264032               273584  9223372036854775807  9223372036854775807                    0
            dgramrcvbuf                     0                 8472  9223372036854775807  9223372036854775807                    0
            numothersock                  178                  185                  300                  300                  524
            dcachesize                2044203              2064861  9223372036854775807  9223372036854775807                    0
            numfile                      8072                 8195  9223372036854775807  9223372036854775807                    0
            dummy                           0                    0                    0                    0                    0
            dummy                           0                    0                    0                    0                    0
            dummy                           0                    0                    0                    0                    0
            numiptent                      14                   14                 2000                 2000                    0

*/

