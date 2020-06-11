#include "gioco-lib.c"

#define MIN_BET_PERCENTAGE 1
#define MAX_BET_PERCENTAGE 50

void waitMessageQueueInitialization();
void setupPlayer(int dataId);
int lookUpGame();
void play(int dataId);
void nextPlayer(int semId, int semNumPlayer);

int main(int argc, char *argv[])
{
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

    int startingMoney = randomValue(getpid(), (int)MIN_INIT_PLAYER_MONEY, (int)MAX_INIT_PLAYER_MONEY);
    GameData *gameData = getGameData();
    PlayerData pd;
    pd.dataId = dataId;
    pd.pid = getpid();
    pd.semNum = dataId + 1;
    strncpy(pd.playerName, playerPossibleNames[dataId], sizeof(char[30]));
    pd.playerStatus = TO_CONNECT;
    pd.startingMoney = startingMoney / 10 * 10; // TODO conti tondi per il momento
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
    receiveMsgQueue(getpid(), msgReceived);
    int dataId = atoi(msgReceived);
    free(msgReceived);
    printf("Partita trovata\n");

    return dataId;
}

void play(int dataId)
{
    GameData *gameData = getGameData();
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
            playerData->currentBet = randomValue(getpid(), minBetValue, maxBetValue);
            playerData->currentBet = playerData->currentBet < 1 ? 1 : playerData->currentBet;
            playerData->currentBetPercentage = ((float)playerData->currentBet * 100) / ((float)playerData->currentMoney);
            nextPlayer(semId, playerData->semNum);
            break;
        }
        case PLAY:
        {
            playerData->firstDiceResult = randomValue(playerData->currentMoney * getpid(), 1, 6);
            playerData->secondDiceResult = randomValue(playerData->currentBet * getpid(), 1, 6);
            playerData->totalDiceResult = playerData->firstDiceResult + playerData->secondDiceResult;
            nextPlayer(semId, playerData->semNum);
            break;
        }
        case LEAVE:
        {
            printf("-------------------------------------------\n");
            printf("GIOCATORE %s [pid:%d, semnum:%d]\n", playerData->playerName, playerData->pid, playerData->semNum);
            printf("             [startingMoney:%d]\n", playerData->startingMoney);
            printf("             [currentMoney:%d]\n", playerData->currentMoney);
            printf("             [currentBet:%d]\n", playerData->currentBet);
            printf("             [playedGamesCount:%d]\n", playerData->playedGamesCount);
            printf("             [winnedGamesCount:%d]\n", playerData->winnedGamesCount);
            printf("             [losedGamesCount:%d]\n", playerData->losedGamesCount);
            printf("-------------------------------------------\n");
            nextPlayer(semId, CROUPIER_SEM_NUM);
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
