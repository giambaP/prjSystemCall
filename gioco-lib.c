/*
    Matricola:              VR380626               
    Nome e cognome:         Giambattista Pomari
    Data di realizzazione:  13/06/2020 
    Titolo esercizio:       Secondo elaborato - UNIX System Call
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

#define KEY_PATH "/dev/null"
#define KEY_ID 100
#define READ_WRITE_CLEARANCE 0660
#define CROUPIER_SEM_NUM 0
#define MIN_INIT_PLAYER_MONEY 100
#define MAX_INIT_PLAYER_MONEY 300
#define ARRSIZE(x) (sizeof(x) / sizeof(x[0]))
#define MSG_QUEUE_SIZE 128
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
// player status
typedef enum playerStatus
{
    TO_CONNECT = 0,
    CONNECTED = 1,
    PLAYING = 2,
    TO_DISCONNECT = 3
} PlayerStatus;
// player action type
typedef enum actionType
{
    INIT = 0,
    WELCOME = 1,
    BET = 2,
    PLAY = 3,
    LEAVE = 4
} PlayerActionType;
// player last round
typedef enum roundResultType
{
    NONE = 0,
    WINNED = 1,
    LOSED = 2
} RoundResultType;
typedef struct pdata
{
    unsigned int dataId;             // indice dati
    pid_t pid;                       // pid del giocatore
    unsigned int semNum;             // numero semaforo del giocatore
    char playerName[30];             // nome giocatore
    PlayerStatus playerStatus;       // definisce lo stato del giocatore
    int startingMoney;               // soldi iniziali
    int currentMoney;                // soldi correnti
    unsigned int currentBet;         // soldi scommessi in questa giocata
    float currentBetPercentage;      // percentuale soldi scommessi in questa giocata rispetto a quelli correnti
    RoundResultType lastRoundResult; // definisce lo stato dell'ultima giocata: vinta o persa
    unsigned int firstDiceResult;    // risultato primo dado di questa giocata
    unsigned int secondDiceResult;   // risultato secondo dado di questa giocata
    unsigned int totalDiceResult;    // risultato totale dadi di questa giocata
    unsigned int playedGamesCount;   // partite giocate
    unsigned int winnedGamesCount;   // partite vinte
    unsigned int losedGamesCount;    // partite perse
} PlayerData;
typedef struct gdata
{
    pid_t croupierPid;                           // pid del banco
    unsigned int croupierSemNum;                 // numero semaforo del banco
    int croupierStartingMoney;                   // soldi iniziali
    int croupierCurrentMoney;                    // soldi correnti
    unsigned int totalPlayedGamesCount;          // partite giocate
    unsigned int winnedGamesCount;               // partite vinte
    unsigned int losedGamesCount;                // partite perse
    PlayerData playersData[MAX_DEFAULT_PLAYERS]; // contiene i dati di tutti i giocatori
    PlayerActionType actionType;                 // definisce il tipo di azione che deve fare il giocatore
} GameData;

void throwException(char *message)
{
    fprintf(stderr, "\n--Execution error (PID %d): %s - %s (code %d)--\n", getpid(), message, strerror(errno), errno);
    exit(errno);
}

key_t getKey()
{
    key_t key = ftok(KEY_PATH, KEY_ID);
    if (key == (key_t)-1)
    {
        throwException("Cannot retrieve key: error during ftok generation!");
    }
    return key;
}

unsigned int randomValue(int seed, int rangeFrom, int rangeTo)
{
    int randValue = rand() * seed;
    randValue = randValue > 0 ? randValue : rand();
    return (randValue % (rangeTo - rangeFrom + 1)) + rangeFrom;
}

int getShmId(bool skipErrorLog)
{
    key_t shmKey = getKey();

    int shmId = shmget(shmKey, sizeof(GameData), READ_WRITE_CLEARANCE);
    if (shmId == -1 && skipErrorLog == false)
    {
        throwException("Error during shm id retrieving!");
    }
    return shmId;
}

GameData *getGameData(bool skipErrorLog)
{
    int shmId = getShmId(skipErrorLog);
    if (shmId == -1 && skipErrorLog == true)
    {
        return (GameData *)-1;
    }
    errno = 0;
    GameData *p = (GameData *)shmat(shmId, 0, READ_WRITE_CLEARANCE);
    if (p == (void *)-1 && skipErrorLog == false)
    {
        char *message = "Cannot get game data! Error during shmat! ";
        throwException(message);
    }
    return p;
}

void unallocateShm(bool skipErrorLog)
{
    //detach from shared memory
    GameData *gameData = getGameData(true);
    if (((long)gameData) != -1)
    {
        int shmdtRes = shmdt(gameData);
        if (shmdtRes == -1)
        {
            throwException("Error during shm detaching! Shm not detached!");
        }
    }
    // unllocating data
    int semId = getShmId(skipErrorLog);
    if (shmctl(semId, IPC_RMID, 0) == -1 && skipErrorLog == false)
    {
        throwException("Error during shm unallocation! Shm not deleted!");
    }
}

void allocateShm(GameData *gameData)
{
    key_t shmKey = getKey();
    // unallocate if exists
    int shmId = shmget(shmKey, sizeof(GameData), IPC_EXCL);
    if (shmId != -1)
    {
        unallocateShm(false);
    }
    // allocating it
    shmId = shmget(shmKey, sizeof(GameData), IPC_CREAT | READ_WRITE_CLEARANCE);
    if (shmId == -1)
    {
        throwException("Error during shm allocation!");
    }
    errno = 0;
    // allocating game data
    GameData *shmP = (GameData *)shmat(shmId, 0, READ_WRITE_CLEARANCE);
    if (shmP == (void *)-1)
    {
        throwException("Cannot allocate game data! Error during shmat! ");
    }
    memcpy(shmP, gameData, sizeof(GameData));
}

int getSemId()
{
    key_t semKey = getKey();
    int semId = semget(semKey, 0, READ_WRITE_CLEARANCE);
    if (semId == -1)
    {
        throwException("Error during sem id retrieving!");
    }
    return semId;
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

void allocateSem(int playersCount)
{
    key_t semKey = getKey();
    // unallocate if exists
    int semRes = semget(semKey, playersCount, IPC_EXCL);
    if (semRes != -1)
    {
        unallocateSem(false);
    }
    // allocating it
    semRes = semget(semKey, playersCount, IPC_CREAT | READ_WRITE_CLEARANCE);
    if (semRes == -1)
    {
        throwException("Error during sem allocation!");
    }
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

int getMsgQueueId(bool skipEEXISTError)
{
    key_t msqKey = getKey();
    int msqId = msgget(msqKey, READ_WRITE_CLEARANCE);
    if (msqId == -1 && skipEEXISTError == false)
    {
        throwException("getMsgQueueId");
    }
    return msqId;
}

void unallocateMsgQueue()
{
    int msqId = getMsgQueueId(false);
    int deleteRes = msgctl(msqId, IPC_RMID, NULL);
    if (deleteRes == -1)
    {
        throwException("unallocateMsgQueue");
    }
}

int allocateMsgQueue()
{
    key_t msqKey = getKey();
    // unallocate if it exists
    int msqId = msgget(msqKey, IPC_EXCL);
    if (msqId == -1 && errno == EEXIST)
    {
        unallocateMsgQueue();
        printf("msg cancellata!\n");
    }
    // allocating it
    msqId = msgget(msqKey, IPC_CREAT | READ_WRITE_CLEARANCE);
    if (msqId == -1)
    {
        throwException("allocateMsgQueue");
    }
    return msqId;
}

int sendMsgQueue(int msgType, char *message, bool skipEEXISTerror)
{
    message_buf msgBuf;
    msgBuf.mtype = msgType;
    sprintf(msgBuf.mtext, "%s", message);

    int msqId = getMsgQueueId(skipEEXISTerror);
    int msgsndRes = msgsnd(msqId, &msgBuf, sizeof(msgBuf.mtext), READ_WRITE_CLEARANCE);
    if (msgsndRes == -1 && skipEEXISTerror == false)
    {
        throwException("sendMsgQueue");
    }
}

int receiveMsgQueue(int msgType, char *receivedMessage, bool skipEEXISTerror)
{
    message_buf msgBuf;
    int msqId = getMsgQueueId(skipEEXISTerror);
    int msgRcvRes = msgrcv(msqId, &msgBuf, sizeof(msgBuf.mtext), msgType, READ_WRITE_CLEARANCE);
    if (msgRcvRes == -1 && errno != EIDRM && skipEEXISTerror == false)
    {
        printf("invalid?1\n");
        throwException("receiveMsgQueue");
    }
    else if (errno == EIDRM && skipEEXISTerror == false)
    {
        printf("invalid?2\n");
        return -1;
    }
    sprintf(receivedMessage, "%s", msgBuf.mtext);
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
