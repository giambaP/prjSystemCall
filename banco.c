#include "gioco-lib.c"

#define REQUIRED_INPUT_PARAMS 2
#define PLAYER_CROUPIER_MONEY_RATIO 5 // definisce il rapporto tra i soldi del croupier e di ogni giocatore
#define PLAYER_PLAYER_WIN_RATIO 2     // definisce, in caso di vincita del giocatore, il numero di volte che deve essere moltiplicata la somma scommessa

void printTitle();
void setupGameData(GameData *gameData);
void connectPlayers();
void connectPlayer(int dataId);
void play();
void nextPlayer(int semId);
void terminateOtherPrograms();
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

    srand(getpid() * 100);

    // setup game data
    int startingMoney = randomValue((int)MIN_INIT_PLAYER_MONEY, (int)MAX_INIT_PLAYER_MONEY) * PLAYER_CROUPIER_MONEY_RATIO;
    GameData gameData;
    gameData.croupierPid = getpid();
    gameData.croupierSemNum = CROUPIER_SEM_NUM;
    gameData.croupierStartingMoney = startingMoney;
    gameData.croupierCurrentMoney = gameData.croupierStartingMoney;
    gameData.totalPlayedGamesCount = 0;
    gameData.winnedGamesCount = 0;
    gameData.losedGamesCount = 0;
    gameData.actionType = WELCOME;

    allocateShm(&gameData);
    allocateSem(MAX_DEFAULT_PLAYERS + 1); // +1 -> croupier
    allocateMsgQueue();

    printTitle();

    connectPlayers();

    play();

    return 0;
}

void printTitle()
{
    printf("\n");
    printf("**************************************\n");
    printf("****     BENVENUTI AL CASINO'     ****\n");
    printf("**************************************\n");
}

void connectPlayer(int dataId)
{
    // listening for player message
    char *msgReceived = (char *)malloc(sizeof(MSG_QUEUE_SIZE));
    receiveMsgQueue(MSG_TYPE_USER_MATCH, msgReceived);
    int playerPid = atoi(msgReceived);
    free(msgReceived);

    // response to player with dataId
    char *msgSent = (char *)malloc(sizeof(MSG_QUEUE_SIZE));
    sprintf(msgSent, "%d", dataId);
    sendMsgQueue(playerPid, msgSent);
    free(msgSent);
}

void connectPlayers()
{
    printf("Ricerca %d giocatori...\n", MAX_DEFAULT_PLAYERS);
    for (int dataId = 0; dataId < MAX_DEFAULT_PLAYERS; dataId++)
    {
        connectPlayer(dataId);
        printf("Giocatore %d/%d connesso.\n", dataId + 1, MAX_DEFAULT_PLAYERS);
    }
    printf("Ricerca giocatori completata.\n");
}

void play()
{
    GameData *gameData = getGameData(false);
    int semId = getSemId();

    printf("\nBanco impostato: totale cassa %d %s\n", gameData->croupierCurrentMoney, EXCHANGE);

    bool loop = true;
    while (loop == true)
    {
        switch (gameData->actionType)
        {
        case WELCOME:
        {
            // start game
            nextPlayer(semId);
            pausePlayer(CROUPIER_SEM_NUM, semId);

            printf("\nDiamo il benvenuto ai nostri giocatori: \n");
            for (int i = 0; i < MAX_DEFAULT_PLAYERS; i++)
            {
                PlayerData p = gameData->playersData[i];
                if (p.playerStatus == CONNECTED)
                {
                    p.playerStatus == PLAYING;
                    printf(" - %-13s-> budget di %d %s\n", p.playerName, p.currentMoney, EXCHANGE);
                }
            }
            gameData->actionType = BET;
        }
        break;
        case BET:
        {
            gameData->totalPlayedGamesCount++;
            printf("\n\n=============[  ROUND N. %d  ]===============\n", gameData->totalPlayedGamesCount);
            printf("\nI giocatori facciano la loro puntata:\n");
            nextPlayer(semId);
            pausePlayer(CROUPIER_SEM_NUM, semId);
            for (int i = 0; i < MAX_DEFAULT_PLAYERS; i++)
            {
                PlayerData p = gameData->playersData[i];
                printf(" - %-13s-> %d %s: %.1f%% di %d %s\n", p.playerName, p.currentBet, EXCHANGE, p.currentBetPercentage, p.currentMoney, EXCHANGE);
            }
            gameData->actionType = PLAY;
        }
        break;
        case PLAY:
        {
            printf("\nTiro dei dati:\n");
            nextPlayer(semId);
            pausePlayer(CROUPIER_SEM_NUM, semId);

            // croupier role dices
            int croupierFirstDiceResult = randomValue(1, 6);
            int croupierSecondDiceResult = randomValue(1, 6);
            int croupierTotalDiceResult = croupierFirstDiceResult + croupierSecondDiceResult;

            // showing dices result
            printf(" - %-13s-> %d e %d, totale %d\n", "Banco", croupierFirstDiceResult, croupierSecondDiceResult, croupierTotalDiceResult);
            for (int i = 0; i < MAX_DEFAULT_PLAYERS; i++)
            {
                PlayerData p = gameData->playersData[i];
                printf(" - %-13s-> %d e %d, totale %d\n", p.playerName, p.firstDiceResult, p.secondDiceResult, p.totalDiceResult);
            }

            // calculating data
            for (int i = 0; i < MAX_DEFAULT_PLAYERS; i++)
            {
                PlayerData *p = gameData->playersData + i;
                // player win
                if (p->totalDiceResult >= croupierTotalDiceResult)
                {
                    int win = p->currentBet * PLAYER_PLAYER_WIN_RATIO;
                    gameData->croupierCurrentMoney -= win;
                    p->currentMoney += win;
                    p->winnedGamesCount++;
                    p->lastRoundResult = WINNED;
                }
                // croupier win (in case of same result player win)
                else
                {
                    gameData->croupierCurrentMoney += p->currentBet;
                    p->currentMoney -= p->currentBet;
                    p->losedGamesCount++;
                    p->lastRoundResult = LOSED;
                }
                p->playedGamesCount++;
            }

            // printing results
            printf("\nRisultato round %d\n", gameData->totalPlayedGamesCount);
            if (gameData->croupierCurrentMoney > 0)
            {
                printf(" - %-13s-> %d %s\n", "Banco", gameData->croupierCurrentMoney, EXCHANGE);
            }
            else
            {
                printf(" - %-13s-> BANCAROTTA\n", "Banco");
            }
            bool playerFailureCount = 0;
            for (int i = 0; i < MAX_DEFAULT_PLAYERS; i++)
            {
                PlayerData *p = gameData->playersData + i;
                switch (p->lastRoundResult)
                {
                case WINNED:
                {
                    printf(" - %-13s-> vince %d %s: ora possiede %d %s\n", p->playerName, p->currentBet * PLAYER_PLAYER_WIN_RATIO, EXCHANGE, p->currentMoney, EXCHANGE);
                }
                break;
                case LOSED:
                {
                    if (p->currentMoney > 0)
                    {
                        printf(" - %-13s-> perde %d %s: ora possiede %d %s\n", p->playerName, p->currentBet, EXCHANGE, p->currentMoney, EXCHANGE);
                    }
                    else
                    {
                        printf(" 4- %-13s-> perde %d %s: non possiede più denaro!\n", p->playerName, p->currentBet, EXCHANGE);
                        p->playerStatus = TO_DISCONNECT;
                        playerFailureCount++;
                    }
                }
                break;
                case INIT:
                default:
                {
                    char message[] = "Unsupported 'lastRoundResult' with code %d!";
                    sprintf(message, message, gameData->actionType);
                    throwException(message);
                }
                }
            }

            printf("\n=============ROUND N. %d TERMINATO===========\n", gameData->totalPlayedGamesCount);

            // checking croupier failure
            if (gameData->croupierCurrentMoney <= 0)
            {
                printf("\nIl Banco è finito in bancarotta!\n");
                terminateOtherPrograms();
                loop = false; // exit program
            }
            // searching for new players in case of players failure
            else if (playerFailureCount > 0)
            {
                printf("\n%s in bancarotta. \n", playerFailureCount > 1 ? "Alcuni giocatori sono andati" : "Un giocatore è andato");
                printf("Salutiamo"); // senza \n volontariamente
                bool addCommaString = false;
                for (int i = 0; i < MAX_DEFAULT_PLAYERS; i++)
                {
                    PlayerData *p = gameData->playersData + i;
                    if (p->playerStatus == TO_DISCONNECT)
                    {
                        printf("%s %s", addCommaString == true ? "," : "", p->playerName);
                        addCommaString = true;
                    }
                }
                printf("\n");

                printf("\nRicerca di %d %s...\n", playerFailureCount, playerFailureCount > 1 ? "nuovi giocatori" : "nuovo giocatore");
                // changed failure players
                gameData->actionType = LEAVE;
                for (int i = 0; i < MAX_DEFAULT_PLAYERS; i++)
                {
                    PlayerData *p = gameData->playersData + i;
                    if (p->playerStatus == TO_DISCONNECT)
                    {
                        // waiting failure player deleting
                        setSem(p->semNum, semId, 1);
                        pausePlayer(CROUPIER_SEM_NUM, semId);

                        // waiting new player
                        connectPlayer(p->dataId);
                    }
                }
                printf("Ricerca completata.\n");
                gameData->actionType = WELCOME;
            }
            // continue to bet
            else
            {
                gameData->actionType = BET;
            }
        }
        break;
        case LEAVE:
        default:
        {
            char message[] = "Unsupported operation type with code %d!";
            sprintf(message, message, gameData->actionType);
            throwException(message);
        }
        }
    }
}

void nextPlayer(int semId)
{
    setSem(CROUPIER_SEM_NUM + 1, semId, 1);
}

void terminateOtherPrograms()
{
    GameData *gameData = getGameData(true);
    if (((long)gameData) != -1)
    {
        int myPid = getpid();
        // killing other players
        for (int i = 0; i < MAX_DEFAULT_PLAYERS; i++)
        {
            PlayerData *p = gameData->playersData + i;
            if (p->pid != myPid)
            {
                kill(p->pid, SIGTERM);
            }
        }
    }
    unallocateMsgQueue();
    unallocateShm(false);
    unallocateSem(false);
    printf("\nProgramma terminato. Arrivederci!\n");
}

void sigIntHandler(int sig)
{
    terminateOtherPrograms();
    exit(0);
}

void sigTermHandler(int sig)
{
    terminateOtherPrograms();
    exit(0);
}