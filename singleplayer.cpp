#include "client.h"

void Client::setSinglePage() {
    // Очищение данных
    profiles.clear();
    players.clear();
    playerProfiles.clear();
    // Создание одиночной игры
    clearWidget();
    resize(600,400);
    setFixedSize(600,400);
    setCentralWindow(600,400);

    QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
    interface = new QWidget(mainWidget);

    setPlayground(interface);
    playground = new QWidget(interface);

    // Создание профиля пользователя и компьютера
    QWidget* my = new QWidget(playground);
    Player* myPlayer = new Player("Вы");
    myPlayer->changeReady(true);
    createUserWidget(my,*myPlayer);
    profiles.append(my);
    players.append(*myPlayer);
    delete myPlayer;

    QWidget* enemy = new QWidget(playground);
    Player* enemyPlayer = new Player("Компьютер");
    enemyPlayer->changeReady(true);
    createUserWidget(enemy,*enemyPlayer);
    profiles.append(enemy);
    players.append(*enemyPlayer);
    delete enemyPlayer;

    setUserPlayground(profiles); // Заполнение игрового поля

    // Сохранение профилей
    playerProfiles.append({0,profiles[0]->x(),profiles[0]->y(),players[0]});
    playerProfiles.append({1,profiles[1]->x(),profiles[1]->y(),players[1]});

    // Подключение кнопок
    connect(buttonStone,&QPushButton::clicked,[=]() {
        playerProfiles[0].player.selectChoice(0);
        buttonStone->setChecked(true);
        buttonScissors->setChecked(false);
        buttonPaper->setChecked(false);
    });
    connect(buttonScissors,&QPushButton::clicked,[=]() {
        playerProfiles[0].player.selectChoice(1);
        buttonStone->setChecked(false);
        buttonScissors->setChecked(true);
        buttonPaper->setChecked(false);
    });
    connect(buttonPaper,&QPushButton::clicked,[=]() {
        playerProfiles[0].player.selectChoice(2);
        buttonStone->setChecked(false);
        buttonScissors->setChecked(false);
        buttonPaper->setChecked(true);
    });
    connect(buttonBack, &QPushButton::clicked, this, &Client::setMenuPage);
    connect(buttonPause, &QPushButton::clicked, [=]() {
        start = false;
        pressedPause();
        changeLabel("Ожидание игрока..");
        playground->setEnabled(true);
        playerProfiles[0].player.changeReady(false);
        playerProfiles[0].player.selectChoice(0);
        updateUserWidget(profiles[0],playerProfiles[0]);
    });
    connect(buttonStart, &QPushButton::clicked,[=]() {
        bool draw = false;
        // Начало игры до победы или поражения
        do{
            // Подготовка
            pressedStart();
            playerProfiles[0].player.changeReady(true);
            playerProfiles[1].player.selectChoice(0);
            updatePlayground();
            playground->setEnabled(false);
            // Начало игры
            start = true;
            startGame(draw);
            if(!start) return;
            // Выдача рандомного выбора компьютеру
            QRandomGenerator *generator = QRandomGenerator::global();
            int rand = generator->bounded(0,3);
            playerProfiles[1].player.selectChoice(rand);
            // Подсчёт и вывод результатов
            if(playerProfiles[0].player.getChoice() == 0 && playerProfiles[1].player.getChoice() == 1){
                changeLabel("Победа");
                playerProfiles[0].player.addScore();
                draw = false;
            } else if(playerProfiles[0].player.getChoice() == 0 && playerProfiles[1].player.getChoice() == 0){
                changeLabel("Ничья");
                draw = true;
            } else if(playerProfiles[0].player.getChoice() == 0 && playerProfiles[1].player.getChoice() == 2){
                changeLabel("Поражение");
                playerProfiles[1].player.addScore();
            } else if(playerProfiles[0].player.getChoice() == 1 && playerProfiles[1].player.getChoice() == 1){
                changeLabel("Ничья");
                draw = true;
            } else if(playerProfiles[0].player.getChoice() == 1 && playerProfiles[1].player.getChoice() == 0){
                changeLabel("Поражение");
                playerProfiles[1].player.addScore();
                draw = false;
            } else if(playerProfiles[0].player.getChoice() == 1 && playerProfiles[1].player.getChoice() == 2){
                changeLabel("Победа");
                playerProfiles[0].player.addScore();
                draw = false;
            } else if(playerProfiles[0].player.getChoice() == 2 && playerProfiles[1].player.getChoice() == 1){
                changeLabel("Поражение");
                playerProfiles[1].player.addScore();
                draw = false;
            } else if(playerProfiles[0].player.getChoice() == 2 && playerProfiles[1].player.getChoice() == 0){
                changeLabel("Победа");
                playerProfiles[0].player.addScore();
                draw = false;
            } else if(playerProfiles[0].player.getChoice() == 2 && playerProfiles[1].player.getChoice() == 2){
                changeLabel("Ничья");
                draw = true;
            }

            // Обновление поля
            playground->setEnabled(true);
            playerProfiles[0].player.changeReady(false);
            updatePlayground();

            if(draw) myPause(1500);
            else pressedPause();
            playerProfiles[0].player.selectChoice(0);

        } while(draw);
    });

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(interface);
    mainWidget->setLayout(mainLayout);
}
