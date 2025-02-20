#include "client.h"

void Client::setMultiMenu() {
    if(udpSocket == nullptr) { // Если сокет не создан, создаём его
        udpSocket = new QUdpSocket(this);
        scanTimer = new QTimer(this);

        if (!udpSocket->bind(QHostAddress::AnyIPv4, 45455, QUdpSocket::ShareAddress)) return;

        connect(udpSocket, &QUdpSocket::readyRead, this, &Client::processPendingDatagrams);
        connect(scanTimer, &QTimer::timeout, this, &Client::sendUdpRequest);
    }

    if(server != nullptr) { // Если сервер был открыт, закрываем его
        delete server;
        server = nullptr;
    }
    // Очистка старых данных
    profiles.clear();
    players.clear();

    // Создание меню мультиплеера
    clearWidget();
    resize(280,260);
    setFixedSize(280,260);
    setCentralWindow(280,260);

    QVBoxLayout* layout = new QVBoxLayout(mainWidget);
    QWidget* widget = new QWidget(mainWidget);

    QLabel* label = new QLabel("Введите игровое имя",widget);
    label->setGeometry(0,0,280,30);
    label->setStyleSheet("font-size:20px;");
    label->setAlignment(Qt::AlignCenter);

    QLineEdit* username = new QLineEdit(widget);
    username->setGeometry(20,40,240,30);
    QRandomGenerator *generator = QRandomGenerator::global();
    int rand = generator->bounded(0,1001);
    if(user != nullptr) username->setText(user->getName());
    else username->setPlaceholderText("Player " + QString::number(rand));

    // Создание кнопок
    QPushButton* joinGame = new QPushButton("Присоединиться к игре",widget);
    joinGame->setGeometry(55,90,170,30);

    QPushButton* createGame = new QPushButton("Создать игру",widget);
    createGame->setGeometry(55,140,170,30);

    QPushButton* menuButton = new QPushButton("Назад в меню",widget);
    menuButton->setGeometry(55,190,170,30);

    //Подключение сигналов к кнопкам
    connect(joinGame, &QPushButton::clicked, this, [=]() {
        sendUdpRequest();
        scanTimer->start(100);
        if(username->text() == "") username->setText("Player " + QString::number(rand));
        QString str = username->text();
        user = new Player(str);
        setJoinMenu(); // Переход на меню подключения к игре
    });

    connect(createGame, &QPushButton::clicked, this, [=]() {
        if(username->text() == "") username->setText("Player " + QString::number(rand));
        QString str = username->text();
        user = new Player(str);
        setCreateMenu(); // Переход на меню создания игры
    });

    connect(menuButton,&QPushButton::clicked,this,&Client::setMenuPage);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(widget);
    mainWidget->setLayout(layout);
}

void Client::setJoinMenu() {
    // Создание меню подключения к игре
    clearWidget();
    resize(280,220);
    setFixedSize(280,220);
    setCentralWindow(280,220);

    QVBoxLayout* layout = new QVBoxLayout(mainWidget);
    QWidget* widget = new QWidget(mainWidget);

    QListWidget* list = new QListWidget(widget);
    list->setGeometry(5,5,270,145);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    receiveServerNames(list);

    // Создание кнопок
    buttonBack = new QPushButton(widget);
    buttonBack->setIcon(QIcon(QPixmap(":/data/Back.png")));
    buttonBack->setIconSize(QSize(40,40));
    buttonBack->setGeometry(5, 155, 40, 40);

    // Переподключение кнопки возврата домой на нужное меню
    disconnect(buttonBack,&QPushButton::clicked,this,nullptr);
    connect(buttonBack,&QPushButton::clicked,this,&Client::setMultiMenu);

    QPushButton* connected = new QPushButton("Подключиться",widget);
    connected->setEnabled(false);
    connected->setGeometry(90,155,100,40);

    connect(list,&QListWidget::itemClicked,[=]() {
        connected->setEnabled(true);
    });
    connect(list,&QListWidget::itemDoubleClicked,connected,&QPushButton::clicked);

    connect(connected,&QPushButton::clicked,this,[=]() {
        int index = list->currentRow();

        if(index > -1) connectToServer(foundServers[index]);

        if(isConnected) { // При подключении отправляем на сервер информацию о себе и подключаемся к комнате
            playerProfile temp = {0,0,0,*user};
            sendMessage(temp);
            createRoom();
        }
    });

    QPushButton* refresh = new QPushButton(widget);
    refresh->setIcon(QIcon(QPixmap(":/data/refresh.png")));
    refresh->setIconSize(QSize(36,36));
    refresh->setGeometry(235, 155, 40, 40);

    connect(refresh, &QPushButton::clicked, [=]() {
        list->clear();
        list->setGeometry(5,5,270,145);
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        receiveServerNames(list); // Повторное получение списка серверов
        foundServers.clear(); // Очищение старого списка для получения нового списка серверов перед следующим обновлением
        sendUdpRequest();
        scanTimer->start(100);
    });

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(widget);
    mainWidget->setLayout(layout);
}

void Client::setCreateMenu() {
    // Создание меню создания игровой сессии
    clearWidget();
    resize(280,190);
    setFixedSize(280,190);
    setCentralWindow(280,190);

    QVBoxLayout* layout = new QVBoxLayout(mainWidget);
    QWidget* widget = new QWidget(mainWidget);

    QLabel* mainLabel = new QLabel("Введите название комнаты",widget);
    mainLabel->setGeometry(5,5,280,30);
    mainLabel->setStyleSheet("font-size:18px");
    mainLabel->setAlignment(Qt::AlignCenter);

    QLineEdit* username = new QLineEdit(widget);
    username->setGeometry(5,45,270,30);
    username->setText(user->getName());

    QLabel* label = new QLabel("Выберите количество мест в комнате",widget);
    label->setGeometry(5,90,240,30);
    label->setAlignment(Qt::AlignCenter);

    // Создание кнопок
    QPushButton* button = new QPushButton("2", widget);
    button->setGeometry(245,90,30,30);

    // Создаем меню для кнопки
    QMenu* menu = new QMenu(button);

    // Добавляем действия (цифры от 2 до 6) в меню
    for (int i = 2; i <= 6; ++i) {
        QAction* action = menu->addAction(QString::number(i));
        // Подключаем действие к слоту, который обновляет текст кнопки
        connect(action, &QAction::triggered, [button, i]() {
            button->setText(QString::number(i));
        });
    }

    // Устанавливаем меню для кнопки
    button->setMenu(menu);

    buttonBack = new QPushButton(widget);
    buttonBack->setIcon(QIcon(QPixmap(":/data/Back.png")));
    buttonBack->setIconSize(QSize(40,40));
    buttonBack->setGeometry(5, 120, 40, 40);

    // Переподключаем кнопку возврата на нужное меню
    disconnect(buttonBack,&QPushButton::clicked,this,nullptr);
    connect(buttonBack,&QPushButton::clicked,this,&Client::setMultiMenu);

    QPushButton* createGame = new QPushButton("Создать игру",widget);
    createGame->setGeometry(60,130,210,30);

    // Подключаем сигнал создания игровой сессии
    connect(createGame,&QPushButton::clicked,[=]() {
        QString str = username->text();
        if(str == "") str = user->getName();
        int max = button->text().toInt();
        server = new Server({str,1,max}); // Создание сервера
        connectToServer(server->getIP()); // Подключение к серверу
        if(isConnected) {
            playerProfile temp = {0,0,0,*user};
            sendMessage(temp);
            createRoom();
        }

    });

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(widget);
    mainWidget->setLayout(layout);
}

void Client::createRoom(){
    // Создание игровой комнаты
    clearWidget();
    resize(600,400);
    setFixedSize(600,400);
    setCentralWindow(600,400);

    QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);

    interface = new QWidget(mainWidget);
    setPlayground(interface);
    playground = new QWidget(interface);

    while(profiles.size()==0) myPause(300);

    setUserPlayground(profiles,profiles.size()); // Добавление в игровое поле пользователей
    updatePlayground(); // Обновление отображения игрового поля

    // Подключение сигналов к кнопкам
    connect(buttonStone,&QPushButton::clicked,[=]() {
        buttonStone->setChecked(true);
        buttonScissors->setChecked(false);
        buttonPaper->setChecked(false);
        user->selectChoice(0);
    });
    connect(buttonScissors,&QPushButton::clicked,[=]() {
        buttonStone->setChecked(false);
        buttonScissors->setChecked(true);
        buttonPaper->setChecked(false);
        user->selectChoice(1);
    });
    connect(buttonPaper,&QPushButton::clicked,[=]() {
        buttonStone->setChecked(false);
        buttonScissors->setChecked(false);
        buttonPaper->setChecked(true);
        user->selectChoice(2);

    });
    connect(buttonBack, &QPushButton::clicked, this, &Client::disconnectSocket);
    connect(buttonPause, &QPushButton::clicked, [=]() {
        start = false;
        user->changeReady(false);
        sendMessage("ChangeReady\n" + QString::number(userProfile.id) + "\n" + QString::number(user->getReady()) + "\n");
        pressedPause();
        playground->setEnabled(true);
    });
    connect(buttonStart, &QPushButton::clicked,[=]() {
        user->changeReady(true);
        pressedStart();
        sendMessage("ChangeReady\n" + QString::number(userProfile.id) + "\n" + QString::number(user->getReady()) + "\n");
        playground->setDisabled(true);
    });

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(interface);
    mainWidget->setLayout(mainLayout);
}


void Client::receiveServerNames(QListWidget* list) {
    if(foundServers.size() == 0) return;

    QUdpSocket* udpSocketInfo = new QUdpSocket(this); // Создаем отдельный сокет для получения информации о сервере
    if (!udpSocketInfo->bind(QHostAddress::AnyIPv4, 45456)) {
        udpSocketInfo->deleteLater();
        return;
    }

    // Подключаем новый сокет в программу
    connect(udpSocketInfo, &QUdpSocket::readyRead, this, [udpSocketInfo,list, this]() {
        QSet<QHostAddress> receivedServers; // Храним уже обработанные IP, чтобы получать только одно сообщение от каждого
        while (receivedServers.size() < foundServers.size()) {
            if(!udpSocketInfo->hasPendingDatagrams()) continue;
            QByteArray datagram;
            datagram.resize(udpSocketInfo->pendingDatagramSize());

            QHostAddress senderAddress;
            quint16 senderPort;

            udpSocketInfo->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);

            if (!receivedServers.contains(senderAddress)) {  // Если от этого IP ещё не получали
                QDataStream stream(&datagram, QIODevice::ReadOnly);
                QString name;
                int currentCount, maxCount;
                stream >> name >> currentCount >> maxCount;

                // Получаем количество игроков в виде строки "1/4"
                QString playersCount = QString("%1/%2").arg(currentCount).arg(maxCount);

                // Создаем объект QFontMetrics для измерения ширины текста
                QFontMetrics metrics(QApplication::font());

                // Получаем доступную ширину для имени сервера (из оставшегося места)
                int widgetWidth = list->viewport()->width();
                int playersCountWidth = metrics.horizontalAdvance(playersCount);
                int availableWidth = widgetWidth - playersCountWidth - 20; // Отнимаем место для количества игроков и небольшое отступление

                // Сокращаем текст имени сервера, если он не помещается
                QString elidedName = metrics.elidedText(name, Qt::ElideRight, availableWidth);

                // Формируем строку, которая будет отображаться в списке
                QString formattedString = QString("%1     %2").arg(elidedName, playersCount);

                QListWidgetItem* item = new QListWidgetItem(formattedString);
                item->setSizeHint(QSize(list->width(), item->sizeHint().height()));
                list->addItem(item);

                receivedServers.insert(senderAddress); // Запоминаем, что от этого IP уже получили
            }
        }
        udpSocketInfo->close();
        udpSocketInfo->deleteLater();// Удаляем сокет после получения информации о серверах
    });
}
