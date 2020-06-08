#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

#define SEM_KEY_PATH "/dev/null"
#define SEM_KEY_ID 100
#define READ_CLEARANCE 0777 // TODO rimuovere
#define MAX_INIT_PLAYER_MONEY 500
#define MIN_INIT_PLAYER_MONEY 200
#define ARRSIZE(x) (sizeof(x) / sizeof(x[0]))
#define MINIMUM_BET 30
#define MSG_QUEUE_SIZE 64
#define MSG_TYPE_USER_MATCH 1
#define MAX_DEFAULT_PLAYERS 3
#define EXCHANGE "euro"

// msg structure
typedef struct msgbuf
{
    long mtype;
    char mtext[MSG_QUEUE_SIZE];
} message_buf;
//semaphore obj
typedef union {
    int val;
    struct semid_ds *buf;
    short *array;
} Semun;
// player action type
typedef enum actionType
{
    INIT = 0,
    WELCOME = 1,
    BET = 2,
    PLAY = 3,
    LEAVE = 4
} PlayerActionType;
typedef struct pdata
{
    unsigned int dataId;               // indice dati
    int pid;                           // pid del giocatore
    unsigned int semNum;               // numero semaforo del giocatore
    char playerName[30];               // nome giocatore
    unsigned int startingMoney;        // soldi iniziali
    unsigned int currentMoney;         // soldi correnti
    unsigned int currentBet;           // soldi scommessi in questa giocata
    unsigned int currentBetPercentage; // percentuale soldi scommessi in questa giocata rispetto a quelli correnti
    unsigned int firstDiceResult;      // risultato primo dado di questa giocata
    unsigned int secondDiceResult;     // risultato secondo dado di questa giocata
    unsigned int totalDiceResult;      // risultato totale dadi di questa giocata
    unsigned int finalMoney;           // soldi finali
    unsigned int playedGamesCount;     // partite giocate
    unsigned int winnedGamesCount;     // partite vinte
    unsigned int losedGamesCount;      // partite perse
} PlayerData;
typedef struct gdata
{
    unsigned int croupierPid;                    // pid del banco
    unsigned int croupierSemNum;                 // numero semaforo del banco
    unsigned int croupierStartingMoney;          // soldi iniziali
    unsigned int croupierCurrentMoney;           // soldi correnti
    unsigned int totalPlayedGamesCount;          // partite giocate
    unsigned int winnedGamesCount;               // partite vinte
    unsigned int losedGamesCount;                // partite perse
    unsigned int playersCount;                   // contatore giocatori
    PlayerData playersData[MAX_DEFAULT_PLAYERS]; // contiene i dati di tutti i giocatori
    PlayerActionType actionType;                 // definisce il tipo di azione che deve fare il giocatore
} GameData;

void printTitle()
{
    printf("\n");
    printf("**************************************\n");
    printf("****     BENVENUTI AL CASINO'     ****\n");
    printf("**************************************\n");
}

void throwException(char *message)
{
    fprintf(stderr, "\n--Execution error (PID %d): %s - %s (code %d)--\n", getpid(), message, strerror(errno), errno);
    exit(errno);
}

key_t getKey()
{
    key_t key = ftok(SEM_KEY_PATH, SEM_KEY_ID);
    if (key == (key_t)-1)
    {
        throwException("Cannot retrieve key: error during ftok generation!");
    }
    return key;
}

unsigned int randomValue(int randomSeed, int rangeFrom, int rangeTo)
{
    srand((unsigned int)randomSeed * time(NULL));
    return (rand() % (rangeTo - rangeFrom + 1)) + rangeFrom;
}

void allocateShm(GameData *gameData)
{
    // CERCARE MEMORIA TODO
    // SE ESISTE GIA' DEALLOCARLA
    // ALLOCCARLA
    key_t shmKey = getKey();
    int shmId = shmget(shmKey, sizeof(GameData), IPC_CREAT | READ_CLEARANCE); // TEMPORANEO ***************
    if (shmId == -1)
    {
        throwException("Error during shm allocation!");
    }
    errno = 0;
    GameData *shmP = (GameData *)shmat(shmId, 0, 0);
    if (errno != 0)
    {
        char *errorMessage;
        switch (errno)
        {
        case EACCES:
            errorMessage = "The calling process does not have the required permissions for the requested attach type, and does not have the CAP_IPC_OWNER capability.";
            break;
        case EIDRM:
            errorMessage = "shmid points to a removed identifier.";
            break;
        case EINVAL:
            errorMessage = "Invalid shmid value, unaligned (i.e., not page-aligned and SHM_RND was not specified) or invalid shmaddr value, or can't attach segment at shmaddr, or SHM_REMAP was specified and shmaddr was NULL.";
            break;
        case ENOMEM:
            errorMessage = "Could not allocate memory for the descriptor or for the page tables.";
            break;
        default:
            errorMessage = "no error message defined.";
        }
        char *message = "Cannot get game data! Error during shmat! ";
        char *res = (char *)malloc(sizeof(*errorMessage) + sizeof(*message));
        strcpy(res, errorMessage);
        strcat(res, message);
        throwException(message);
    }
    memcpy(shmP, gameData, sizeof(GameData));
}

int getShmId()
{
    key_t shmKey = getKey();

    int shmId = shmget(shmKey, sizeof(GameData), READ_CLEARANCE);
    if (shmId == -1)
    {
        throwException("Error during shm id retrieving!");
    }
    return shmId;
}

GameData *getGameData()
{
    // CERCARE MEMORIA TODO
    // SE ESISTE GIA' DEALLOCARLA
    // ALLOCCARLA
    int shmId = getShmId();
    errno = 0;
    GameData *p = (GameData *)shmat(shmId, 0, 0);
    if (errno != 0)
    {
        char *errorMessage;
        switch (errno)
        {
        case EACCES:
            errorMessage = "The calling process does not have the required permissions for the requested attach type, and does not have the CAP_IPC_OWNER capability.";
            break;
        case EIDRM:
            errorMessage = "shmid points to a removed identifier.";
            break;
        case EINVAL:
            errorMessage = "Invalid shmid value, unaligned (i.e., not page-aligned and SHM_RND was not specified) or invalid shmaddr value, or can't attach segment at shmaddr, or SHM_REMAP was specified and shmaddr was NULL.";
            break;
        case ENOMEM:
            errorMessage = "Could not allocate memory for the descriptor or for the page tables.";
            break;
        default:
            errorMessage = "no error message defined.";
            break;
        }
        char *message = "Cannot get game data! Error during shmat! ";
        char *res = (char *)malloc(sizeof(*errorMessage) + sizeof(*message));
        strcpy(res, errorMessage);
        strcat(res, message);
        throwException(message);
    }
    // GameData *res = (GameData *)malloc(sizeof(GameData));
    // memcpy(res, p, sizeof(GameData));
    // return res;
    return p;
}

void unallocateShm(bool skipLog)
{
    /// TODO .... usare anche SHMDT prima del semctl per svuotare la memoria
    //detach from shared memory
    //shmdt(str);
    int semId = getShmId();
    if (shmctl(semId, IPC_RMID, 0) == -1 && skipLog == false)
    {
        throwException("Error during shm unallocation! Shm not deleted!");
    }
}

void allocateSem(int playersCount)
{
    // int semId = semget(semkey, playersCount, IPC_CREAT | IPC_EXCL);
    // if (semId == -1)
    // {
    //     if (errno != EEXIST)
    //     {
    // throwException("Error during sem allocation!");
    //     }
    //     unallocateSem();
    //     semId = semget(semkey, playersCount, IPC_CREAT);
    //      /// CHECK
    // }
    key_t semKey = getKey();
    int semRes = semget(semKey, playersCount, IPC_CREAT | READ_CLEARANCE); // TEMPORANEO ***************
    if (semRes == -1)
    {
        throwException("Error during sem allocation!");
    }
}

int getSemId()
{
    key_t semKey = getKey();
    int semId = semget(semKey, 0, READ_CLEARANCE);
    if (semId == -1)
    {
        throwException("Error during sem id retrieving!");
    }
    return semId;
}

void setSem(unsigned int semNum, int semId, int incremental)
{
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = incremental;
    sops.sem_flg = SEM_UNDO;
    errno = 0;
    if (semop(semId, &sops, 1) == -1)
    {
        char *errorMessage;
        switch (errno)
        {
        case E2BIG:
            errorMessage = "The value nsops is greater than the system limit.";
            break;
        case EACCES:
            errorMessage = "Operation permission is denied to the calling process. Read access is required when sem_op is zero. Write access is required when sem_op is not zero.";
            break;
        case EAGAIN:
            errorMessage = "The operation would result in suspension of the calling process but IPC_NOWAIT in sem_flg was specified.";
            break;
        case EFBIG:
            errorMessage = "sem_num is less than zero or greater or equal to the number of semaphores in the set specified on in semget() argument nsems.";
            break;
        case EIDRM:
            errorMessage = "semid was removed from the system while the invoker was waiting.";
            break;
        case EINTR:
            errorMessage = "semop() was interrupted by a signal.";
            break;
        case EINVAL:
            errorMessage = "The value of argument semid is not a valid semaphore identifier. For an __IPC_BINSEM semaphore set, the sem_op is other than +1 for a sem_val of 0 or -1 for a sem_val of 0 or 1. Also, for an __IPC_BINSEM semaphore set, the number of semaphore operations is greater than one.";
            break;
        case ENOSPC:
            errorMessage = "The limit on the number of individual processes requesting a SEM_UNDO would be exceeded.";
            break;
        case ERANGE:
            errorMessage = "An operation would cause semval or semadj to overflow the system limit as defined in <sys/sem.h>.";
            break;
        default:
            errorMessage = "no error message defined.";
            break;
        }
        char *message = "Cannot setSem: ";
        char *res = (char *)malloc(sizeof(*errorMessage) + sizeof(*message));
        strcpy(res, message);
        strcat(res, errorMessage);
        throwException(res);
    }
}

void unallocateSem(bool skipLog)
{
    int semId = getSemId();
    int semDeleteResult = semctl(semId, 0, IPC_RMID, 0);
    if (semDeleteResult == -1 && skipLog == false)
    {
        throwException("Error during sem unallocation! Shm not deleted!");
    }
}

int allocateMsgQueue()
{
    key_t msqKey = getKey();
    int msqId = msgget(msqKey, IPC_CREAT | READ_CLEARANCE);
    if (msqId == -1)
    {
        throwException("allocateMsgQueue");
    }
    return msqId;
}

int getMsgQueueId(bool skipEEXISTError)
{
    key_t msqKey = getKey();
    int msqId = msgget(msqKey, IPC_CREAT | READ_CLEARANCE);
    if (msqId == -1 && (skipEEXISTError != true || errno != EEXIST))
    {
        throwException("getMsgQueueId");
    }
    return msqId;
}

int sendMsgQueue(int msgType, char *message)
{
    message_buf msgBuf;
    msgBuf.mtype = msgType;
    sprintf(msgBuf.mtext, message);

    int msqId = getMsgQueueId(false);
    int msgsndRes = msgsnd(msqId, &msgBuf, sizeof(message_buf), 0);
    if (msgsndRes < 0)
    {
        throwException("sendMsgQueue");
    }
}

void receiveMsgQueue(int msgType, char *receivedMessage)
{
    message_buf msgBuf;
    int msqId = getMsgQueueId(false);
    int msgRcvRes = msgrcv(msqId, &msgBuf, MSG_QUEUE_SIZE, msgType, READ_CLEARANCE);
    if (msgRcvRes == -1)
    {
        throwException("receiveMsgQueue");
    }
    sprintf(receivedMessage, msgBuf.mtext);
}

void unallocateMsgQueue()
{
    int msqId = getMsgQueueId(false);
    int deleteRes = msgctl(msqId, IPC_RMID, NULL);
    if (deleteRes == -1)
    {
        perror("unallocateMsgQueue");
        exit(-1);
    }
}

void endProgram(int i)
{
    if ((int)getppid() == 1) // main process as parent pid = 1
    {
        unallocateShm(1);
        unallocateSem(1);
    }
    exit(0);
}

void randomSort(char *array[], size_t n, size_t size)
{
    char tmp[size];
    char *arr = (char *)array;
    size_t stride = size * sizeof(char);
    if (n > 1)
    {
        // shuffle
        size_t i;
        for (i = 0; i < n - 1; ++i)
        {
            srand((unsigned int)i * time(NULL));
            size_t rnd = (size_t)rand();
            size_t j = i + rnd / (RAND_MAX / (n - i) + 1);

            memcpy(tmp, arr + j * stride, size);
            memcpy(arr + j * stride, arr + i * stride, size);
            memcpy(arr + i * stride, tmp, size);
        }
        // reverse
        char *tmp[n];
        for (int i = n - 1; i >= 0; i--)
        {
            tmp[n - 1 - i] = array[i];
        }
        for (int i = 0; i < n; i++)
        {
            array[i] = tmp[i];
        }
    }
}

void pausePlayer(unsigned int semNum, int semId)
{
    setSem(semNum, semId, -1);
}
