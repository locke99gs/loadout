/*	
	Loadout (in C!), by Dusten Barker
	---------------------------------
	Fuck with this code before I license it
	and I'll hit you in your filfthy face.


	v0.2 - 25sep11 - "Clear Water"
		-Previous versions were bash scripts
		-Cause v0.1 stalls at loads ~10*corecount
		-Cause bash doesn't do fractional maths with built-ins or core-utils.
		-Cause this kickassery spawns one process vs a bajillion.
		-Cause I'm awesome, bored, and this is the sex.
		*5k for command line argument and sanity testing. Fuck.
	
	v0.2.1 - 25sep11 - "More Beef"
		+Added signal handling
		*Signal handling increases binary size ~200 bytes.
	v0.2.2 - 26sep11 - "Horror Machine"
		+Can now redirect output on command line
		-Put errors on stderr
		+Can now set delay
		+Can now display date, with configurable date output.
		*Surpasses feature parity with v0.1 bash script
		*POSIX compliant, cause I'm amazing
		+Add counter for up, down, and show or not.
		+Add rudimentary IOwait colorized output
			-I think my math works. Will have to run by somebody.
			-Also not sure about the colors I used here. Good guess.
		*And to wrap up, 0 compile warnings with -Wall.
*/

#define VERSION "0.2.2"
#define VDATE "26sep11"
#define CODENAME "Horror Machine"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

// Global
FILE *fptr; // for passing to signal()
int getIOuse, getIOuse2; // I'm a dirty cheater, I know.

// Function: showHelp - how to use
void showHelp()
{
	printf("I should put something here.\nSomeday, if anybody gives a shit.\n");
}

// Function: showVersion() - some stuffs about the program
//     Note: Need to break it up.
void showVersion()
{
	printf("progname: Loadout\n  author: Dusten Barker\n version: %s\n written: %s\ncodename: %s\n", VERSION, VDATE, CODENAME);
}

// Function: iowait - Try to get iowait
//     Note: This voodoo may or may not work
//       In: 0 to start, 1 to end.
//  Returns: Int of wait ticks when ending. None on start.
int getIOwait(int i2)
{
	char str[45];
	int i, er;
	if(i2==0)
	{
		fptr=fopen("/proc/stat", "r");
		er=fscanf(fptr, "%s", str);
		for(i=1;i<=5;i++)
			if(!fscanf(fptr, "%i", &getIOuse))
			{
				fprintf(stderr, "Error gathering iowait data");
				exit(1);
			}
		fclose(fptr);
		return 0;
	}
	if(i2==1)
	{
		fptr=fopen("/proc/stat", "r");
		er=fscanf(fptr, "%s", str);
		for(i=1;i<=5;i++)
			if(!fscanf(fptr, "%i", &getIOuse2))
			{
				fprintf(stderr, "Error gathering iowait data");
				exit(1);
			}
		fclose(fptr);
		return (getIOuse2-getIOuse);
	}
	return 0;


}


// Function: bye - Graceful exit
//     Note: Don't really need, but I was bored. Thats why I do most things.
//       In: Signal catch
void bye()
{
	printf("\nBoom: Headshot.\n\n");
	if (ftell(fptr)>0) fclose(fptr);
	(void)signal(SIGINT, SIG_DFL);
	exit(0);
}

// Function isInt - Test if cstring is all ints
//    Note: This is fucking amazing.
//      In: cstring to test
// Returns: true if cstring only contains numbers, else false
int isInt(const char *s)
{
	if((s==NULL)||(*s=='\0')||(*s==' ')) return 0;
	char *p;
	if (!strtol(s, &p, 10)) return 0; // Test if it function succeeds
	return(*p=='\0'); // <-- The magic; Test if it made it to the end
}


// --------------------- Main ----------------------
int main(int argc, char *argv[])
{
	float ld,jiffy=sysconf(_SC_CLK_TCK),tmp;
	char defDate[45]="", *dateFormat=defDate, str[45], opt, color[12], delimiter[]="//";
	int first=1, corecount=0, i=0, showDate=0, pause=2, docount=-1, cnt=0, showCounter=0;
	time_t curtime, getIO=0;
	struct tm *loctime;

	(void)signal(SIGINT, bye); //tc shuts up cw
	opterr=0;
    setvbuf(stdout, NULL, _IOLBF,0);

	while((opt=getopt(argc, argv, "+whvD::d:Cc:"))!=-1)
		switch(opt)
		{
			case 'h':
				showHelp();
				exit(0);
				break;
			case 'v':
				showVersion();
				exit(0);
				break;
			case 'c':
				if(!strcmp(optarg,"0"))
				{
					docount=0;
					cnt=1;
				}
				else if(isInt(optarg)) docount=cnt=atoi(optarg);
				else fprintf(stderr, "Argument %s is invalid. Must be an integer value.\n", optarg);
				break;
			case 'C':
				showCounter=1;
				break;
			case 'd':
				// Base case - Delay 0 makes this consume 20-25% CPU on my 4-core Desktop. 0% for delay>=1.
				if(!strcmp(optarg,"0")) fprintf(stderr, "Argument 0 is not allowed.\n");
				else if(isInt(optarg)) pause=atoi(optarg);	// Test = bye-bye segfaults
				else fprintf(stderr, "Argument %s is invalid. Must be an integer value.\n", optarg);
				break;
			case 'D':
				if(optarg) dateFormat=optarg;
				else dateFormat="%b %d %X %Y";
				showDate=1;
				break;
			case 'w':
				getIO=1;
				break;
		}
	if(optind<argc)
		if(!strcmp(argv[optind], "moo"))
		{
			printf("ZOMG WOW!!!11!111one1!!! Super cow powers! KTHXBAI!\n");
			exit(0);
		}
	if((showCounter)&&(!cnt))
	{
		docount=0;
		cnt=1;
	}

	// Get CPU core count
	fptr=fopen("/proc/cpuinfo", "r");
	while(fscanf(fptr, "%s", str)!=EOF)	
		if(!strcmp(str, "processor")) corecount++;
	fclose(fptr);

	// Main loop
	while(1)
	{
		// Do counter
		if(showCounter) printf("%4i %s ", cnt, delimiter);
		if(docount>0) cnt--;
		if(docount==0) cnt++;
		// Do date
		if (showDate)
		{
			curtime=time(NULL);
			loctime=localtime(&curtime);
			strftime (str, 45, dateFormat, loctime);
			printf("%s %s ", str, delimiter);
		}
		if(getIO) getIOwait(0);
		if(!first) sleep(pause);
		first=0;
		// Do load
		fptr=fopen("/proc/loadavg", "r");
		for(i=1; i<=3; i++)
		{
			// Get load stuffs
			if(!fscanf(fptr, "%f", &ld))
			{
				fprintf(stderr, "Error gathering load data");
				exit(1);
			}
			// This is the whole damned point of this program.
			if(ld>=corecount*2) strcpy(color, "\E[0;30;41m");
			else if(ld>=corecount*1.05) strcpy(color, "\E[1;31m");
			else if((ld>corecount-(corecount*0.05))&&(ld<corecount*1.05)) strcpy(color, "\E[1;33m");
			else strcpy(color, "\E[1m");		
			printf("%s%.2f\e[0;m", color, ld);
			if(i!=3) printf(" - ");
		}
		fclose(fptr); //Close asap. Can I rewind instead? Prolly not. Find out later.
		// Show IO stuffs
		if(getIO)
		{
			tmp=(((float)getIOwait(1)*100*pause)/jiffy)/corecount;
			if(tmp>=25) strcpy(color, "\E[0;30;41m");
			else if(tmp>=15) strcpy(color, "\E[1;31m");
			else if(tmp>10) strcpy(color, "\E[1;33m");
			else strcpy(color, "\E[0m");
			printf(" %s %s%.2f\e[0;m", delimiter, color, tmp);
		}

		printf("\n");
		if((docount>0)&&(cnt==0)) break;
	}
	printf("Boom.\n");
	return 0;
}