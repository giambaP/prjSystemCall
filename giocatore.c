#include "gioco-lib.c"

#define MIN_BET_PERCENTAGE 1
#define MAX_BET_PERCENTAGE 50

void waitMessageQueueInitialization();
void setupPlayer(int dataId);
int lookUpGame();
void play(int dataId);
void nextPlayer(int semId, int semNumPlayer);
void sigIntHandler(int sig);
void sigTermHandler(int sig);

int main(int argc, char *argv[])
{
    if (signal(SIGINT, sigIntHandler) == SIG_ERR)
    {
        printf("SIGINT install error\n");
        exit(2);
    }
    if (signal(SIGTERM, sigTermHandler) == SIG_ERR)
    {
        printf("SIGTERM install error\n");
        exit(2);
    }
    if (signal(SIGQUIT, sigIntHandler) == SIG_ERR)
    {
        printf("SIGQUIT install error\n");
        exit(2);
    }

    srand(getpid());

    waitMessageQueueInitialization();

    int dataId = lookUpGame();
    setupPlayer(dataId);
    play(dataId);

    return 0;
}

void waitMessageQueueInitialization()
{
    printf("Inizializzazione giocatore...\n");
    bool skipEEXISTError = true;
    bool msgQueueFound = false;
    while (msgQueueFound == false)
    {
        msgQueueFound = getMsgQueueId(skipEEXISTError) != -1 ? true : false;
        sleep(1);
    }
}

void setupPlayer(int dataId)
{
    // sorting list of possibile names
    char *playerPossibleNames[] = {"Giovanni", "Pietro", "Arianna", "Tommaso", "Alice", "Michael", "Arturo", "Stefano", "Michele", "Giacomo", "Silvia", "Martina", "Lucrezia", "Filippo", "Giambattista", "Michael", "Tiziana", "Elia", "Sara", "Raffaele"};
    randomSort(playerPossibleNames, ARRSIZE(playerPossibleNames), sizeof(playerPossibleNames[0]));

    GameData *gameData = getGameData(false);

    int startingMoney = randomValue((int)MIN_INIT_PLAYER_MONEY, (int)MAX_INIT_PLAYER_MONEY);
    PlayerData pd;
    pd.dataId = dataId;
    pd.pid = getpid();
    pd.semNum = dataId + 1;
    strncpy(pd.playerName, playerPossibleNames[dataId], sizeof(char[30]));
    pd.playerStatus = TO_CONNECT;
    pd.startingMoney = startingMoney;
    pd.currentMoney = pd.startingMoney;
    pd.currentBet = 0;
    pd.currentBetPercentage = 0;
    pd.lastRoundResult = NONE;
    pd.firstDiceResult = 0;
    pd.secondDiceResult = 0;
    pd.totalDiceResult = 0;
    pd.playedGamesCount = 0;
    pd.winnedGamesCount = 0;
    pd.losedGamesCount = 0;
    memcpy(gameData->playersData + pd.dataId, &pd, sizeof(PlayerData));
}

int lookUpGame()
{
    printf("Ricerca partita...\n");

    // send message to banco
    char *msgSent = malloc(sizeof(MSG_QUEUE_SIZE));
    sprintf(msgSent, "%d", getpid());
    sendMsgQueue(MSG_TYPE_USER_MATCH, msgSent);
    free(msgSent);

    // receive message from banco
    char *msgReceived = (char *)malloc(sizeof(MSG_QUEUE_SIZE));
    if (receiveMsgQueue(getpid(), msgReceived) == -1)
    {
        exit(0);
    }
    int dataId = atoi(msgReceived);
    free(msgReceived);
    printf("Partita trovata\n");

    return dataId;
}

void play(int dataId)
{
    GameData *gameData = getGameData(false);
    PlayerData *playerData = gameData->playersData + dataId;
    int semId = getSemId();

    bool loop = true;
    while (loop == true)
    {
        pausePlayer(playerData->semNum, semId);

        switch (gameData->actionType)
        {
        case WELCOME:
        {
            if (playerData->playerStatus != CONNECTED)
            {
                printf("Giocatore %s connesso\n", playerData->playerName);
                playerData->playerStatus = CONNECTED;
            }
            nextPlayer(semId, playerData->semNum);
            break;
        }
        case BET:
        {
            // betting between 1 and 50 percent of current money
            int minBetValue = (playerData->currentMoney * MIN_BET_PERCENTAGE) / 100;
            int maxBetValue = (playerData->currentMoney * MAX_BET_PERCENTAGE) / 100;
            playerData->currentBet = randomValue(minBetValue, maxBetValue);
            playerData->currentBet = playerData->currentBet < 1 ? 1 : playerData->currentBet;
            playerData->currentBetPercentage = ((float)playerData->currentBet * 100) / ((float)playerData->currentMoney);
            nextPlayer(semId, playerData->semNum);
            break;
        }
        case PLAY:
        {
            playerData->firstDiceResult = randomValue(1, 6);
            playerData->secondDiceResult = randomValue(1, 6);
            playerData->totalDiceResult = playerData->firstDiceResult + playerData->secondDiceResult;
            nextPlayer(semId, playerData->semNum);
            break;
        }
        case LEAVE:
        {
            printf("Partita finita: il giocatore è finito in bancarotta!\n");
            printf("-------------------------------------------\n");
            printf("Informazioni giocatore:\n");
            printf("  - Nome:            %s\n", playerData->playerName);
            printf("  - Soldi iniziali:  %d %s\n", playerData->startingMoney, EXCHANGE);
            printf("  - Soldi correnti:  %d %s\n", playerData->currentMoney, EXCHANGE);
            printf("  - Partite giocate: %d \n", playerData->playedGamesCount);
            printf("  - Partite vinte:   %d \n", playerData->winnedGamesCount);
            printf("  - Partite perse:   %d \n", playerData->losedGamesCount);
            printf("-------------------------------------------\n");
            setSem(CROUPIER_SEM_NUM, semId, 1);
            loop = false;
            break;
        }
        default:
        {
            char message[] = "Unsupported operation type with code %d!";
            sprintf(message, message, gameData->actionType);
            throwException(message);
        }
        }
    }
}

void nextPlayer(int semId, int semNumPlayer)
{
    int nextSemNumPlayer = (semNumPlayer + 1) <= MAX_DEFAULT_PLAYERS ? (semNumPlayer + 1) : CROUPIER_SEM_NUM;
    setSem(nextSemNumPlayer, semId, 1);
}

void sigIntHandler(int sig)
{
    GameData *gameData = getGameData(true);
    if (((long)gameData) != -1)
    {
        int myPid = getpid();
        // killing other players
        for (int i = 0; i < MAX_DEFAULT_PLAYERS; i++)
        {
            PlayerData *p = gameData->playersData + i;
            if (p->pid != myPid && p->pid != 0)
            {
                kill(p->pid, SIGTERM);
            }
        }
        // killing banco
        kill(gameData->croupierPid, SIGTERM);
    }
    exit(0);
}

void sigTermHandler(int sig)
{
    printf("\nProgramma terminato. Arrivederci!\n");
    exit(0);
}