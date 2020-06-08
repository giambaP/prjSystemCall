#include "gioco-lib.c"

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
    GameData *gameData = getGameData();
    printf("SIZE=%d,2=%s\n", ARRSIZE(gameData->playerPossibleNames), gameData->playerPossibleNames + 0);
    int startingMoney = randomValue(semNumPlayer, (int)MIN_INIT_PLAYER_MONEY, (int)MAX_INIT_PLAYER_MONEY);
    PlayerData pd;
    pd.pid = getpid();
    pd.semNum = semNumPlayer;
    pd.startingMoney = startingMoney / 10 * 10; // TODO conti tondi per il momento
    pd.currentMoney = startingMoney;
    pd.currentBet = 0;
    pd.finalMoney = 0;
    pd.playedGamesCount = 0;
    pd.winnedGamesCount = 0;
    pd.losedGamesCount = 0;
    printf("primo nome è %s\n", gameData->playerPossibleNames + semNumPlayer);
    // pd.playerName = *;
    memcpy((gameData->playersData + semNumPlayer), &pd, sizeof(PlayerData));

    PlayerData *playerData = gameData->playersData + semNumPlayer;

    int semId = getSemId();

    while (1)
    {
        pausePlayer(semNumPlayer, semId);
        switch (gameData->actionType)
        {
        case WELCOME:
        {
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
            // printf("Ciao a tutti, sono %s e il mio budget è di %d euro. Buona partita a tutti\n", playerData->playerName, playerData->startingMoney);

            // int nextSemNumPlayer = (semNumPlayer + 1) < gameData->playersCount ? (semNumPlayer + 1) : 0;
            // setSem(nextSemNumPlayer + 1, semId, 1);
        }
        break;
        case PLAY:
        {
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