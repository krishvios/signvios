// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#include "os/stiOS.h"
#include "os/stiOSNet.h"

#include <errno.h>

#include <netdb.h>	/* for hostent and gethostbyname */

static void 
ts_printf (const char *fmt, ...)
{
    time_t t = time(0);
    char *ts = ctime(&t);
    va_list argp;

    va_start(argp, fmt);
    if (ts) {
        char *c = strchr(ts, '\n');
        if (c) { 
            *c = 0;
        }
        printf("%s: ", ts);
    }
    vprintf(fmt, argp);
    va_end(argp);
}

EstiResult MacAddressTest ()
{
	EstiResult eResult;
	uint8_t aucMacAddress[6];
	
	printf ("Testing stiOSGetMACAddress MAC address...\n");

	/* Can we retrieve the MAC address? */
	eResult = stiOSGetMACAddress (aucMacAddress);
	if (estiOK == eResult)
	{
		/* Yes! Print the address we received. */
		printf ("\tSUCCESS!\n");
		printf ("\tMAC Address is: %02X:%02X:%02X:%02X:%02X:%02X\n",
			aucMacAddress[0], 
			aucMacAddress[1], 
			aucMacAddress[2],
			aucMacAddress[3], 
			aucMacAddress[4], 
			aucMacAddress[5]);
	} /* end if */
	else
	{
		printf ("\tFAILED to retrieve MAC address!\n");
	} /* end else */

	printf ("Done\n\n");

	return (eResult);
} /* end MacAddressTest */

EstiResult HostNameTest ()
{
	EstiResult eResult = estiOK;
	struct hostent * pstHostEnt;
	char szHost[] = "www.sorenson.com";
	char szBuff[48];

	printf ("Testing stiOSGetHostByName...\n");

	/* Can we retrieve the name? */
	printf ("\tResolving %s...\n", szHost);

	pstHostEnt = stiOSGetHostByName (szHost, pstHostEnt, szBuff, 48);
	if (NULL != pstHostEnt)
	{
		char szAddr[INET_ADDRSTRLEN];

		/* Yes! Print the address we received. */
		printf ("\tSUCCESS!\n");

		printf ("\t\th_name = %s\n", pstHostEnt->h_name);
		printf ("\t\th_aliases[0] = %s\n", pstHostEnt->h_aliases[0]);
		printf ("\t\th_addrtype = %d\n", pstHostEnt->h_addrtype);
		printf ("\t\th_length = %d\n", pstHostEnt->h_length);
		inet_ntop (AF_INET, pstHostEnt->h_addr_list[0], szAddr, sizeof (szAddr));
		printf ("\t\th_addr_list[0] = %s\n", szAddr);
	} /* end if */
	else
	{
		printf ("\tFAILED to retrieve host!\n");
		eResult = estiERROR;
	} /* end else */

	printf ("Done\n\n");

	return (eResult);
} /* end HostNameTest */

EstiResult MutexTest ()
{
	EstiResult eResult = estiOK;

	printf ("Testing Mutex...\n");

	stiMUTEX_ID muxID;
	
	muxID = stiOSMutexCreate ();

	if (NULL != muxID)
	{
		if (estiOK == stiOSMutexLock (muxID))
		{
			if (estiOK == stiOSMutexUnlock (muxID))
			{
				printf ("\tSUCCESS locking Mutex!\n");
			} /* end if */
			else
			{
				printf ("\tFAILED to unlock Mutex! errno = %d\n", errno);
				eResult = estiERROR;
			} /* end else */
		} /* end if */
		else
		{
			printf ("\tFAILED to lock Mutex! errno = %d\n", errno);
				eResult = estiERROR;
		} /* end else */

		if (estiOK == stiOSMutexDestroy (muxID))
		{
			muxID = NULL;

			printf ("\tMutex destroyed\n");
			printf ("\tTrying to lock destroyed mutex...\n");

			if (estiOK == stiOSMutexLock (muxID))
			{
				stiOSMutexUnlock (muxID);
				printf ("\t\tFAILED! Was able to lock destroyed Mutex!\n");
				eResult = estiERROR;
			} /* end if */
			else
			{
				printf ("\t\tSUCCESS! Was not able to lock destroyed Mutex!\n");
			} /* end else */

		} /* end if */
		else
		{
			printf ("\tFAILED to destroy Mutex! errno = %d\n", errno);
			eResult = estiERROR;
		} /* end else */
	} /* end if */
	else
	{
		printf ("\tFAILED to create Mutex! errno = %d\n", errno);
		eResult = estiERROR;
	} /* end else */

	printf ("Done\n\n");

	return (eResult);
} /* end MutextTest */

EstiResult BinarySemaphoreTest ()
{
	EstiResult eResult = estiOK;
	printf ("Testing Binary Semaphore...\n");

	stiSEM_ID semID;
	
	semID = stiOSSemBCreate (stiSEM_FULL);

	if (NULL != semID)
	{
		if (estiOK == stiOSSemTake (semID, stiWAIT_FOREVER))
		{
			if (estiOK == stiOSSemGive (semID))
			{
				printf ("\tSUCCESS taking/giving Semaphore!\n");
			} /* end if */
			else
			{
				printf ("\tFAILED to give Semaphore! errno = %d\n", errno);
				eResult = estiERROR;
			} /* end else */
		} /* end if */
		else
		{
			printf ("\tFAILED to take Semaphore! errno = %d\n", errno);
			eResult = estiERROR;
		} /* end else */

		if (estiOK == stiOSSemGive (semID))
		{
			if (estiOK == stiOSSemTake (semID, stiWAIT_FOREVER))
			{
				printf ("\tSUCCESS giving/taking Semaphore!\n");
			} /* end if */
			else
			{
				printf ("\tFAILED to Take Semaphore! errno = %d\n", errno);
				eResult = estiERROR;
			} /* end else */
		} /* end if */
		else
		{
			printf ("\tFAILED to Give Semaphore! errno = %d\n", errno);
			eResult = estiERROR;
		} /* end else */

		if (estiOK == stiOSSemDelete (semID))
		{
			semID = NULL;

			printf ("\tSemaphore deleted\n");
			printf ("\tTrying to take deleted Semaphore...\n");

			if (estiOK == stiOSSemTake (semID, stiWAIT_FOREVER))
			{
				stiOSSemGive (semID);
				printf ("\t\tFAILED! Was able to take destroyed Semaphore!\n");
				eResult = estiERROR;
			} /* end if */
			else
			{
				printf ("\t\tSUCCESS! Was not able to take destroyed Semaphore!\n");
			} /* end else */

			printf ("\tTrying to give deleted Semaphore...\n");

			if (estiOK == stiOSSemGive (semID))
			{
				printf ("\t\tFAILED! Was able to give destroyed Semaphore!\n");
				eResult = estiERROR;
			} /* end if */
			else
			{
				printf ("\t\tSUCCESS! Was not able to give destroyed Semaphore!\n");
			} /* end else */
		} /* end if */
		else
		{
			printf ("\tFAILED to destroy Semaphore! errno = %d\n", errno);
			eResult = estiERROR;
		} /* end else */
	} /* end if */
	else
	{
		printf ("\tFAILED to create Semaphore! errno = %d\n", errno);
		eResult = estiERROR;
	} /* end else */

	printf ("Done\n\n");

	return (eResult);
} /* end BinarySemaphoreTest */

EstiResult CountingSemaphoreTest ()
{
	EstiResult eResult = estiOK;

	printf ("Testing Counting Semaphore...\n");

	stiSEM_ID semID;
	
	semID = stiOSSemCCreate (0);

	if (NULL != semID)
	{
		if (estiOK == stiOSSemGive (semID))
		{
			if (estiOK == stiOSSemTake (semID, stiWAIT_FOREVER))
			{
				printf ("\tSUCCESS giving/taking Semaphore!\n");
			} /* end if */
			else
			{
				printf ("\tFAILED to Take Semaphore! errno = %d\n", errno);
				eResult = estiERROR;
			} /* end else */
		} /* end if */
		else
		{
			printf ("\tFAILED to Give Semaphore! errno = %d\n", errno);
			eResult = estiERROR;
		} /* end else */

		if (estiOK == stiOSSemGive (semID))
		{
			stiOSSemGive (semID);
			stiOSSemGive (semID);
			stiOSSemGive (semID);
			stiOSSemGive (semID);

			stiOSSemTake (semID, stiWAIT_FOREVER);
			stiOSSemTake (semID, stiWAIT_FOREVER);
			stiOSSemTake (semID, stiWAIT_FOREVER);
			stiOSSemTake (semID, stiWAIT_FOREVER);

			if (estiOK == stiOSSemTake (semID, stiWAIT_FOREVER))
			{
				printf ("\tSUCCESS giving (5) /taking (4) Semaphore!\n");
			} /* end if */
			else
			{
				printf ("\tFAILED to Take Semaphore! errno = %d\n", errno);
				eResult = estiERROR;
			} /* end else */
		} /* end if */
		else
		{
			printf ("\tFAILED to Give Semaphore! errno = %d\n", errno);
			eResult = estiERROR;
		} /* end else */

		if (estiOK == stiOSSemDelete (semID))
		{
			semID = NULL;

			printf ("\tSemaphore deleted\n");
			printf ("\tTrying to take deleted Semaphore...\n");

			if (estiOK == stiOSSemTake (semID, stiWAIT_FOREVER))
			{
				stiOSSemGive (semID);
				printf ("\t\tFAILED! Was able to take destroyed Semaphore!\n");
				eResult = estiERROR;
			} /* end if */
			else
			{
				printf ("\t\tSUCCESS! Was not able to take destroyed Semaphore!\n");
			} /* end else */

			printf ("\tTrying to give deleted Semaphore...\n");

			if (estiOK == stiOSSemGive (semID))
			{
				printf ("\t\tFAILED! Was able to give destroyed Semaphore!\n");
				eResult = estiERROR;
			} /* end if */
			else
			{
				printf ("\t\tSUCCESS! Was not able to give destroyed Semaphore!\n");
			} /* end else */
		} /* end if */
		else
		{
			printf ("\tFAILED to destroy Semaphore! errno = %d\n", errno);
			eResult = estiERROR;
		} /* end else */
	} /* end if */
	else
	{
		printf ("\tFAILED to create Semaphore! errno = %d\n", errno);
		eResult = estiERROR;
	} /* end else */

	printf ("Done\n\n");

	return (eResult);	
} /* end CountingSemaphoreTest */

static int g_nTaskFinished = 0;
int stiTaskProc (size_t pArg)
{
	printf ("\n\t***\n");
	printf ("\tstiTaskProc called with argument: %ld\n", pArg);

	printf ("\tSleeping for 1 second...\n");
	stiOSTaskDelay (stiOSLinuxSysClkRateGet ());
	printf ("\tAwake!\n");

	printf ("\t***\n");

	g_nTaskFinished = 1;
    
    return 0;
} /* end stiTaskProc */

EstiResult TaskTest ()
{
	EstiResult eResult = estiERROR;
	stiTaskID idTask;
	int x;

	printf ("Testing Task Spawn...\n");

	idTask = stiOSTaskSpawn ("TestTask", 0, 1000, stiTaskProc, (void *)0xACE);

	if (idTask != stiOS_TASK_INVALID_ID)
	{
		printf ("Task name is %s\n", stiOSTaskName (idTask));

		for (x = 0; (x < 10) && (g_nTaskFinished == 0); x++)
		{
			stiOSTaskDelay (stiOSLinuxSysClkRateGet () / 2);
		} /* end for */

		if (g_nTaskFinished)
		{
			eResult = estiOK;
		} /* end if */
		else
		{
			printf ("Timed out waiting for task. Killing task...\n\n");
			stiOSTaskDelete (idTask);
		} /* end else */
	} /* end if */

	printf ("Done\n\n");

	return (eResult);
} /* end TaskTest */

static int
stiTaskSemProc (size_t pArg)
{
	stiSEM_ID semID = (stiSEM_ID) pArg;

	ts_printf("task calling semTake (except 3 second delay)\n");
    if (estiOK == stiOSSemTake (semID, stiWAIT_FOREVER)) {
        ts_printf("task semTake succeeded\n");
    } else {
        ts_printf("**task semTake failed\n");
    }
    stiOSTaskDelay(stiOSLinuxSysClkRateGet() * 7);
    stiOSSemGive(semID);

	g_nTaskFinished = 1;
    
    return 0;
} /* end stiTaskProc */

static EstiResult 
TaskSemTest (void)
{
	EstiResult eResult = estiERROR;
	stiTaskID idTask;
	stiSEM_ID semID;
	int x;
	
	g_nTaskFinished = 0;
	semID = stiOSSemBCreate (stiSEM_EMPTY);
	if (NULL == semID) {
        ts_printf("Failed to create binary semaphore\n");
        return (estiERROR);
    }

	idTask = stiOSTaskSpawn ("SemTestTask", 0, 1000, stiTaskSemProc, semID);
	if (idTask == stiOS_TASK_INVALID_ID) {
		ts_printf ("Task spawn failed\n");
        return (estiERROR);
    }
    ts_printf("Task name is %s\n", stiOSTaskName (idTask));
    ts_printf("Main sleeping for 3 seconds\n");
    stiOSTaskDelay (stiOSLinuxSysClkRateGet () * 3);
    if (estiOK == stiOSSemGive (semID)) {
        ts_printf("Main semGive succeeded\n");
    } else {
        ts_printf("*** Main semGive failed\n");
        eResult = estiERROR;
    }

    stiOSTaskDelay (stiOSLinuxSysClkRateGet ());

    ts_printf("Main testing semTake with timeout\n");
    /* Expect this to fail. */
    if (estiOK == stiOSSemTake(semID, stiOSLinuxSysClkRateGet() * 3)) {
        ts_printf("***Main took semaphore, didn't expect to\n");
        eResult = estiERROR;
    } else {
        ts_printf("Main timed out (did 3 seconds elapse?)\n");
    }

    ts_printf("Main testing semTake with timeout\n");
    /* Expect this to succeed. */
    if (estiOK == stiOSSemTake(semID, stiOSLinuxSysClkRateGet() * 10)) {
        ts_printf("Main took semaphore with timeout\n");
    } else {
        ts_printf("***Main timed out\n");
        eResult = estiERROR;
    }

    for (x = 0; (x < 10) && (g_nTaskFinished == 0); x++) {
        stiOSTaskDelay (stiOSLinuxSysClkRateGet () / 2);
    } /* end for */

    if (g_nTaskFinished) {
        eResult = estiOK;
    } else {
        ts_printf ("Main Timed out waiting for task. Killing task...\n\n");
        stiOSTaskDelete(idTask);
    } 

	ts_printf("Main Done\n\n");
	return (eResult);
} /* end TaskSemTest */

#define NUM_WDOG_IDS 64
struct {
    int index;
    stiWDOG_ID id;
    int flags;
    EstiResult result;
    int callback_complete;
} wdog_param[NUM_WDOG_IDS];

static int
wdog_callback (int idx)
{
    if (!wdog_param[idx].flags) {
        ts_printf("** wdog_callback[%d]: inappropriate callback\n", idx);
        wdog_param[idx].result = estiERROR;
    } else {
        ts_printf("wdog_callback[%d]: success\n", idx);
        wdog_param[idx].result = estiOK;
    }
    wdog_param[idx].callback_complete = estiTRUE;
    return (0);
}

static EstiResult 
wdogTest (void)
{
	EstiResult eResult = estiERROR;
    int idx;
    int ticks_per_sec = stiOSLinuxSysClkRateGet();
    double expire[] = {
        0.25, 0.5, 0.75, 1.0,
        0.5, 0.75, 1.0, 0.25,
        0.75, 0.25, 1.0, 0.5,
        1.0, 0.25, 0.75, 0.5
    };
    double *p_expire = expire;

    for (idx = 0; idx < NUM_WDOG_IDS; idx++) {
        wdog_param[idx].index = idx;
        wdog_param[idx].flags = 0;
        wdog_param[idx].callback_complete = estiFALSE;
        wdog_param[idx].result = estiERROR;
        wdog_param[idx].id = stiOSWdCreate();
        if (!wdog_param[idx].id) {
            ts_printf("wdogTest: Failed to create timer %d\n", idx);
            return (estiERROR);
        }
    }

    /* 
     * Implementation uses a linked list.  Try adding/removing from
     * the list in various configurations.  Set the delay high
     * enough that they shouldn't fire during this exercise.
     */

/* Test 1: add to empty list, then remove */
    if (estiOK != stiOSWdStart(wdog_param[0].id, 1 * ticks_per_sec, 
                               (stiFUNC_PTR)wdog_callback, 0)) {
        ts_printf("wdogTest:1: Failed to start timer %d\n", 0);
        return (estiERROR);
    }
    if (estiOK != stiOSWdCancel(wdog_param[0].id)) {
        ts_printf("wdogTest:1: Failed to cancel timer %d\n", 0);
        return (estiERROR);
    }

/* Test 2: add three entries to empty list, then remove in same order */
    for (idx = 1; idx < 4; idx++) {
        if (estiOK != stiOSWdStart(wdog_param[idx].id, 1 * ticks_per_sec, 
                                   (stiFUNC_PTR)wdog_callback, idx)) {
            ts_printf("wdogTest:2: Failed to start timer %d\n", idx);
            return (estiERROR);
        }
    }
    for (idx = 1; idx < 4; idx++) {
        if (estiOK != stiOSWdCancel(wdog_param[idx].id)) {
            ts_printf("wdogTest:2: Failed to cancel timer %d\n", idx);
            return (estiERROR);
        }
    }

/* Test 3: add three entries to empty list, then remove in reverse order */
    for (idx = 5; idx < 8; idx++) {
        if (estiOK != stiOSWdStart(wdog_param[idx].id, 1 * ticks_per_sec, 
                                   (stiFUNC_PTR)wdog_callback, idx)) {
            ts_printf("wdogTest:3: Failed to start timer %d\n", idx);
            return (estiERROR);
        }
    }
    for (idx = 8; --idx >= 5; ) {
        if (estiOK != stiOSWdCancel(wdog_param[idx].id)) {
            ts_printf("wdogTest:3: Failed to cancel timer %d\n", idx);
            return (estiERROR);
        }
    }

/* Test 4: add four entries to empty list, expiring in various orders */
    for (idx = 8; idx < 24; idx++) {
        wdog_param[idx].flags = 1;
        if (estiOK != stiOSWdStart(wdog_param[idx].id, 
                                   (int) (*p_expire++ * ticks_per_sec), 
                                   (stiFUNC_PTR)wdog_callback, idx)) {
            ts_printf("wdogTest:4: Failed to start timer %d\n", idx);
            return (estiERROR);
        }
    }

	stiOSTaskDelay(5 * ticks_per_sec);

/* Ensure all appropriate callbacks were made */
    eResult = estiOK;
    for (idx = 0; idx < 24; idx++) {
        if (wdog_param[idx].flags) {  /* callback expected */
            if (wdog_param[idx].callback_complete) {  /* callback made */
                if (estiERROR == wdog_param[idx].result) {
                    ts_printf("*** wdogTest: callback failed timer %d\n", idx);
                    eResult = estiERROR;
                }
            } else {
                ts_printf("*** wdogTest: missing expected callback "
                          "for timer %d\n", idx);
                eResult = estiERROR;
            }
        } else {  /* no callback expected */
            if (wdog_param[idx].callback_complete) {  /* callback made */
                ts_printf("*** wdogTest: unexpected callback "
                          "for timer %d\n", idx);
                eResult = estiERROR;
            }
        }
    }
    return (eResult);
}

static int
stiTaskMsgProc (size_t pArg)
{
    stiMSG_Q_ID id = (stiMSG_Q_ID) pArg;
	stiOSTaskDelay(stiOSLinuxSysClkRateGet());
    ts_printf("msgThread: Sending message\n");
    if (estiOK != stiOSMsgQSend(id, "1234567890123456", 16, 0)) {
        ts_printf("*** msgTest: msgQSend failed\n");
    }
    return (0);
}

static int
stiTaskPipeProc (size_t pArg)
{
    STI_OS_PIPE_TYPE fd = (STI_OS_PIPE_TYPE) pArg;
	stiOSTaskDelay(2 * stiOSLinuxSysClkRateGet());
    ts_printf("pipeThread: Sending message\n");
    if (estiOK != stiOsPipeMsgSend(fd, (void*)"1234567890123456", 16)) {
        ts_printf("*** pipeTest: pipeMsgSend failed\n");
    }
    return (0);
}

static EstiResult 
msgTest (void)
{
    stiMSG_Q_ID id;
    int idx;
    int ticks_per_sec = stiOSLinuxSysClkRateGet();
    char msg[5][17] = {
        { "0000AAAAAAAAAAAA" }
       ,{ "0001BBBBBBBBBBBB" }
       ,{ "0002CCCCCCCCCCCC" }
       ,{ "0003DDDDDDDDDDDD" }
       ,{ "0004EEEEEEEEEEEE" }
    };
    
    id = stiOSMsgQCreate(4, 16);
    if (!id) {
        ts_printf("msgTest: Failed to create queue\n");
        return (estiERROR);
    }

    /* try to send a big message */
    if (estiERROR != stiOSMsgQSend(id, msg[0], 64, 0)) {
        ts_printf("*** msgTest: msgQSend should fail for large messages\n");
        return (estiERROR);
    }
    if (estiERROR != stiOSMsgQUrgentSend(id, msg[0], 64, 0)) {
        ts_printf("*** msgTest: msgQUrgentSend should fail for large messages\n");
        return (estiERROR);
    }

    /* Send some messages and verify expected queue depth. */
    for (idx = 0; idx < 4; idx++) {
        int n;
        if (estiOK != stiOSMsgQSend(id, msg[idx], 16, 0)) {
            ts_printf("*** msgTest: msgQSend failed for msg %d\n", idx);
            return (estiERROR);
        }
        n = stiOSMsgQNumMsgs(id);
        if ((idx + 1) != n) {
            ts_printf("*** msgTest: msgQNumMsgs returns %d; expected %d\n",
                      n, idx + 1);
            return (estiERROR);
        }
    }

    /* Try to send more messages than the queue is configured for. */
    if (estiERROR != stiOSMsgQSend(id, msg[0], 16, 0)) {
        ts_printf("*** msgTest: msgQSend should fail for too many msgs\n", idx);
        return (estiERROR);
    }

    /* Try to receive into a small buffer */
    {
        char buffer[8];
        if (stiOSMsgQReceive(id, buffer, sizeof(buffer), 0) == estiOK) {
            ts_printf("*** msgTest: stiOSMsgQReceive should fail for small"
                      "buffers\n");
            return (estiERROR);
        }
    }

    /* Receive the four messages and check integrity */
    for (idx = 0; idx < 4; idx++) {
        char buffer[16];
        EstiResult r = stiOSMsgQReceive(id, buffer, sizeof(buffer), 0);
        int32_t n;
        if (r != estiOK) {
            ts_printf("*** msgTest: stiOSMsgReceive failed on msg %d\n", idx);
            return (estiERROR);
        }
        if (memcmp(buffer, msg[idx], sizeof(buffer))) {
            ts_printf("*** msgTest: data integrity error on msg %d\n", idx);
            return (estiERROR);
        }
        n = stiOSMsgQNumMsgs(id);
        if ((3 - idx) != n) {
            ts_printf("*** msgTest: msgQNumMsgs returns %d; expected %d\n",
                      n, (3 - idx));
            return (estiERROR);
        }
    }

    /* Send some messages with priority */
    idx = 1;
    if (estiOK != stiOSMsgQSend(id, msg[idx], 16, 0)) {
        ts_printf("*** msgTest: msgQSend failed for msg %d\n", idx);
        return (estiERROR);
    }
    idx = 3;
    if (estiOK != stiOSMsgQSend(id, msg[idx], 16, 0)) {
        ts_printf("*** msgTest: msgQSend failed for msg %d\n", idx);
        return (estiERROR);
    }
    idx = 0;
    if (estiOK != stiOSMsgQUrgentSend(id, msg[idx], 16, 0)) {
        ts_printf("*** msgTest: msgQSend failed for msg %d\n", idx);
        return (estiERROR);
    }

    /* Expect to receive index 0 then index 1 */
    for (idx = 0; idx < 2; idx++) {
        char buffer[16];
        EstiResult r = stiOSMsgQReceive(id, buffer, sizeof(buffer), 0);
        if (r != estiOK) {
            ts_printf("*** msgTest: stiOSMsgReceive failed on msg %d\n", idx);
            return (estiERROR);
        }
        if (memcmp(buffer, msg[idx], sizeof(buffer))) {
            ts_printf("*** msgTest: data integrity error on msg %d\n", idx);
            return (estiERROR);
        }
    }

    /* This should be put before the existing message 3 in the q */
    idx = 2;
    if (estiOK != stiOSMsgQUrgentSend(id, msg[idx], 16, 0)) {
        ts_printf("*** msgTest: msgQSend failed for msg %d\n", idx);
        return (estiERROR);
    }

    /* Expect to receive index 2 then index 3 */
    for (idx = 2; idx < 4; idx++) {
        char buffer[16];
        EstiResult r = stiOSMsgQReceive(id, buffer, sizeof(buffer), 0);
        if (r != estiOK) {
            ts_printf("*** msgTest: stiOSMsgReceive failed on msg %d\n", idx);
            return (estiERROR);
        }
        if (memcmp(buffer, msg[idx], sizeof(buffer))) {
            ts_printf("*** msgTest: data integrity error on msg %d\n", idx);
            return (estiERROR);
        }
    }

    /* Try to receive a non-existent message, but with a delay */
    {
        char buffer[16];
        EstiResult r;

        ts_printf("msgTest: testing stiOSMsgQReceive with timeout\n");
        r = stiOSMsgQReceive(id, buffer, sizeof(buffer), 2 * ticks_per_sec);
        if (r != estiOK) {
            ts_printf("msgTest: stiOSMsgReceive correctly returned "
                      "with no msg (after 2 seconds?)\n");
        } else {
            ts_printf("** msgTest: stiOSMsgReceive unexpectedly returned "
                      "a msg\n");
            return (estiERROR);
        }
    }

    /* 
     * Try to receive a non-existent message, but with a delay
     * This time we'll have another thread actually send a message.
     */

    {
        stiTaskID idTask;
        idTask = stiOSTaskSpawn ("MsgThread", 0, 1000, stiTaskMsgProc, id );
        if (idTask == stiOS_TASK_INVALID_ID) {
            ts_printf ("Task spawn failed\n");
            return (estiERROR);
        }
    }
    {
        char buffer[16];
        EstiResult r;

        ts_printf("msgTest: testing stiOSMsgQReceive with timeout\n");
        r = stiOSMsgQReceive(id, buffer, sizeof(buffer), 5 * ticks_per_sec);
        if (r != estiOK) {
            ts_printf("*** msgTest: stiOSMsgReceive failed\n");
            return (estiERROR);
        } else {
            ts_printf("msgTest: stiOSMsgReceive succeeded\n");
        }
    }

    return (estiOK);
}

static EstiResult 
pipeTest (void)
{
	EstiResult eResult = estiERROR;
    STI_OS_PIPE_TYPE fd[2];
    int os_fd[2];
    int idx;
    char msg[5][17] = {
        { "0000AAAAAAAAAAAA" }
       ,{ "0001BBBBBBBBBBBB" }
       ,{ "0002CCCCCCCCCCCC" }
       ,{ "0003DDDDDDDDDDDD" }
       ,{ "0004EEEEEEEEEEEE" }
    };
    
    /* Open up the two pipes */
    ts_printf("pipeTest: Opening pipes\n");
    eResult = stiOsPipeCreate(4, 16, &fd[0]);
    if (eResult != estiOK) {
        ts_printf("pipeTest: stiOsPipeOpen failed for /tmp/p1\n");
        return (estiERROR);
    }
    eResult = stiOsPipeCreate(4, 16, &fd[1]);
    if (eResult != estiOK) {
        ts_printf("pipeTest: stiOsPipeOpen failed for /tmp/p2\n");
        return (estiERROR);
    }
    os_fd[0] = stiOsPipeDescriptor(fd[0]);
    os_fd[1] = stiOsPipeDescriptor(fd[1]);
    if ((os_fd[0] < 0) || (os_fd[1] < 0)) {
        ts_printf("pipeTest: stiOsPipeDescriptor failed\n");
        return (estiERROR);
    }

    /* try to send a big message */
    ts_printf("pipeTest: Sending message through pipe\n");
    if (estiERROR != stiOsPipeMsgSend(fd[0], msg[0], 64)) {
        ts_printf("pipeTest: stiOsPipeMsgSend should fail "
                  "for large messages\n");
        // return (estiERROR);  // expected failure for Linux
    }

    /* Send some messages and verify expected queue depth. */
    ts_printf("pipeTest: Verifing queue depths\n");
    for (idx = 0; idx < 4; idx++) {
        if (estiOK != stiOsPipeMsgSend(fd[0], msg[idx], 16)) {
            ts_printf("*** pipeTest: stiOsPipeMsgSend failed for msg %d\n", idx);
            return (estiERROR);
        }
    }

    /* Try to receive into a small buffer */
    ts_printf("pipeTest: Verifing small buffer check\n");
    {
        char buffer[8];
        if (stiOsPipeMsgRecv(fd[0], buffer, sizeof(buffer)) == estiOK) {
            ts_printf("*** pipeTest: stiOsPipeMsgRecv should fail for small"
                      "buffers\n");
            return (estiERROR);
        }
    }

    /* Receive the four messages and check integrity */
    ts_printf("pipeTest: Verifing message integrity\n");
    for (idx = 0; idx < 4; idx++) {
        char buffer[16];
        EstiResult r = stiOsPipeMsgRecv(fd[0], buffer, sizeof(buffer));
        if (r != estiOK) {
            ts_printf("*** pipeTest: stiOsPipeMsgRecv failed on msg %d\n", idx);
            return (estiERROR);
        }
        if (memcmp(buffer, msg[idx], sizeof(buffer))) {
            ts_printf("*** pipeTest: data integrity error on msg %d\n", idx);
            return (estiERROR);
        }
    }

    ts_printf("pipeTest: Checking select behaviour\n");
    {
        struct timeval tv_zero = { 0, 0 };
        struct timeval tv = { 0, 0 };
        fd_set read_set;
        int n = ((os_fd[0] > os_fd[1]) ? os_fd[0] : os_fd[1]) + 1;
        int os_r;
        EstiResult r;
        char buffer[16];

        FD_ZERO(&read_set);
        FD_SET(os_fd[0], &read_set);
        FD_SET(os_fd[1], &read_set);
        tv = tv_zero;
        os_r = select(n, &read_set, NULL, NULL, &tv);
        if (os_r < 0) {
            ts_printf("pipeTest: 1. select failed with %s\n", strerror(errno));
            return (estiERROR);
        } else if (os_r) {
            ts_printf("pipeTest: 1. select succeeded but expected to fail\n");
            ts_printf("          %u descriptors readable a%u b%u\n",
                       FD_ISSET(os_fd[0], &read_set),
                       FD_ISSET(os_fd[1], &read_set));
            return (estiERROR);
        } else {
            ts_printf("pipeTest: 1. select on empty pipe succeeded\n");
        }

        /* Put a message in one pipe */
        if (estiOK != stiOsPipeMsgSend(fd[0], msg[0], 16)) {
            ts_printf("*** pipeTest: stiOsPipeMsgSend failed\n");
            return (estiERROR);
        }

        FD_ZERO(&read_set);
        FD_SET(os_fd[0], &read_set);
        FD_SET(os_fd[1], &read_set);
        tv = tv_zero;
        os_r = select(n, &read_set, NULL, NULL, &tv);
        if (os_r < 0) {
            ts_printf("pipeTest: 2. select failed with %s\n", strerror(errno));
            return (estiERROR);
        } else if (os_r != 1) {
            ts_printf("pipeTest: 2. select expected a1\n");
            ts_printf("          %u descriptors readable a%u b%u\n",
                       FD_ISSET(os_fd[0], &read_set),
                       FD_ISSET(os_fd[1], &read_set));
            return (estiERROR);
        } else {
            ts_printf("pipeTest: 2. select on non-empty pipe succeeded\n");
        }
        r = stiOsPipeMsgRecv(fd[0], buffer, sizeof(buffer));
        if (r != estiOK) {
            ts_printf("pipeTest: msgRecv failed\n");
            return (estiERROR);
        }

        /* Verify pipe is no longer selectable */

        FD_ZERO(&read_set);
        FD_SET(os_fd[0], &read_set);
        FD_SET(os_fd[1], &read_set);
        tv = tv_zero;
        os_r = select(n, &read_set, NULL, NULL, &tv);
        if (os_r < 0) {
            ts_printf("pipeTest: 3. select failed with %s\n", strerror(errno));
            return (estiERROR);
        } else if (os_r) {
            ts_printf("pipeTest: 3. select succeeded but expected to fail\n");
            ts_printf("          %u descriptors readable a%u b%u\n",
                       FD_ISSET(os_fd[0], &read_set),
                       FD_ISSET(os_fd[1], &read_set));
            return (estiERROR);
        } else {
            ts_printf("pipeTest: 3. select on empty pipe succeeded\n");
        }


        for (idx = 0; idx < 1; idx++) {
            if (estiOK != stiOsPipeMsgSend(fd[0], msg[idx], 16)) {
                ts_printf("*** pipeTest: stiOsPipeMsgSend failed for msg %d\n", idx);
                return (estiERROR);
            }
        }

        /* 
         * Verify it is selectable for each of two messages, and not
         * selectable when empty.  
         */
        for (idx = 0; idx < 1; idx++) {
            FD_ZERO(&read_set);
            FD_SET(os_fd[0], &read_set);
            FD_SET(os_fd[1], &read_set);
            tv = tv_zero;
            os_r = select(n, &read_set, NULL, NULL, &tv);
            if (os_r < 0) {
                ts_printf("pipeTest: 4. select failed with %s\n", strerror(errno));
                return (estiERROR);
            } else if (os_r != 1) {
                ts_printf("pipeTest: 4. select expected a1\n");
                ts_printf("          %u descriptors readable a%u b%u\n",
                           FD_ISSET(os_fd[0], &read_set),
                           FD_ISSET(os_fd[1], &read_set));
                return (estiERROR);
            } else {
                ts_printf("pipeTest: 4. select on non-empty pipe succeeded\n");
            }
            r = stiOsPipeMsgRecv(fd[0], buffer, sizeof(buffer));
            if (r != estiOK) {
                ts_printf("pipeTest: msgRecv failed\n");
                return (estiERROR);
            }
        }

        /* Verify pipe is no longer selectable */

        FD_ZERO(&read_set);
        FD_SET(os_fd[0], &read_set);
        FD_SET(os_fd[1], &read_set);
        tv = tv_zero;
        os_r = select(n, &read_set, NULL, NULL, &tv);
        if (os_r < 0) {
            ts_printf("pipeTest: 5. select failed with %s\n", strerror(errno));
            return (estiERROR);
        } else if (os_r) {
            ts_printf("pipeTest: 5. select succeeded but expected to fail\n");
            ts_printf("          %u descriptors readable a%u b%u\n",
                       FD_ISSET(os_fd[0], &read_set),
                       FD_ISSET(os_fd[1], &read_set));
            return (estiERROR);
        } else {
            ts_printf("pipeTest: 5. select on empty pipe succeeded\n");
        }

        /* 
         * Try to select with timeout on an empty pipe.
         * Have another thread actually send a message.
         */

        {
            stiTaskID idTask;
            idTask = stiOSTaskSpawn ("PipeThread", 0, 1000, 
                                     stiTaskPipeProc, fd[0] );
            if (idTask == stiOS_TASK_INVALID_ID) {
                ts_printf ("Task spawn failed\n");
                return (estiERROR);
            }
        }

        FD_ZERO(&read_set);
        FD_SET(os_fd[0], &read_set);
        FD_SET(os_fd[1], &read_set);
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        os_r = select(n, &read_set, NULL, NULL, &tv);
        if (os_r < 0) {
            ts_printf("pipeTest: 6. select failed with %s\n", strerror(errno));
            return (estiERROR);
        } else if (os_r != 1) {
            ts_printf("pipeTest: 6. select expected a1\n");
            ts_printf("          %u descriptors readable a%u b%u\n",
                       FD_ISSET(os_fd[0], &read_set),
                       FD_ISSET(os_fd[1], &read_set));
            return (estiERROR);
        } else {
            ts_printf("pipeTest: 6. select with timeout succeeded\n");
        }
        r = stiOsPipeMsgRecv(fd[0], buffer, sizeof(buffer));
        if (r != estiOK) {
            ts_printf("pipeTest: msgRecv failed\n");
            return (estiERROR);
        }
    }

    if (estiOK != stiOsPipeClose(&fd[0])) {
        ts_printf("*** pipeTest: failed to close first pipe\n");
        return (estiERROR);
    }
    if (estiOK != stiOsPipeClose(&fd[1])) {
        ts_printf("*** pipeTest: failed to close second pipe\n");
        return (estiERROR);
    }
    
    return (estiOK);
}

int main (int argc, char * argv)
{
	EstiResult eResult = estiOK;

	printf ("Testing stiOS Library...\n\n");

	if (eResult == estiOK)
	{
		eResult = MacAddressTest ();
	} /* end if */

	if (eResult == estiOK)
	{
		eResult = HostNameTest ();
	} /* end if */

	if (eResult == estiOK)
	{
		eResult = MutexTest ();
	} /* end if */

	if (eResult == estiOK)
	{
		eResult = BinarySemaphoreTest ();
	} /* end if */

	if (eResult == estiOK)
	{
		eResult = CountingSemaphoreTest ();
	} /* end if */

	if (eResult == estiOK)
	{
		eResult = TaskTest ();
	} /* end if */

    if (estiOK == eResult) {
        eResult = TaskSemTest();
    }
    if (estiOK == eResult) {
        eResult = wdogTest();
    }
    if (estiOK == eResult) {
        eResult = msgTest();
    }
    if (estiOK == eResult) {
        eResult = pipeTest();
    }

	if (estiOK == eResult)
	{
		printf ("Finished without errors.\n");
	} /* end if */
	else
	{
		printf ("ERROR! Could not complete tests!\n");
	} /* end else */

	return (0);
} /* end main */

