#include "gioco-lib.c"

#define REQUIRED_INPUT_PARAMS 2
#define CROUPIER_SEM_NUM 0
#define PLAYER_CROUPIER_MONEY_RATIO 4 // definisce il rapporto tra i soldi del croupier e di ogni giocatore

void connectPlayers(int maxPlayersCount);
void play();
void nextPlayer(int semId);

int main(int argc, char *argv[])
{
    // if (argc != 1 && argc != REQUIRED_INPUT_PARAMS)
    // {
    //     fprintf(stderr, "Numero parametri input non corretto! Attesi: %d, Trovati: %d\n", REQUIRED_INPUT_PARAMS, argc);
    //     fprintf(stderr, "Programma terminato!\n");
    //     exit(1);
    // }
    // int maxPlayersCount = (argc == REQUIRED_INPUT_PARAMS ? atoi(argv[1]) : MAX_DEFAULT_PLAYERS);
    // maxPlayersCount = maxPlayersCount > 0 ? maxPlayersCount : MAX_DEFAULT_PLAYERS;

    // TODO manage SIGNALS

    // setting game data
    int startingMoney = randomValue(CROUPIER_SEM_NUM, (int)MIN_INIT_PLAYER_MONEY, (int)MAX_INIT_PLAYER_MONEY) * PLAYER_CROUPIER_MONEY_RATIO;
    GameData gameData;
    gameData.croupierPid = getpid();
    gameData.croupierSemNum = CROUPIER_SEM_NUM;
    gameData.croupierStartingMoney = startingMoney / 10 * 10; // TODO conti tondi per il momento
    gameData.croupierCurrentMoney = gameData.croupierStartingMoney;
    gameData.totalPlayedGamesCount = 0;
    gameData.winnedGamesCount = 0;
    gameData.losedGamesCount = 0;
    gameData.playersCount = MAX_DEFAULT_PLAYERS; // TO REMOVE
    gameData.actionType = WELCOME;

    allocateShm(&gameData);
    allocateSem(MAX_DEFAULT_PLAYERS + 1); // +1 -> croupier
    allocateMsgQueue();

    printTitle();
    connectPlayers(MAX_DEFAULT_PLAYERS); // TO REMOVE INPUT PARAMETERS -> use directly MAX_DEFAULT_PLAYERS

    printf("MESSAGE: players connected\n");

    play();

    // TODO manage error on signal
    unallocateMsgQueue();
    unallocateShm(false);
    unallocateSem(false);

    return 0;
}

void connectPlayers(int maxPlayersCount)
{
    printf("IL MIO PID = %d\n", getpid());
    printf("Ricerca %d giocatori...\n", maxPlayersCount);
    for (int semNum = 1; semNum <= maxPlayersCount; semNum++)
    {   
        char *msgReceived = (char *)malloc(sizeof(MSG_QUEUE_SIZE));
        receiveMsgQueue(MSG_TYPE_USER_MATCH, msgReceived);
        int playerPid = atoi(msgReceived);
        free(msgReceived);

        char *msgSent = (char *)malloc(sizeof(MSG_QUEUE_SIZE));
        sprintf(msgSent, "%d", semNum);
        printf("MESSAGE: sending message to giocatore -> %s\n", msgSent);
        sendMsgQueue(playerPid, msgSent);
        free(msgSent);
    }
    printf("Ricerca giocatori completata.\n");
}

void play()
{
    GameData *gameData = getGameData();

    // TODO to remove
    printf("-------------------------------------------\n");
    printf("BANCO [pid:%d, semnum:%d]\n", gameData->croupierPid, gameData->croupierSemNum);
    printf("      [croupierStartingMoney:%d]\n", gameData->croupierStartingMoney);
    printf("      [croupierCurrentMoney:%d]\n", gameData->croupierCurrentMoney);
    printf("      [totalPlayedGamesCount:%d]\n", gameData->totalPlayedGamesCount);
    printf("      [winnedGamesCount:%d]\n", gameData->winnedGamesCount);
    printf("      [losedGamesCount:%d]\n", gameData->losedGamesCount);
    printf("      [playersCount:%d]\n", gameData->playersCount);
    printf("      [playersData:%lu]\n", ARRSIZE(gameData->playersData));
    printf("      [actionType:%d]\n", gameData->actionType);
    printf("-------------------------------------------\n");
 
    int semId = getSemId();

    // start game
    nextPlayer(semId);
    pausePlayer(CROUPIER_SEM_NUM, semId);

    // TODO impostare uno stato di "nuovo player" nei dati giocatore: all'inizio metto tutti a nuovo e all'interno di questo for stampo solo quelli nuovi
    // TODO e sposto questo parte di codice nel WELCOME
    // presentation of players
    printf("Diamo il benvenuto ai %d giocatori: \n", gameData->playersCount);
    for (int i = 0; i < gameData->playersCount; i++)
    {
        PlayerData p = gameData->playersData[i];
        printf("- %s con un bugdet di %d %s\n", p.playerName, p.currentMoney, EXCHANGE);
    }
    printf("Che il gioco inizi!!! \n");
    printf("\n");
    gameData->actionType = BET;

    while (1)
    {
        switch (gameData->actionType)
        {
        case WELCOME:
        {
        }
        break;
        case BET:
        {
            gameData->totalPlayedGamesCount++;
            printf("---->  Match n. %d  <----\n", gameData->totalPlayedGamesCount);
            printf("I giocatori facciano la loro puntata:\n");
            nextPlayer(semId);
            pausePlayer(CROUPIER_SEM_NUM, semId);
            for (int i = 0; i < gameData->playersCount; i++)
            {
                PlayerData p = gameData->playersData[i];
                printf("- %-13s -> %d %s: %d %% di %d %s\n", p.playerName, p.currentBet, EXCHANGE, p.currentBetPercentage, p.currentMoney, EXCHANGE);
            }
            gameData->actionType = PLAY;
        }
        break;
        case PLAY:
        {
            printf("Tiro dei dati:\n");
            nextPlayer(semId);
            pausePlayer(CROUPIER_SEM_NUM, semId);

            int firstDiceResult = randomValue(gameData->croupierCurrentMoney * 10, 1, 6);
            int secondDiceResult = randomValue(gameData->croupierCurrentMoney * 100, 1, 6);
            int totalDiceResult = firstDiceResult + secondDiceResult;

            // showing dice result
            printf("- %-13s ->  %d e %d, totale %d\n", "banco", firstDiceResult, secondDiceResult, totalDiceResult);
            for (int i = 0; i < gameData->playersCount; i++)
            {
                PlayerData p = gameData->playersData[i];
                printf("- %-13s ->  %d e %d, totale %d\n", p.playerName, p.firstDiceResult, p.secondDiceResult, p.totalDiceResult);
            }

            // calculating data
            for (int i = 0; i < gameData->playersCount; i++)
            {
                PlayerData p = gameData->playersData[i];
                printf("- %-13s ->  %d e %d, totale %d\n", p.playerName, p.firstDiceResult, p.secondDiceResult, p.totalDiceResult);
            }

            pausePlayer(CROUPIER_SEM_NUM, semId);
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
}

void nextPlayer(int semId)
{
    setSem(CROUPIER_SEM_NUM + 1, semId, 1);
}
