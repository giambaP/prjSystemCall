#include "gioco-lib.c"

#define MIN_BET_PERCENTAGE 1
#define MAX_BET_PERCENTAGE 50

char *playerPossibleNames[] = {"Giovanni", "Pietro", "Arianna", "Tommaso", "Alice", "Michael", "Arturo", "Stefano", "Michele", "Giacomo", "Silvia", "Martina", "Lucrezia", "Filippo", "Giambattista", "Michael", "Tiziana", "Elia", "Sara", "Raffaele"};

void nextPlayer(int semId, int semNumPlayer, int playersCount);

int main(int argc, char *argv[])
{
    printf("Ricerca partita...\n");

    // waiting message queue creation
    bool skipEEXISTError = true;
    int msqId = -1;
    while (msqId == -1)
    {
        msqId = getMsgQueueId(skipEEXISTError);
        sleep(1);
    }

    // sorting list of players possible names
    randomSort(playerPossibleNames, ARRSIZE(playerPossibleNames), sizeof(playerPossibleNames[0]));

    // send message to banco
    char *msgSent = malloc(sizeof(char));
    sprintf(msgSent, "%d", getpid());
    sendMsgQueue(MSG_TYPE_USER_MATCH, msgSent);
    free(msgSent);

    // receive message from banco
    char *msgReceived = (char *)malloc(sizeof(char));
    receiveMsgQueue(getpid(), msgReceived);
    int semNumPlayer = atoi(msgReceived);
    free(msgReceived);
    printf("Partita trovata\n");

    // setup player data
    int startingMoney = randomValue(semNumPlayer, (int)MIN_INIT_PLAYER_MONEY, (int)MAX_INIT_PLAYER_MONEY);
    GameData *gameData = getGameData();
    PlayerData pd;
    pd.dataId = semNumPlayer - 1;
    pd.pid = (int)getpid();
    pd.semNum = semNumPlayer;
    strncpy(pd.playerName, playerPossibleNames[semNumPlayer], sizeof(char[30]));
    pd.startingMoney = startingMoney / 10 * 10; // TODO conti tondi per il momento
    pd.currentMoney = pd.startingMoney;
    pd.currentBet = 0;
    pd.firstDiceResult = 0;
    pd.secondDiceResult = 0;
    pd.totalDiceResult = 0;
    pd.finalMoney = 0;
    pd.playedGamesCount = 0;
    pd.winnedGamesCount = 0;
    pd.losedGamesCount = 0;
    memcpy(gameData->playersData + pd.dataId, &pd, sizeof(PlayerData));

    PlayerData *playerData = gameData->playersData + pd.dataId;
    int semId = getSemId();

    while (1)
    {
        pausePlayer(semNumPlayer, semId);

        switch (gameData->actionType)
        {
        case WELCOME:
        {
            // TODO to remove
            printf("-------------------------------------------\n");
            printf("GIOCATORE %s [pid:%d, semnum:%d]\n", playerData->playerName, playerData->pid, playerData->semNum);
            printf("             [startingMoney:%d]\n", playerData->startingMoney);
            printf("             [currentMoney:%d]\n", playerData->currentMoney);
            printf("             [currentBet:%d]\n", playerData->currentBet);
            printf("             [finalMoney:%d]\n", playerData->finalMoney);
            printf("             [playedGamesCount:%d]\n", playerData->playedGamesCount);
            printf("             [winnedGamesCount:%d]\n", playerData->winnedGamesCount);
            printf("             [losedGamesCount:%d]\n", playerData->losedGamesCount);
            printf("-------------------------------------------\n");

            nextPlayer(semId, semNumPlayer, gameData->playersCount);
        }
        break;
        case BET:
        {
            // betting between 1 and 50 percent of current money
            int randomSeed = semNumPlayer + playerData->currentMoney;
            playerData->currentBetPercentage = randomValue(randomSeed, MIN_BET_PERCENTAGE, MAX_BET_PERCENTAGE);
            playerData->currentBet = (playerData->currentMoney * playerData->currentBetPercentage) / 100;
            if (playerData->currentBet < 1)
            {
                playerData->currentBet = 1;
                playerData->currentBetPercentage = (playerData->currentBet * 100) / playerData->currentMoney;
            }
            // TODO to remove
            printf("Scommessa: %d %% di %d pari a %d %s\n", playerData->currentBetPercentage, playerData->currentMoney, playerData->currentBet, EXCHANGE);
            nextPlayer(semId, semNumPlayer, gameData->playersCount);
        }
        break;
        case PLAY:
        {
            playerData->firstDiceResult = randomValue(playerData->currentMoney, 1, 6);
            playerData->secondDiceResult = randomValue(playerData->currentBet, 1, 6);
            playerData->totalDiceResult = playerData->firstDiceResult + playerData->secondDiceResult;
            // TODO to remove
            printf("Ho tirato i dati: %d e %d, totale %d\n", playerData->firstDiceResult, playerData->secondDiceResult, playerData->totalDiceResult);
            nextPlayer(semId, semNumPlayer, gameData->playersCount);
        }
        break;
        case LEAVE: // TODO definire una giocata minima!!! SENNO' NON FINIRA' MAI LA PARTITA (sempre il 50%)
        {
        }
        break;
        default:
        {
            char message[] = "Unsupported operation type with code %d!";
            sprintf(message, message, gameData->actionType);
            throwException(message);
        }
        }
    }

    return 0;
}

void nextPlayer(int semId, int semNumPlayer, int playersCount)
{
    int nextSemNumPlayer = (semNumPlayer + 1) <= playersCount ? (semNumPlayer + 1) : 0;
    setSem(nextSemNumPlayer, semId, 1);
}
