#include <../../nrnconf.h>
/*LINTLIBRARY*/
/* /local/src/master/nrn/src/memacs/lock.c,v 1.1.1.1 1994/10/12 17:21:24 hines Exp */
/*
lock.c,v
 * Revision 1.1.1.1  1994/10/12  17:21:24  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.2  89/07/10  10:25:45  mlh
 * LINT free
 * 
 * Revision 1.1  89/07/08  15:36:38  mlh
 * Initial revision
 * 
*/

/*	LOCK:	File locking command routines for MicroEMACS
		written by Daniel Lawrence
								*/

#include <stdio.h>
#include "estruct.h"
#include "edef.h"

#if	FILOCK
#if	V7 & BSD
#include <sys/errno.h>

extern int sys_nerr;		/* number of system error messages defined */
extern char *sys_errlist[];	/* list of message texts */
extern int errno;		/* current error */

char *lname[NLOCKS];	/* names of all locked files */
int numlocks;		/* # of current locks active */

/* lockchk:	check a file for locking and add it to the list */

int lockchk(fname)

char *fname;	/* file to check for a lock */

{
	register int i;		/* loop indexes */
	register int status;	/* return status */
	char *undolock();

	/* check to see if that file is already locked here */
	if (numlocks > 0)
		for (i=0; i < numlocks; ++i)
			if (strcmp(fname, lname[i]) == 0)
				return(TRUE);

	/* if we have a full locking table, bitch and leave */
	if (numlocks == NLOCKS) {
		mlwrite("LOCK ERROR: Lock table full");
		return(ABORT);
	}

	/* next, try to lock it */
	status = lock(fname);
	if (status == ABORT)	/* file is locked, no override */
		return(ABORT);
	if (status == FALSE)	/* locked, overriden, dont add to table */
		return(TRUE);

	/* we have now locked it, add it to our table */
	lname[++numlocks - 1] = (char *)malloc(strlen(fname) + 1);
	if (lname[numlocks - 1] == NULL) {	/* malloc failure */
		undolock(fname);		/* free the lock */
		mlwrite("Cannot lock, out of memory");
		--numlocks;
		return(ABORT);
	}

	/* everthing is cool, add it to the table */
	Strcpy(lname[numlocks-1], fname);
	return(TRUE);
}

/*	lockrel:	release all the file locks so others may edit */

int lockrel()

{
	register int i;		/* loop index */
	register int status;	/* status of locks */
	register int s;		/* status of one unlock */

	status = TRUE;
	if (numlocks > 0)
		for (i=0; i < numlocks; ++i) {
			if ((s = unlock(lname[i])) != TRUE)
				status = s;
			free(lname[i]);
		}
	numlocks = 0;
	return(status);
}

/* lock:	Check and lock a file from access by others
		returns	TRUE = files was not locked and now is
			FALSE = file was locked and overridden
			ABORT = file was locked, abort command
*/

int lock(fname)

char *fname;	/* file name to lock */

{
	register char *locker;	/* lock error message */
	register int status;	/* return status */
	char msg[NSTRING];	/* message string */
	char *dolock();

	/* attempt to lock the file */
	locker = dolock(fname);
	if (locker == NULL)	/* we win */
		return(TRUE);

	/* file failed...abort */
	if (strncmp(locker, "LOCK", 4) == 0) {
		lckerror(locker);
		return(ABORT);
	}

	/* someone else has it....override? */
	Strcpy(msg, "File in use by ");
	Strcat(msg, locker);
	Strcat(msg, ", overide?");
	status = mlyesno(msg);		/* ask them */
	if (status == TRUE)
		return(FALSE);
	else
		return(ABORT);
}

/*	unlock:	Unlock a file
		this only warns the user if it fails
							*/

int unlock(fname)

char *fname;	/* file to unlock */

{
	register char *locker;	/* undolock return string */
	char *undolock();

	/* unclock and return */
	locker = undolock(fname);
	if (locker == NULL)
		return(TRUE);

	/* report the error and come back */
	lckerror(locker);
	return(FALSE);
}

int lckerror(errstr)	/* report a lock error */

char *errstr;		/* lock error string to print out */

{
	char obuf[NSTRING];	/* output buffer for error message */

	Strcpy(obuf, errstr);
	Strcat(obuf, " - ");
	if (errno < sys_nerr)
		Strcat(obuf, sys_errlist[errno]);
	else
		Strcat(obuf, "[can not get system error message]");
	mlwrite(obuf);
}
#endif
#else
int lckhello()	/* dummy function */
{
  return TRUE;
}
#endif
