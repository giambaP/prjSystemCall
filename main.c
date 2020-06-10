// #include <stdio.h>
// #include <unistd.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <string.h>
// #include <sys/types.h>
// #include <stdbool.h>
// #include <time.h>
// #include <sys/wait.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>
// #include <sys/sem.h>

// #define SEM_KEY_PATH "/dev/null"
// #define SEM_KEY_ID 100
// #define MAX_DEFAULT_PLAYERS 3
// #define REQUIRED_INPUT_PARAMS 2
// #define CROUPIER_PLAYER_ID 0
// #define READ_CLEARANCE 0777 // TODO rimuovere
// #define MAX_INIT_PLAYER_MONEY 500
// #define MIN_INIT_PLAYER_MONEY 200
// #define ARRSIZE(x) (sizeof(x) / sizeof(x[0]))
// #define MINIMUM_BET 30

// int PLAYER_CROUPIER_MONEY_RATIO; // definisce il rapporto tra i soldi del croupier e di ogni giocatore

// typedef union {
//     int val;
//     struct semid_ds *buf;
//     short *array;
// } Semun;
// typedef enum actionType
// {
//     INIT = 0,
//     WELCOME = 1,
//     PLAY = 2,
//     LEAVE = 3
// } PlayerActionType;
// typedef struct pdata
// {
//     unsigned int playerId;
//     char *playerName;
//     unsigned int actionType;       // definisce il tipo di azione che deve fare il giocatore
//     unsigned int startingMoney;    // soldi iniziali
//     unsigned int currentMoney;     // soldi correnti
//     unsigned int currentBet;       // soldi scommessi in questa giocata
//     unsigned int finalMoney;       // soldi finali
//     unsigned int playedGamesCount; // partite giocate
//     unsigned int winnedGamesCount; // partite vinte
//     unsigned int losedGamesCount;  // partite perse
// } PlayerData;
// typedef struct gdata
// {
//     PlayerData *playersData; // contiene i dati di tutti i giocatori
//     unsigned int playersCount;
//     unsigned int playedGames;
// } GameData;

// void printTitle();
// void initPlayer(unsigned int playerId);
// void pausePlayer(unsigned int playerId, int semId);
// void buildCroupier(PlayerData *player, int playerId);
// GameData *getGameData();
// unsigned int randomValue(int randomSeed, int rangeFrom, int rangeTo);

// void allocateShm(GameData *gameData);
// int getShmId();
// void unallocateShm(int skipLog);
// void allocateSem(int playersCount);
// int getSemId();
// void setSem(unsigned int playerId, int semId, int incremental);
// void unallocateSem(int skipLog);
// key_t getKey();
// void throwException(char *message);
// void randomSort(char *array[], size_t n, size_t size);
// void endProgram(int i);

// char *playerPossibleNames[] = {"Giovanni", "Pietro", "Arianna", "Tommaso", "Alice", "Michael", "Arturo", "Stefano", "Michele", "Giacomo", "Silvia", "Martina", "Lucrezia", "Filippo", "Giambattista", "Michael", "Tiziana", "Elia", "Sara", "Raffaele"};
// int _usedNameIndex;

// int main(int argc, char *argv[])
// {
//     // checking parameters
//     if (argc != 1 && argc != REQUIRED_INPUT_PARAMS)
//     {
//         fprintf(stderr, "Numero parametri input non corretto! Attesi: %d, Trovati: %d\n", REQUIRED_INPUT_PARAMS, argc);
//         fprintf(stderr, "Programma terminato!\n");
//         exit(1);
//     }
//     // setting players count
//     int playersCount = (argc == REQUIRED_INPUT_PARAMS ? atoi(argv[1]) : MAX_DEFAULT_PLAYERS);
//     PLAYER_CROUPIER_MONEY_RATIO = playersCount;
//     // adding croupier
//     playersCount++;

//     // managing signal
//     signal(SIGINT, endProgram);
//     signal(SIGTERM, endProgram);
//     signal(SIGKILL, endProgram);

//     // sort players names
//     _usedNameIndex = playersCount - 1;
//     randomSort(playerPossibleNames, ARRSIZE(playerPossibleNames), sizeof(playerPossibleNames[0]));

//     // init game data
//     PlayerData playersData[playersCount];
//     GameData gameData;
//     gameData.playersData = playersData;
//     gameData.playersCount = playersCount;
//     gameData.playedGames = 0;

//     allocateShm(&gameData);
//     allocateSem(playersCount);

//     printTitle();

//     // init players
//     for (int playerId = 0; playerId < playersCount; playerId++)
//     {
//         int pid = fork();
//         if (pid < 0)
//         {
//             throwException("Error during fork!");
//         }
//         if (pid == 0) // players and croupier
//         {
//             initPlayer(playerId);
//         }
//     }

//     // starting game
//     setSem(CROUPIER_PLAYER_ID, getSemId(), 1);

//     while (1)
//     {
//         int status;
//         int i = wait(&status);
//         if (status == 0) // only first sem, croupier, could finish with 0
//         {
//             endProgram(0);
//         }
//         else
//         {
//             throwException("Child process failed!");
//         }
//     }

//     return 0;
// }

// void printTitle()
// {
//     printf("**************************************\n");
//     printf("****     BENVENUTI AL CASINO'     ****\n");
//     printf("**************************************\n");
// }

// // gestito come se fosse su un altro programma (ogni player recupera tutte le informazioni che gli servono)
// void initPlayer(unsigned int playerId)
// {
//     // setup player data
//     GameData *gameData = getGameData();
//     PlayerData *playerData = &(gameData->playersData[playerId]);
//     buildCroupier(playerData, playerId);

//     int semId = getSemId();
//     bool firstInit = true;

//     while (1)
//     {
//         if (playerId == CROUPIER_PLAYER_ID)
//         {
//             pausePlayer(playerId, semId);
//             if (firstInit == true)
//             {
//                 printf("Diamo il benvenuto ai nostri giocatori!\n");
//                 for (int nextPlayerId = 1; nextPlayerId < gameData->playersCount; nextPlayerId++)
//                 {
//                     gameData->playersData[playerId].actionType = WELCOME;
//                 }
//                 setSem(playerId + 1, semId, 1);
//                 firstInit = false;
//             }
//             else
//             {
//                 // next action !!
//             }

//             if (playerData->currentMoney < MINIMUM_BET) // TODO manage exit 0 when croupier in bancarotta
//             {
//                 printf("Il banco è finito in bancarotta! Complimenti ai giocatori!\n");
//                 printf("Gioco finito!!!\n");
//                 exit(0);
//             }
//         }
//         else // normal player
//         {
//             pausePlayer(playerId, semId);
//             printf("Player %d, action is %d\n", playerId, playerData->actionType);
//             switch (playerData->actionType)
//             {
//             case WELCOME:
//             {
//                 int nextPlayerId = (playerId + 1) < gameData->playersCount ? (playerId + 1) : 0;
//                 printf("Ciao a tutti, sono %s e il mio budget è di %d euro. Buona partita a tutti\n", playerData->playerName, playerData->startingMoney);
//                 setSem(nextPlayerId + 1, semId, 1);
//             }
//             break;
//             case PLAY:
//             {
//             }
//             break;
//             case LEAVE: // TODO definire una giocata minima!!! SENNO' NON FINIRA' MAI LA PARTITA (sempre il 50%)
//             {
//             }
//             break;
//             default:
//             {
//                 char message[] = "Unsupported operation type with code %d!";
//                 sprintf(message, message, playerData->actionType);
//                 throwException(message);
//             }
//             }
//         }
//     }
// }

// void pausePlayer(unsigned int playerId, int semId)
// {
//     setSem(playerId, semId, -1);
// }

// void buildCroupier(PlayerData *playerData, int playerId)
// {
//     int startingMoney = randomValue(playerId, (int)MIN_INIT_PLAYER_MONEY, (int)MAX_INIT_PLAYER_MONEY) * (playerId == CROUPIER_PLAYER_ID ? (int)PLAYER_CROUPIER_MONEY_RATIO : 1);
//     startingMoney = startingMoney / 10 * 10; // TODO conti tondi per il momento
//     playerData->playerId = playerId;
//     playerData->playerName = playerId != CROUPIER_PLAYER_ID ? playerPossibleNames[playerId - 1] : "Croupier";
//     playerData->actionType = 0;
//     playerData->startingMoney = startingMoney;
//     playerData->currentMoney = startingMoney;
//     playerData->finalMoney = 0;
//     playerData->playedGamesCount = 0;
//     playerData->winnedGamesCount = 0;
//     playerData->losedGamesCount = 0;
// }

// unsigned int randomValue(int randomSeed, int rangeFrom, int rangeTo)
// {
//     srand((unsigned int)randomSeed * time(NULL));
//     return (rand() % (rangeTo - rangeFrom + 1)) + rangeFrom;
// }

// void allocateShm(GameData *gameData)
// {
//     // CERCARE MEMORIA TODO
//     // SE ESISTE GIA' DEALLOCARLA
//     // POI ALLOCCARLA
//     key_t shmKey = getKey();
//     int shmId = shmget(shmKey, sizeof(GameData), IPC_CREAT | READ_CLEARANCE); // TEMPORANEO ***************
//     if (shmId == -1)
//     {
//         throwException("Error during shm allocation!");
//     }
//     errno = 0;
//     GameData *shmP = (GameData *)shmat(shmId, 0, 0);
//     if (errno != 0)
//     {
//         char *errorMessage;
//         switch (errno)
//         {
//         case EACCES:
//             errorMessage = "The calling process does not have the required permissions for the requested attach type, and does not have the CAP_IPC_OWNER capability.";
//             break;
//         case EIDRM:
//             errorMessage = "shmid points to a removed identifier.";
//             break;
//         case EINVAL:
//             errorMessage = "Invalid shmid value, unaligned (i.e., not page-aligned and SHM_RND was not specified) or invalid shmaddr value, or can't attach segment at shmaddr, or SHM_REMAP was specified and shmaddr was NULL.";
//             break;
//         case ENOMEM:
//             errorMessage = "Could not allocate memory for the descriptor or for the page tables.";
//             break;
//         default:
//             errorMessage = "no error message defined.";
//         }
//         char *message = "Cannot get game data! Error during shmat! ";
//         char *res = (char *)malloc(sizeof(*errorMessage) + sizeof(*message));
//         strcpy(res, errorMessage);
//         strcat(res, message);
//         throwException(message);
//     }
//     memcpy(shmP, gameData, sizeof(GameData));
// }

// GameData *getGameData()
// {
//     // CERCARE MEMORIA TODO
//     // SE ESISTE GIA' DEALLOCARLA
//     // ALLOCCARLA
//     int shmId = getShmId();
//     errno = 0;
//     GameData *p = (GameData *)shmat(shmId, 0, 0);
//     if (errno != 0)
//     {
//         char *errorMessage;
//         switch (errno)
//         {
//         case EACCES:
//             errorMessage = "The calling process does not have the required permissions for the requested attach type, and does not have the CAP_IPC_OWNER capability.";
//             break;
//         case EIDRM:
//             errorMessage = "shmid points to a removed identifier.";
//             break;
//         case EINVAL:
//             errorMessage = "Invalid shmid value, unaligned (i.e., not page-aligned and SHM_RND was not specified) or invalid shmaddr value, or can't attach segment at shmaddr, or SHM_REMAP was specified and shmaddr was NULL.";
//             break;
//         case ENOMEM:
//             errorMessage = "Could not allocate memory for the descriptor or for the page tables.";
//             break;
//         default:
//             errorMessage = "no error message defined.";
//             break;
//         }
//         char *message = "Cannot get game data! Error during shmat! ";
//         char *res = (char *)malloc(sizeof(*errorMessage) + sizeof(*message));
//         strcpy(res, errorMessage);
//         strcat(res, message);
//         throwException(message);
//     }
//     GameData *res = (GameData *)malloc(sizeof(GameData));
//     memcpy(res, p, sizeof(GameData));
//     return res;
// }

// int getShmId()
// {
//     key_t shmKey = getKey();

//     int shmId = shmget(shmKey, sizeof(GameData), READ_CLEARANCE);
//     if (shmId == -1)
//     {
//         throwException("Error during shm id retrieving!");
//     }
//     return shmId;
// }

// void unallocateShm(int skipLog)
// {
//     /// TODO .... usare anche SHMDT prima del semctl per svuotare la memoria
//     //detach from shared memory
//     //shmdt(str);
//     int semId = getShmId();
//     if (shmctl(semId, IPC_RMID, 0) == -1 && skipLog != 1)
//     {
//         throwException("Error during shm unallocation! Shm not deleted!");
//     }
// }

// void allocateSem(int playersCount)
// {
//     // int semId = semget(semkey, playersCount, IPC_CREAT | IPC_EXCL);
//     // if (semId == -1)
//     // {
//     //     if (errno != EEXIST)
//     //     {
//     // throwException("Error during sem allocation!");
//     //     }
//     //     unallocateSem();
//     //     semId = semget(semkey, playersCount, IPC_CREAT);
//     //      /// CHECK
//     // }
//     key_t semKey = getKey();
//     int semRes = semget(semKey, playersCount, IPC_CREAT | READ_CLEARANCE); // TEMPORANEO ***************
//     if (semRes == -1)
//     {
//         throwException("Error during sem allocation!");
//     }
// }

// int getSemId()
// {
//     key_t semKey = getKey();
//     int semId = semget(semKey, 0, READ_CLEARANCE);
//     if (semId == -1)
//     {
//         throwException("Error during sem id retrieving!");
//     }
//     return semId;
// }

// void setSem(unsigned int playerId, int semId, int incremental)
// {
//     struct sembuf sops;
//     sops.sem_num = playerId;
//     sops.sem_op = incremental;
//     sops.sem_flg = SEM_UNDO;
//     errno = 0;
//     if (semop(semId, &sops, 1) == -1)
//     {
//         char *errorMessage;
//         switch (errno)
//         {
//         case E2BIG:
//             errorMessage = "The value nsops is greater than the system limit.";
//             break;
//         case EACCES:
//             errorMessage = "Operation permission is denied to the calling process. Read access is required when sem_op is zero. Write access is required when sem_op is not zero.";
//             break;
//         case EAGAIN:
//             errorMessage = "The operation would result in suspension of the calling process but IPC_NOWAIT in sem_flg was specified.";
//             break;
//         case EFBIG:
//             errorMessage = "sem_num is less than zero or greater or equal to the number of semaphores in the set specified on in semget() argument nsems.";
//             break;
//         case EIDRM:
//             errorMessage = "semid was removed from the system while the invoker was waiting.";
//             break;
//         case EINTR:
//             errorMessage = "semop() was interrupted by a signal.";
//             break;
//         case EINVAL:
//             errorMessage = "The value of argument semid is not a valid semaphore identifier. For an __IPC_BINSEM semaphore set, the sem_op is other than +1 for a sem_val of 0 or -1 for a sem_val of 0 or 1. Also, for an __IPC_BINSEM semaphore set, the number of semaphore operations is greater than one.";
//             break;
//         case ENOSPC:
//             errorMessage = "The limit on the number of individual processes requesting a SEM_UNDO would be exceeded.";
//             break;
//         case ERANGE:
//             errorMessage = "An operation would cause semval or semadj to overflow the system limit as defined in <sys/sem.h>.";
//             break;
//         default:
//             errorMessage = "no error message defined.";
//             break;
//         }
//         char *message = "Cannot setSem: ";
//         char *res = (char *)malloc(sizeof(*errorMessage) + sizeof(*message));
//         strcpy(res, errorMessage);
//         strcat(res, message);
//         throwException(res);
//     }
// }

// void unallocateSem(int skipLog)
// {
//     int semId = getSemId();
//     int semDeleteResult = semctl(semId, 0, IPC_RMID, 0);
//     if (semDeleteResult == -1 && skipLog != 1)
//     {
//         throwException("Error during sem unallocation! Shm not deleted!");
//     }
// }

// key_t getKey()
// {
//     key_t key = ftok(SEM_KEY_PATH, SEM_KEY_ID);
//     if (key == (key_t)-1)
//     {
//         throwException("Cannot retrieve key: error during ftok generation!");
//     }
//     return key;
// }

// void throwException(char *message)
// {
//     fprintf(stderr, "\n--Execution error (PID %d): %s - %s (code %d)--\n", getpid(), message, strerror(errno), errno);
//     exit(errno);
// }

// void endProgram(int i)
// {
//     if ((int)getppid() == 1) // main process as parent pid = 1
//     {
//         unallocateShm(1);
//         unallocateSem(1);
//     }
//     exit(0);
// }

// void randomSort(char *array[], size_t n, size_t size)
// {
//     char tmp[size];
//     char *arr = (char *)array;
//     size_t stride = size * sizeof(char);
//     if (n > 1)
//     {
//         // shuffle
//         size_t i;
//         for (i = 0; i < n - 1; ++i)
//         {
//             srand((unsigned int)i * time(NULL));
//             size_t rnd = (size_t)rand();
//             size_t j = i + rnd / (RAND_MAX / (n - i) + 1);

//             memcpy(tmp, arr + j * stride, size);
//             memcpy(arr + j * stride, arr + i * stride, size);
//             memcpy(arr + i * stride, tmp, size);
//         }
//         // reverse
//         char *tmp[n];
//         for (int i = n - 1; i >= 0; i--)
//         {
//             tmp[n - 1 - i] = array[i];
//         }
//         for (int i = 0; i < n; i++)
//         {
//             array[i] = tmp[i];
//         }
//     }
// }