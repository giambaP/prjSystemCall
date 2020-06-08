#include "gioco-lib.c"

#define REQUIRED_INPUT_PARAMS 2
#define MAX_DEFAULT_PLAYERS 1
#define CROUPIER_SEM_NUM 0
#define PLAYER_CROUPIER_MONEY_RATIO 4 // definisce il rapporto tra i soldi del croupier e di ogni giocatore

void connectPlayers(int maxPlayersCount);
void play();

int main(int argc, char *argv[])
{
    if (argc != 1 && argc != REQUIRED_INPUT_PARAMS)
    {
        fprintf(stderr, "Numero parametri input non corretto! Attesi: %d, Trovati: %d\n", REQUIRED_INPUT_PARAMS, argc);
        fprintf(stderr, "Programma terminato!\n");
        exit(1);
    }
    int maxPlayersCount = (argc == REQUIRED_INPUT_PARAMS ? atoi(argv[1]) : MAX_DEFAULT_PLAYERS);
    maxPlayersCount = maxPlayersCount > 0 ? maxPlayersCount : MAX_DEFAULT_PLAYERS;

    // TODO manage SIGNALS

    // setting game data
    int startingMoney = randomValue(CROUPIER_SEM_NUM, (int)MIN_INIT_PLAYER_MONEY, (int)MAX_INIT_PLAYER_MONEY) * PLAYER_CROUPIER_MONEY_RATIO;
    PlayerData *playersData = (PlayerData *)malloc(sizeof(PlayerData[maxPlayersCount]));
    GameData gameData;
    gameData.croupierPid = getpid();
    gameData.croupierSemNum = CROUPIER_SEM_NUM;
    gameData.croupierStartingMoney = startingMoney / 10 * 10; // TODO conti tondi per il momento
    gameData.croupierCurrentMoney = gameData.croupierStartingMoney;
    gameData.totalPlayedGamesCount = 0;
    gameData.winnedGamesCount = 0;
    gameData.losedGamesCount = 0;
    gameData.playersCount = maxPlayersCount;
    gameData.playersData = playersData;
    gameData.actionType = WELCOME;

    allocateShm(&gameData);
    allocateSem(maxPlayersCount + 1); // +1 -> croupier
    allocateMsgQueue();

    printTitle();
    connectPlayers(maxPlayersCount);

    play();

    // TODO manage error on signal
    unallocateMsgQueue();
    unallocateShm(false);
    unallocateSem(false);

    return 0;
}

void connectPlayers(int maxPlayersCount)
{
    printf("Ricerca %d giocatori...\n", maxPlayersCount);
    for (int semNum = 1; semNum <= maxPlayersCount; semNum++)
    {
        char *msgReceived = (char *)malloc(sizeof(char));
        receiveMsgQueue(MSG_TYPE_USER_MATCH, msgReceived);
        int playerPid = atoi(msgReceived);
        free(msgReceived);

        char *msgSent = (char *)malloc(sizeof(char));
        sprintf(msgSent, "%d", semNum);
        sendMsgQueue(playerPid, msgSent);
        free(msgSent);
    }
    printf("Ricerca giocatori completata.\n");
}

void play()
{
    GameData *gameData = getGameData();

    // setting presentation action to players
    printf("Diamo il benvenuto ai nostri giocatori!\n");

    int semId = getSemId();

    // start game
    printf("incremento semaforo %d di 1 (semId=%d\n", (CROUPIER_SEM_NUM + 1), semId);
    setSem(CROUPIER_SEM_NUM + 1, semId, 1);

    printf("-------------------------------------------\n");
    printf("BANCO [pid:%d, semnum:%d]\n", gameData->croupierPid, gameData->croupierSemNum);
    printf("      [croupierStartingMoney:%d]\n", gameData->croupierStartingMoney);
    printf("      [croupierCurrentMoney:%d]\n", gameData->croupierCurrentMoney);
    printf("      [totalPlayedGamesCount:%d]\n", gameData->totalPlayedGamesCount);
    printf("      [winnedGamesCount:%d]\n", gameData->winnedGamesCount);
    printf("      [losedGamesCount:%d]\n", gameData->losedGamesCount);
    printf("      [playersCount:%d]\n", gameData->playersCount);
    printf("      [playersData:%d]\n", ARRSIZE(gameData->playersData));
    printf("      [actionType:%d]\n", gameData->actionType);
    printf("-------------------------------------------\n");

    while (1)
    {
        pausePlayer(CROUPIER_SEM_NUM, semId);
        printf("MI SONO SVEGLIATO!!\n");
        // TODO
    }
}