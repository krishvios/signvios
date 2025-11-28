/*!
* \file stiSysUtilizationInfo.cpp
*
* This Module contains miscellaneous functions that are used as supporting
* functions for carrying out the necessary duties of the overall
* project.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/



//
// Includes
//
#include "stiSysUtilizationInfo.h"

#ifdef stiLINUX

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#ifndef __APPLE__
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
#endif


//
// Constants
//
#define PROC_SUPER_MAGIC 0x9fa0
#define PROCFS "/proc"


//
// Typedefs
//
#define bytetok(x)	(((x) + 512) >> 10)


//
// Forward Declarations
//

//
// Globals
//

static const char *cpustatenames[NCPUSTATES+1] =
{
	"user", "nice", "sys", "idle",
		nullptr
};

static const char *memorynames[NMEMSTATES+1] =
{
	"total", "used", "free", "shared", "buffers", "cached",
		nullptr
};


// these are for calculating cpu state percentages
static long cpu_time[NCPUSTATES];
static long cpu_old[NCPUSTATES];
static long cpu_diff[NCPUSTATES];

// these are for passing data back 
static int cpu_states[NCPUSTATES];
static int memory_stats[NMEMSTATES];

//
// Locals
//

//
// Function Definitions
//

static inline char *
skip_ws(const char *p)
{
	while (isspace(*p))
	{
		p++;
	}
	
	return (char *)p;
}

static inline char *
skip_token(const char *p)
{
	while (isspace(*p))
	{
		p++;
	}

	while (*p && !isspace(*p))
	{
		p++;
	}
	
	return (char *)p;
}

/*
 *  percentages(cnt, out, newVal, oldVal, diffs) - calculate percentage change
 *	between array "oldVal" and "newVal", putting the percentages i "out".
 *	"cnt" is size of each array and "diffs" is used for scratch space.
 *	The array "oldVal" is updated on each call.
 *	The routine assumes modulo arithmetic.  This function is especially
 *	useful on BSD mchines for calculating cpu state percentages.
*/

long percentages(int cnt, int * out, long * newVal, long * oldVal, long * diffs)
{
	int i;
	long change;
	long total_change;
	long *dp;
	long half_total;
	
	// initialization
	total_change = 0;
	dp = diffs;
	
	// calculate changes for each state and the overall change 
	for (i = 0; i < cnt; i++)
	{
		if ((change = *newVal - *oldVal) < 0)
		{
			// this only happens when the counter wraps
			change = (int)
				((unsigned long)*newVal-(unsigned long)*oldVal);
		}
		total_change += (*dp++ = change);
		*oldVal++ = *newVal++;
	}
	
	// avoid divide by zero potential
	if (total_change == 0)
	{
		total_change = 1;
	}
	
	// calculate percentages based on overall change, rounding up
	half_total = total_change / 2L;
	for (i = 0; i < cnt; i++)
	{
		*out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
	}
	
	//return the total in case the caller wants to use it
	return(total_change);
}

int stiSystemInfoInit(void)
{

#if 0
	// Initialize the variables
	pstInfo->last_pid = -1;
	for (int i=0; i<NUM_AVERAGES; i++)
	{
		pstInfo->load_avg[i] = 0.0;
	}
	pstInfo->cpustates = NULL;
	pstInfo->memory = NULL;
	pstInfo->cpustate_names = NULL;
	pstInfo->memory_names = NULL; 
#endif

	memset (cpu_time, 0, NCPUSTATES * sizeof (long));
	memset (cpu_old, 0, NCPUSTATES * sizeof (long));
	memset (cpu_diff, 0, NCPUSTATES * sizeof (long));    


	// Make sure the proc filesystem is mounted
	struct statfs sb{};
	if (statfs(PROCFS, &sb) < 0 || sb.f_type != PROC_SUPER_MAGIC)
	{
		fprintf(stderr, "proc filesystem not mounted on " PROCFS "\n");
		return -1;
	}
	
	// chdir to the proc filesystem to make things easier
	// Don't do this unless it gets changed back.  During the course of the program, we try creating
	// files on disk (e.g. a pipe) that fail to be created in this directory due to permissions.
	//	chdir(PROCFS);

#if 0  
	// fill in the information
	pstInfo->cpustate_names = cpustatenames;
	pstInfo->memory_names = memorynames;
#endif  
	
	// all done!
	return 0;
}

void stiSystemInfoGet (
	struct SstiSysUtilizationInfo *pstInfo)
{
	char buffer[4096+1];
	int fd, len;
	char *p;

	pstInfo->cpustate_names = cpustatenames;
	pstInfo->memory_names = memorynames;
  
	//	
	// get load averages
	//
	{
		fd = open(PROCFS "/loadavg", O_RDONLY);
		len = read(fd, buffer, sizeof(buffer)-1);
		close(fd);
		buffer[len] = '\0';
		
		pstInfo->load_avg[0] = strtod(buffer, &p);
		pstInfo->load_avg[1] = strtod(p, &p);
		pstInfo->load_avg[2] = strtod(p, &p);
		p = skip_token(p); // skip running/tasks
		p = skip_ws(p);
		if (*p)
		{
			pstInfo->last_pid = atoi(p);
		}
		else
		{
			pstInfo->last_pid = -1;
		}
	}

	//
	// get the cpu time info
	//
	{    
		fd = open(PROCFS "/stat", O_RDONLY);
		len = read(fd, buffer, sizeof(buffer)-1);
		close(fd);
		buffer[len] = '\0';
		
		p = skip_token(buffer);	// skip "cpu"
		cpu_time[0] = strtoul(p, &p, 0);
		cpu_time[1] = strtoul(p, &p, 0);
		cpu_time[2] = strtoul(p, &p, 0);
		cpu_time[3] = strtoul(p, &p, 0);
		
		/// convert cpu_time counts to percentages
		int cnt = NCPUSTATES;
		percentages(cnt, cpu_states, cpu_time, cpu_old, cpu_diff);
		
		pstInfo->cpustates = cpu_states;
	}

	//
	// get system wide memory usage
	//  
	{
		fd = open(PROCFS "/meminfo", O_RDONLY);
		len = read(fd, buffer, sizeof(buffer)-1);
		close(fd);
		buffer[len] = '\0';

#ifdef stiSTATS_MEMORY_METHOD_1
		/* Method 1 has a file (/proc/meminfo) that looks like the following.
		 * Note that the only line used to populate the structure is the
		 * second line that begins with "Mem:".  The values on that line are
		 * in bytes.
		 * 
		 *         total:     used:    free: shared: buffers:   cached:
		 * Mem:  29618176  25137152  4481024       0   253952  20881408
		 * Swap:        0         0        0
		 * MemTotal:         28924 kB
		 * ...
		 */
		
		// be prepared for extra columns to appear be seeking to ends of lines
		p = strchr(buffer, '\n');
		p = skip_token(p); // skip "Mem:" 
		
		memory_stats[0] = strtoul(p, &p, 10); 	
		memory_stats[1] = strtoul(p, &p, 10);
		memory_stats[2] = strtoul(p, &p, 10);
		memory_stats[3] = strtoul(p, &p, 10);
		memory_stats[4] = strtoul(p, &p, 10);
		memory_stats[5] = strtoul(p, &p, 10);
		
		memory_stats[0] = bytetok(memory_stats[0]);
		memory_stats[1] = bytetok(memory_stats[1]);
		memory_stats[2] = bytetok(memory_stats[2]);
		memory_stats[3] = bytetok(memory_stats[3]);
		memory_stats[4] = bytetok(memory_stats[4]);
		memory_stats[5] = bytetok(memory_stats[5]);

#elif defined stiSTATS_MEMORY_METHOD_2 
		/* Method 2 has a file (/proc/meminfo) that looks like the following.
		 * Note that we get all the values we are interested in from the first
		 * four lines.
		 * 
		 * MemTotal:   158904 kB
		 * MemFree:     94692 kB
		 * Buffers:         0 kB
		 * Cached:      39752 kB
		 * SwapCached:      0 kB
		 * ...
		 */
		
		p = skip_token(buffer); // skip "MemTotal:"
		memory_stats[0] = strtoul(p, &p, 10); // Read value
		p = strchr(p, '\n');  // skip the rest of this line

		p = skip_token(p); // skip "MemFree:"
		memory_stats[2] = strtoul(p, &p, 10); // Read value
		p = strchr(p, '\n');  // skip the rest of this line

		// Used = total - free
		memory_stats[1] = memory_stats[0] - memory_stats[2];

		// Set shared to 0, (not sure what values to look at)
		memory_stats[3] = 0;
		
		p = skip_token(p); // skip "Buffers:"
		memory_stats[4] = strtoul(p, &p, 10); // Read value
		p = strchr(p, '\n');  // skip the rest of this line

		p = skip_token(p); // skip "Cached:"
		memory_stats[5] = strtoul(p, &p, 10); // Read value
		p = strchr(p, '\n');  // skip the rest of this line
#else
		// Set to 0 until implemented for the given OS.
		memory_stats[0] = 0;
		memory_stats[1] = 0;
		memory_stats[2] = 0;
		memory_stats[3] = 0;
		memory_stats[4] = 0;
		memory_stats[5] = 0;
#endif
		
		pstInfo->memory = memory_stats;
	}
}

unsigned long ReadProcTime(int pid, int tid)
{
	char procFilename[256];
	char buffer[1024];
	int fd, num_read;
	long user = 0;
	long sys = 0;

	// Read /proc file
	sprintf(procFilename, "/proc/%d/task/%d/stat", pid, tid);
	fd = open(procFilename, O_RDONLY, 0);

	if (fd != -1)
	{
		num_read = read(fd, buffer, 1023);

		if (num_read > 0)
		{
			buffer[num_read] = '\0';

			// The 14th field is utime (User Time)
			char* pPtr = strrchr(buffer, ')') + 1;
			for (int i = 3; i != 14; ++i)
			{
				pPtr = strchr(pPtr + 1, ' ');
			}

			pPtr++;
			user = atol(pPtr);
			pPtr = strchr(pPtr,' ') + 1;
			sys = atol(pPtr);
		//    pPtr = strchr(pPtr,' ') + 1;
		//    long nice = atol(pPtr);
		//    pPtr = strchr(pPtr,' ') + 1;
		//    long idle = atol(pPtr);
		}

		close(fd);
	}

	// We're only concerned with user & sys time for now.
	return user + sys;// + nice + idle;
}

void stiSystemThreadInfoGet(SstiThreadUtilizationInfo *pstInfo)
{
	// Total CPU usage - NOTE: This assumes CPU info is up to date!
	int nCPU = cpu_diff[0] + cpu_diff[1] + cpu_diff[2] + cpu_diff[3];

	pid_t pid = getpid();

	// Read task time
	long nCurrent = ReadProcTime(pid, pstInfo->nTaskId);

	pstInfo->nTime = nCurrent;
	pstInfo->nPercentage = (nCurrent - pstInfo->nOldTime) * 100 / nCPU;
}


#endif //#ifdef stiLINUX
