#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/resource.h>

/* Define semaphores to be placed in a single semaphore set */
/* Numbers indicate index in semaphore set for named semaphore */
#define SEM_COWSINGROUP 0
#define SEM_PCOWSINGROUP 1
#define SEM_COWSWAITING 7
#define SEM_PCOWSEATEN 11
#define SEM_COWSEATEN 12
#define SEM_COWSDEAD 16

#warning TODO_EDIT_NUMBER
#define SEM_SHEEPINGROUP 0
#define SEM_PSHEEPINGROUP 1
#define SEM_SHEEPWAITING 7
#define SEM_PSHEEPEATEN 11
#define SEM_SHEEPEATEN 12
#define SEM_SHEEPDEAD 16

#define SEM_PTERMINATE 17
#define SEM_DRAGONEATING 19
#define SEM_DRAGONFIGHTING 20
#define SEM_DRAGONSLEEPING 21
#define SEM_PMEALWAITINGFLAG 23

/* System constants used to control simulation termination */
#define MAX_SHEEP_EATEN 36
#define MAX_SHEEP_CREATED 240
#define MAX_COWS_EATEN 12
#define MAX_COWS_CREATED 80
#define MAX_HUNTERS_DEFEATED 48
#define MAX_THIEVES_DEFEATED 36
#define MAX_TREASURE_IN_HOARD 1000
#define INITIAL_TREASURE_IN_HOARD 500

#warning TODO_COWS_NUMBER_4_INITIAL
/* System constants to specify size of groups of cows*/
#define COWS_IN_GROUP 1
#define SHEEP_IN_GROUP 3

/* CREATING YOUR SEMAPHORES */
int semID;

union semun seminfo;
//{
//	int val;
//	struct semid_ds *buf;
//	ushort *array;
//} seminfo;

struct timeval startTime;

/*  Pointers and ids for shared memory segments */
int *terminateFlagp = NULL;
int *cowCounterp = NULL;
int *cowsEatenCounterp = NULL;
int *sheepCounterp = NULL;
int *sheepEatenCounterp = NULL;
int *mealWaitingFlagp = NULL;
int terminateFlag = 0;
int cowCounter = 0;
int cowsEatenCounter = 0;
int sheepCounter = 0;
int sheepEatenCounter = 0;
int mealWaitingFlag = 0;

/* Group IDs for managing/removing processes */
int smaugProcessID = -1;
int cowProcessGID = -1;
int sheepProcessGID = -1;
int parentProcessGID = -1;

/* Define the semaphore operations for each semaphore */
/* Arguments of each definition are: */
/* Name of semaphore on which the operation is done */
/* Increment (amount added to the semaphore when operation executes*/
/* Flag values (block when semaphore <0, enable undo ...)*/

/*Number in group semaphores*/
struct sembuf WaitCowsInGroup = {SEM_COWSINGROUP, -1, 0};
struct sembuf SignalCowsInGroup = {SEM_COWSINGROUP, 1, 0};
struct sembuf WaitSheepInGroup = {SEM_SHEEPINGROUP, -1, 0};
struct sembuf SignalSheepInGroup = {SEM_SHEEPINGROUP, 1, 0};

/*Number in group mutexes*/
struct sembuf WaitProtectCowsInGroup = {SEM_PCOWSINGROUP, -1, 0};
struct sembuf WaitProtectSheepInGroup = {SEM_PSHEEPINGROUP, -1, 0};
struct sembuf WaitProtectMealWaitingFlag = {SEM_PMEALWAITINGFLAG, -1, 0};
struct sembuf SignalProtectCowsInGroup = {SEM_PCOWSINGROUP, 1, 0};
struct sembuf SignalProtectSheepInGroup = {SEM_PSHEEPINGROUP, 1, 0};
struct sembuf SignalProtectMealWaitingFlag = {SEM_PMEALWAITINGFLAG, 1, 0};

/*Number waiting sempahores*/
struct sembuf WaitCowsWaiting = {SEM_COWSWAITING, -1, 0};
struct sembuf SignalCowsWaiting = {SEM_COWSWAITING, 1, 0};
struct sembuf WaitSheepWaiting = {SEM_SHEEPWAITING, -1, 0};
struct sembuf SignalSheepWaiting = {SEM_SHEEPWAITING, 1, 0};

/*Number eaten or fought semaphores*/
struct sembuf WaitCowsEaten = {SEM_COWSEATEN, -1, 0};
struct sembuf SignalCowsEaten = {SEM_COWSEATEN, 1, 0};
struct sembuf WaitSheepEaten = {SEM_SHEEPEATEN, -1, 0};
struct sembuf SignalSheepEaten = {SEM_SHEEPEATEN, 1, 0};

/*Number eaten or fought mutexes*/
struct sembuf WaitProtectCowsEaten = {SEM_PCOWSEATEN, -1, 0};
struct sembuf SignalProtectCowsEaten = {SEM_PCOWSEATEN, 1, 0};
struct sembuf WaitProtectSheepEaten = {SEM_PSHEEPEATEN, -1, 0};
struct sembuf SignalProtectSheepEaten = {SEM_PSHEEPEATEN, 1, 0};

/*Number Dead semaphores*/
struct sembuf WaitCowsDead = {SEM_COWSDEAD, -1, 0};
struct sembuf SignalCowsDead = {SEM_COWSDEAD, 1, 0};
struct sembuf WaitSheepDead = {SEM_SHEEPDEAD, -1, 0};
struct sembuf SignalSheepDead = {SEM_SHEEPDEAD, 1, 0};

/*Dragon Semaphores*/
struct sembuf WaitDragonEating = {SEM_DRAGONEATING, -1, 0};
struct sembuf SignalDragonEating = {SEM_DRAGONEATING, 1, 0};
struct sembuf WaitDragonFighting = {SEM_DRAGONFIGHTING, -1, 0};
struct sembuf SignalDragonFighting = {SEM_DRAGONFIGHTING, 1, 0};
struct sembuf WaitDragonSleeping = {SEM_DRAGONSLEEPING, -1, 0};
struct sembuf SignalDragonSleeping = {SEM_DRAGONSLEEPING, 1, 0};

/*Termination Mutex*/
struct sembuf WaitProtectTerminate = {SEM_PTERMINATE, -1, 0};
struct sembuf SignalProtectTerminate = {SEM_PTERMINATE, 1, 0};

double timeChange(struct timeval starttime);
void initialize();
void smaug();
void cow(int startTimeN);
void sheep(int startTimeN);
void terminateSimulation();
void releaseSemandMem();
void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something);
void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo);

int main() {
	//	int k;
	//	int temp;

	/* variables to hold process ID numbers */
	int parentPID = 0;
	int cowPID = 0;
    int sheepPID = 0;
	int smaugPID = 0;

	/* local counters, keep track of total number */
	/* of processes of each type created */
	int cowsCreated = 0;
    int sheepCreated = 0;

	/* Variables to control the time between the arrivals */
	/* of successive beasts*/
	//	double minwait = 0;
	int newSeed = 0;
	int sleepingTime = 0;
	int maxCowIntervalUsec = 0;
    int maxSheepIntervalUsec = 0;
	int nextInterval = 0.0;
	int status;
	int w = 0;
	double maxCowInterval = 0.0;
	double totalCowInterval = 0.0;
    double maxSheepInterval = 0.0;
    double totalSheepInterval = 0.0;
	double elapsedTime;
	//	double hold;

	parentPID = getpid();
	setpgid(parentPID, parentPID);
	parentProcessGID = getpgid(0);
	printf("CRCRCRCRCRCRCRCRCRCRCRCR  main process group  %d %d\n", parentPID, parentProcessGID);

	/* initialize semaphores and allocate shared memory */
	initialize();

	/* inialize each variable in shared memory */
	*cowCounterp = 0;
	*cowsEatenCounterp = 0;
    *sheepCounterp = 0;
	*sheepEatenCounterp = 0;
	*mealWaitingFlagp = 0;

	printf("Please enter a random seed to start the simulation\n");
	scanf("%d", &newSeed);
	srand(newSeed);

	printf("Please enter the maximum interval length for cow (ms)\n");
	scanf("%lf", &maxCowInterval);
    printf("max Cow interval time %f \n", maxCowInterval);
    maxCowIntervalUsec = (int)maxCowInterval * 1000;

    printf("Please enter the maximum interval length for sheep (ms)\n");
	scanf("%lf", &maxSheepInterval);
    printf("max Sheep interval time %f \n", maxSheepInterval);
    maxSheepIntervalUsec = (int)maxSheepInterval * 1000;

	gettimeofday(&startTime, NULL);

	if ((smaugPID = fork()) == 0) {
		printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug is born\n");
		smaug();
		printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug dies\n");
		exit(0);
	} else {
		if (smaugProcessID == -1) {
			smaugProcessID = smaugPID;
		}
		setpgid(smaugPID, smaugProcessID);
		printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug PID %8d PGID %8d\n", smaugPID,
					 smaugProcessID);
	}

	printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug PID  create cow %8d \n", smaugPID);
	usleep(10);

	while (TRUE) {
		semopChecked(semID, &WaitProtectTerminate, 1);
		if (*terminateFlagp != 0) {
			semopChecked(semID, &SignalProtectTerminate, 1);
			break;
		}
		semopChecked(semID, &SignalProtectTerminate, 1);

		/* Create a cow process if needed */
		/* The condition used to determine if a process is needed is */
		/* if the last cow created will be enchanted */
		elapsedTime = timeChange(startTime);
		if (totalCowInterval - elapsedTime < totalCowInterval) {
			nextInterval = (int)((double)rand() / RAND_MAX * maxCowIntervalUsec);
			totalCowInterval += nextInterval / 1000.0;
			sleepingTime = (int)((double)rand() / RAND_MAX * maxCowIntervalUsec);
			if ((cowPID = fork()) == 0) {
				/* Child becomes a beast */
				elapsedTime = timeChange(startTime);
				cow(sleepingTime);
				/* Child (beast) quits after being consumed */
				exit(0);
			} else if (cowPID > 0) {
				cowsCreated++;
				if (cowProcessGID == -1) {
					cowProcessGID = cowPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  cow PGID %8d \n", cowPID,
								 cowProcessGID);
				}
				setpgid(cowPID, cowProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   NEW COW CREATED %8d \n", cowsCreated);
			} else {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR cow process not created \n");
				continue;
			}
			if (cowsCreated == MAX_COWS_CREATED) {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR Exceeded maximum number of cows\n");
				*terminateFlagp = 1;
				break;
			}
		}

        /* Create a sheep process if needed */
		/* The condition used to determine if a process is needed is */
		/* if the last sheep created will be enchanted */
		elapsedTime = timeChange(startTime);
		if (totalSheepInterval - elapsedTime < totalSheepInterval) {
			nextInterval = (int)((double)rand() / RAND_MAX * maxSheepIntervalUsec);
			totalCowInterval += nextInterval / 1000.0;
			sleepingTime = (int)((double)rand() / RAND_MAX * maxSheepIntervalUsec);
			if ((sheepPID = fork()) == 0) {
				/* Child becomes a beast */
				elapsedTime = timeChange(startTime);
				sheep(sleepingTime);
				/* Child (beast) quits after being consumed */
				exit(0);
			} else if (sheepPID > 0) {
				sheepCreated++;
				if (sheepProcessGID == -1) {
					sheepProcessGID = sheepPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  sheep PGID %8d \n", sheepPID, sheepProcessGID);
				}
				setpgid(sheepPID, sheepProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   NEW SHEEP CREATED %8d \n", sheepCreated);
			} else {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR sheep process not created \n");
				continue;
			}
			if (sheepCreated == MAX_COWS_CREATED) {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR Exceeded maximum number of sheep\n");
				*terminateFlagp = 1;
				break;
			}
		}

		/* wait for processes that have exited so we do not accumulate zombie or cows */
		while ((w = waitpid(-1, &status, WNOHANG)) > 1) {
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) > 0) {
					printf("exited, status=%8d\n", WEXITSTATUS(status));
					terminateSimulation();
					printf("GOODBYE from main process %8d\n", getpid());
					exit(0);
				} else {
					printf("                           REAPED process %8d\n", w);
				}
			}
		}

		/* terminateFlagp is set to 1 (from initial value of 0) when any */
		/* termination condition is satisfied (max sheep eaten ... ) */
		if (*terminateFlagp == 1)
			break;

		/* sleep for 80% of the cow interval */
		/* then try making more processes */
		usleep((totalCowInterval * 800));
	}
	if (*terminateFlagp == 1) {
		terminateSimulation();
	}
	printf("GOODBYE from main process %8d\n", getpid());
	exit(0);
}

void smaug() {
	int k;
	//	int temp;
	int localpid;
	//	double elapsedTime;

	/* local counters used only for smaug routine */
	int cowsEatenTotal = 0;
    int sheepEatenTotal = 0;

	/* Initialize random number generator*/
	/* Random numbers are used to determine the time between successive beasts */
	smaugProcessID = getpid();
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   PID is %d \n", smaugProcessID);
	localpid = smaugProcessID;
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has gone to sleep\n");
	semopChecked(semID, &WaitDragonSleeping, 1);
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has woken up \n");
	while (TRUE) {
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		while (*mealWaitingFlagp >= 1) {
			*mealWaitingFlagp = *mealWaitingFlagp - 1;
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   signal meal flag %d\n", *mealWaitingFlagp);
			semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is eating a meal\n");
			for (k = 0; k < COWS_IN_GROUP; k++) {
				semopChecked(semID, &SignalCowsWaiting, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   A cow is ready to eat\n");
			}
            for (k = 0; k < SHEEP_IN_GROUP; k++) {
				semopChecked(semID, &SignalSheepWaiting, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   A sheep is ready to eat\n");
			}

			/*Smaug waits to eat*/
			semopChecked(semID, &WaitDragonEating, 1);
			for (k = 0; k < COWS_IN_GROUP; k++) {
				semopChecked(semID, &SignalCowsDead, 1);
				cowsEatenTotal++;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating a cow\n");
			}
            for (k = 0; k < SHEEP_IN_GROUP; k++) {
				semopChecked(semID, &SignalSheepDead, 1);
				sheepEatenTotal++;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating a sheep\n");
			}
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has finished a meal\n");
			if (cowsEatenTotal >= MAX_COWS_EATEN) {
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has eaten the allowed number of cows\n");
				*terminateFlagp = 1;
				break;
			}
            if (sheepEatenTotal >= MAX_SHEEP_EATEN) {
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has eaten the allowed number of sheep\n");
				*terminateFlagp = 1;
				break;
			}

			/* Smaug check to see if another snack is waiting */
			semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
			if (*mealWaitingFlagp > 0) {
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug eats again\n"); //, localpid);
				continue;
			} else {
				semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug sleeps again\n"); //, localpid);
				semopChecked(semID, &WaitDragonSleeping, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is awake again\n"); //, localpid);
				break;
			}
		}
	}
}

void initialize() {
	/* Init semaphores */
	semID = semget(IPC_PRIVATE, 25, 0666 | IPC_CREAT);

	/* Init to zero, no elements are produced yet */
	seminfo.val = 0;
	semctlChecked(semID, SEM_COWSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSDEAD, SETVAL, seminfo);

    semctlChecked(semID, SEM_SHEEPINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPDEAD, SETVAL, seminfo);

	semctlChecked(semID, SEM_DRAGONFIGHTING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONSLEEPING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONEATING, SETVAL, seminfo);
	printf("!!INIT!!INIT!!INIT!!  semaphores initiialized\n");

	/* Init Mutex to one */
	seminfo.val = 1;
	semctlChecked(semID, SEM_PCOWSINGROUP, SETVAL, seminfo);
    semctlChecked(semID, SEM_PSHEEPINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_PMEALWAITINGFLAG, SETVAL, seminfo);
	semctlChecked(semID, SEM_PCOWSEATEN, SETVAL, seminfo);
    semctlChecked(semID, SEM_PSHEEPEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_PTERMINATE, SETVAL, seminfo);
	printf("!!INIT!!INIT!!INIT!!  mutexes initiialized\n");

	/* Now we create and attach  the segments of shared memory*/
	if ((terminateFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for terminateFlag\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm created for terminateFlag\n");
	}
	if ((cowCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowCounter\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowCounter\n");
	}
    if ((sheepCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepCounter\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepCounter\n");
	}
	if ((mealWaitingFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) <
			0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for mealWaitingFlag\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm created for mealWaitingFlag\n");
	}
	if ((cowsEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) <
			0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowsEatenCounter\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowsEatenCounter\n");
	}
    if ((sheepEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) <
        0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepEatenCounter\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepEatenCounter\n");
	}

	/* Now we attach the segment to our data space.  */
	if ((terminateFlagp = shmat(terminateFlag, NULL, 0)) == (int *)-1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for terminateFlag\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for terminateFlag\n");
	}

	if ((cowCounterp = shmat(cowCounter, NULL, 0)) == (int *)-1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowCounter\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowCounter\n");
	}
    if ((sheepCounterp = shmat(sheepCounter, NULL, 0)) == (int *)-1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepCounter\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepCounter\n");
	}
	if ((mealWaitingFlagp = shmat(mealWaitingFlag, NULL, 0)) == (int *)-1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for mealWaitingFlag\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for mealWaitingFlag\n");
	}
	if ((cowsEatenCounterp = shmat(cowsEatenCounter, NULL, 0)) == (int *)-1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowsEatenCounter\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowsEatenCounter\n");
	}
    if ((sheepEatenCounterp = shmat(sheepEatenCounter, NULL, 0)) == (int *)-1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepEatenCounter\n");
		exit(1);
	} else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepEatenCounter\n");
	}
	printf("!!INIT!!INIT!!INIT!!   initialize end\n");
}

void cow(int startTimeN) {
	int localpid;
	//	int retval;
	int k;
	localpid = getpid();

	/* graze */
	printf("CCCCCCC %8d CCCCCCC   A cow is born\n", localpid);
	if (startTimeN > 0) {
		if (usleep(startTimeN) == -1) {
			/* exit when usleep interrupted by kill signal */
			if (errno == EINTR)
				exit(4);
		}
	}
	printf("CCCCCCC %8d CCCCCCC   cow grazes for %f ms\n", localpid,
				 startTimeN / 1000.0);

	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsInGroup, 1);
	semopChecked(semID, &SignalCowsInGroup, 1);
	*cowCounterp = *cowCounterp + 1;
	printf("CCCCCCC %8d CCCCCCC   %d  cows have been enchanted \n", localpid,
				 *cowCounterp);
	if ((*cowCounterp >= COWS_IN_GROUP)) {
		*cowCounterp = *cowCounterp - COWS_IN_GROUP;
		for (k = 0; k < COWS_IN_GROUP; k++) {
			semopChecked(semID, &WaitCowsInGroup, 1);
		}
		printf("CCCCCCC %8d CCCCCCC   The last cow is waiting\n", localpid);
		semopChecked(semID, &SignalProtectCowsInGroup, 1);
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		*mealWaitingFlagp = *mealWaitingFlagp + 1;
		printf("CCCCCCC %8d CCCCCCC   signal meal flag %d\n", localpid,
					 *mealWaitingFlagp);
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		semopChecked(semID, &SignalDragonSleeping, 1);
		printf("CCCCCCC %8d CCCCCCC   last cow  wakes the dragon \n", localpid);
	} else {
		semopChecked(semID, &SignalProtectCowsInGroup, 1);
	}

	semopChecked(semID, &WaitCowsWaiting, 1);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsEaten, 1);
	semopChecked(semID, &SignalCowsEaten, 1);
	*cowsEatenCounterp = *cowsEatenCounterp + 1;
	if ((*cowsEatenCounterp >= COWS_IN_GROUP)) {
		*cowsEatenCounterp = *cowsEatenCounterp - COWS_IN_GROUP;
		for (k = 0; k < COWS_IN_GROUP; k++) {
			semopChecked(semID, &WaitCowsEaten, 1);
		}
		printf("CCCCCCC %8d CCCCCCC   The last cow has been eaten\n", localpid);
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		semopChecked(semID, &SignalDragonEating, 1);
	} else {
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		printf("CCCCCCC %8d CCCCCCC   A cow is waiting to be eaten\n", localpid);
	}
	semopChecked(semID, &WaitCowsDead, 1);

	printf("CCCCCCC %8d CCCCCCC   cow  dies\n", localpid);
}

void sheep(int startTimeN) {
	int localpid;
	int k;
	localpid = getpid();

	/* graze */
	printf("SSSSSSS %8d SSSSSSS   A sheep is born\n", localpid);
	if (startTimeN > 0) {
		if (usleep(startTimeN) == -1) {
			/* exit when usleep interrupted by kill signal */
			if (errno == EINTR)
				exit(4);
		}
	}
	printf("SSSSSSS %8d SSSSSSS   sheep grazes for %f ms\n", localpid, startTimeN / 1000.0);

	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectSheepInGroup, 1);
	semopChecked(semID, &SignalSheepInGroup, 1);
	*sheepCounterp = *sheepCounterp + 1;
	printf("SSSSSSS %8d SSSSSSS   %d  sheep have been enchanted \n", localpid, *sheepCounterp);
	if ((*sheepCounterp >= SHEEP_IN_GROUP)) {
		*sheepCounterp = *sheepCounterp - SHEEP_IN_GROUP;
		for (k = 0; k < SHEEP_IN_GROUP; k++) {
			semopChecked(semID, &WaitSheepInGroup, 1);
		}
		printf("SSSSSSS %8d SSSSSSS   The last sheep is waiting\n", localpid);
		semopChecked(semID, &SignalProtectSheepInGroup, 1);
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		*mealWaitingFlagp = *mealWaitingFlagp + 1;
#warning TODO_CHECK_IF_MEAL_READY
		printf("SSSSSSS %8d SSSSSSS   signal meal flag %d\n", localpid,
               *mealWaitingFlagp);
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		semopChecked(semID, &SignalDragonSleeping, 1);
		printf("SSSSSSS %8d SSSSSSS   last sheep  wakes the dragon \n", localpid);
	} else {
		semopChecked(semID, &SignalProtectSheepInGroup, 1);
	}

	semopChecked(semID, &WaitSheepWaiting, 1);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectSheepEaten, 1);
	semopChecked(semID, &SignalSheepEaten, 1);
	*sheepEatenCounterp = *sheepEatenCounterp + 1;
	if ((*sheepEatenCounterp >= SHEEP_IN_GROUP)) {
		*sheepEatenCounterp = *sheepEatenCounterp - SHEEP_IN_GROUP;
		for (k = 0; k < SHEEP_IN_GROUP; k++) {
			semopChecked(semID, &WaitSheepEaten, 1);
		}
		printf("SSSSSSS %8d SSSSSSS   The last sheep has been eaten\n", localpid);
		semopChecked(semID, &SignalProtectSheepEaten, 1);
		semopChecked(semID, &SignalDragonEating, 1);
	} else {
		semopChecked(semID, &SignalProtectSheepEaten, 1);
		printf("SSSSSSS %8d SSSSSSS   A sheep is waiting to be eaten\n", localpid);
	}
	semopChecked(semID, &WaitSheepDead, 1);

	printf("SSSSSSS %8d SSSSSSS  sheep dies\n", localpid);
}

void terminateSimulation() {
	pid_t localpgid;
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();
	printf("RELEASESEMAPHORES   Terminating Simulation from process %8d\n", localpgid);
	if (cowProcessGID != (int)localpgid) {
		if (killpg(cowProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   COWS NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed cows \n");
	}
    if (sheepProcessGID != (int)localpgid) {
		if (killpg(sheepProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   SHEEP NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed sheep \n");
	}
	if (smaugProcessID != (int)localpgid) {
		kill(smaugProcessID, SIGKILL);
		printf("XXTERMINATETERMINATE   killed smaug\n");
	}
	while ((w = waitpid(-1, &status, WNOHANG)) > 1) {
		printf("                           REAPED process in terminate %d\n", w);
	}
	releaseSemandMem();
	printf("GOODBYE from terminate\n");
}

void releaseSemandMem() {
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();

	// should check return values for clean termination
	semctl(semID, 0, IPC_RMID, seminfo);

	// wait for the semaphores
	usleep(2000);
	while ((w = waitpid(-1, &status, WNOHANG)) > 1) {
		printf("                           REAPED process in terminate %d\n", w);
	}
	printf("\n");
	if (shmdt(terminateFlagp) == -1) {
		printf("RELEASERELEASERELEAS   terminateFlag share memory detach failed\n");
	} else {
		printf("RELEASERELEASERELEAS   terminateFlag share memory detached\n");
	}
	if (shmctl(terminateFlag, IPC_RMID, NULL)) {
		printf("RELEASERELEASERELEAS   share memory delete failed %d\n", *terminateFlagp);
	} else {
		printf("RELEASERELEASERELEAS   share memory deleted\n");
	}
	if (shmdt(cowCounterp) == -1) {
		printf("RELEASERELEASERELEAS   cowCounterp memory detach failed\n");
	} else {
		printf("RELEASERELEASERELEAS   cowCounterp memory detached\n");
	}
	if (shmctl(cowCounter, IPC_RMID, NULL)) {
		printf("RELEASERELEASERELEAS   cowCounter memory delete failed \n");
	} else {
		printf("RELEASERELEASERELEAS   cowCounter memory deleted\n");
	}
    if (shmdt(sheepCounterp) == -1) {
		printf("RELEASERELEASERELEAS   sheepCounterp memory detach failed\n");
	} else {
		printf("RELEASERELEASERELEAS   sheepCounterp memory detached\n");
	}
	if (shmctl(sheepCounter, IPC_RMID, NULL)) {
		printf("RELEASERELEASERELEAS   sheepCounter memory delete failed \n");
	} else {
		printf("RELEASERELEASERELEAS   sheepCounter memory deleted\n");
	}
	if (shmdt(mealWaitingFlagp) == -1) {
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detach failed\n");
	} else {
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detached\n");
	}
	if (shmctl(mealWaitingFlag, IPC_RMID, NULL)) {
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory delete failed \n");
	} else {
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory deleted\n");
	}
	if (shmdt(cowsEatenCounterp) == -1) {
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detach failed\n");
	} else {
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detached\n");
	}
	if (shmctl(cowsEatenCounter, IPC_RMID, NULL)) {
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory delete failed \n");
	} else {
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory deleted\n");
	}
    if (shmdt(sheepEatenCounterp) == -1) {
		printf("RELEASERELEASERELEAS   sheepEatenCounterp memory detach failed\n");
	} else {
		printf("RELEASERELEASERELEAS   sheepEatenCounterp memory detached\n");
	}
	if (shmctl(sheepEatenCounter, IPC_RMID, NULL)) {
		printf("RELEASERELEASERELEAS   sheepEatenCounter memory delete failed \n");
	} else {
		printf("RELEASERELEASERELEAS   sheepEatenCounter memory deleted\n");
	}
}

void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo) {
	/* wrapper that checks if the semaphore control request has terminated */
	/* successfully. If it has not the entire simulation is terminated */

	if (semctl(semaphoreID, semNum, flag, seminfo) == -1) {
		if (errno != EIDRM) {
			printf("semaphore control failed: simulation terminating\n");
			printf("errno %8d \n", errno);
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		} else {
			exit(3);
		}
	}
}

void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something) {
	/* wrapper that checks if the semaphore operation request has terminated */
	/* successfully. If it has not the entire simulation is terminated */
	if (semop(semaphoreID, operation, something) == -1) {
		if (errno != EIDRM) {
			printf("semaphore operation failed: simulation terminating\n");
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		} else {
			exit(3);
		}
	}
}

double timeChange(const struct timeval startTime) {
	struct timeval nowTime;
	double elapsedTime;

	gettimeofday(&nowTime, NULL);
	elapsedTime = (nowTime.tv_sec - startTime.tv_sec) * 1000.0;
	elapsedTime += (nowTime.tv_usec - startTime.tv_usec) / 1000.0;
	return elapsedTime;
}
