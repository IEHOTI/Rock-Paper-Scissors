#include "client.h"

Client::Client(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Камень - Ножницы - Бумага");

    // Создание меню
    QMenuBar* menuBar = new QMenuBar(this);
    QAction* playInstruction = new QAction("Как играть?", this);
    QAction* aboutMe = new QAction("О программе", this);
    menuBar->addAction(playInstruction);
    menuBar->addAction(aboutMe);
    setMenuBar(menuBar);
    menuBar->setStyleSheet("background-color:#f0f0f0");

    // Подключение сигналов меню
    connect(playInstruction, &QAction::triggered, this, &Client::playInfo);
    connect(aboutMe, &QAction::triggered, this, &Client::aboutMyProgramm);

    // Загрузка изображений
    QStringList imagePaths = {":/data/miniRock.png",":/data/miniScissors.png",":/data/miniPaper.png",
                              ":/data/smallRock.png",":/data/smallScissors.png",":/data/smallPaper.png",
                              ":/data/Rock.png",":/data/Scissors.png",":/data/Paper.png",
                              ":/data/notReady.png",":/data/ready.png"};
    for(const QString& path : imagePaths) pixmap.append(QPixmap(path));

    // Установка возможных выборов
    choiceList << "Камень" << "Ножницы" << "Бумага";

    // Инициализация указателей
    start = false;
    user = nullptr;
    server = nullptr;
    interface = nullptr;
    playground = nullptr;
    tcpSocket = nullptr;
    udpSocket = nullptr;
    result = nullptr;

    // Установка главного виджета
    mainWidget = new QWidget(this);
    setCentralWidget(mainWidget);
    setMenuPage(); // Отображение стартового меню
}

Client::~Client() {
    // Очистка динамически выделенных объектов
    delete user;
    delete result;
    delete buttonStone;
    delete buttonScissors;
    delete buttonPaper;
    delete buttonBack;
    delete buttonStart;
    delete buttonPause;
    delete playground;
    delete interface;
    delete mainWidget;

    // Очистка контейнеров
    profiles.clear();
    players.clear();
    pixmap.clear();
    choiceList.clear();
    playerProfiles.clear();
    foundServers.clear();

    // Закрытие сетевых соединений
    if (udpSocket) {
        udpSocket->close();
        delete udpSocket;
    }
    if (tcpSocket) {
        tcpSocket->abort();
        delete tcpSocket;
    }
    if (scanTimer) {
        scanTimer->stop();
        delete scanTimer;
    }
    delete server;
}

void Client::setMenuPage() {
    if(tcpSocket != nullptr) disconnectSocket(); // Отключение сокета при наличии
    profiles.clear();
    players.clear();
    clearWidget(); // Очистка виджета
    start = false;
    resize(400,200);
    setFixedSize(400,200);
    setCentralWindow(400,200);

    // Создание макета и элементов меню
    QVBoxLayout* layout = new QVBoxLayout(mainWidget);
    QLabel* label = new QLabel("Камень - Ножницы - Бумага", mainWidget);
    label->setStyleSheet("font-size:26px;");
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout* buttonLayout = new QVBoxLayout();
    QPushButton* buttonSingle = new QPushButton("Одиночная игра", mainWidget);
    QPushButton* buttonMulti = new QPushButton("Мультиплеерная игра", mainWidget);

    buttonLayout->addWidget(buttonSingle);
    buttonLayout->addWidget(buttonMulti);

    layout->addStretch();
    layout->addWidget(label);
    layout->addLayout(buttonLayout);
    layout->addStretch();
    mainWidget->setLayout(layout);

    // Подключение сигналов кнопок
    connect(buttonSingle, &QPushButton::clicked, this, &Client::setSinglePage);
    connect(buttonMulti, &QPushButton::clicked, this, &Client::setMultiMenu);
}

void Client::setPlayground(QWidget* widget) {
    // Создание рамки игрового поля
    QFrame* topLine = new QFrame(widget);
    topLine->setFrameShape(QFrame::HLine);
    topLine->setFrameShadow(QFrame::Sunken);
    topLine->setLineWidth(3);
    topLine->setFixedHeight(3);
    topLine->setStyleSheet("background-color: black;");
    topLine->setGeometry(0, 50, 600, 3);

    QFrame* bottomLine = new QFrame(widget);
    bottomLine->setFrameShape(QFrame::HLine);
    bottomLine->setFrameShadow(QFrame::Sunken);
    bottomLine->setLineWidth(3);
    bottomLine->setFixedHeight(3);
    bottomLine->setStyleSheet("background-color: black;");
    bottomLine->setGeometry(0, 300, 600, 3);

    QFrame* rightLine = new QFrame(widget);
    rightLine->setFrameShape(QFrame::VLine);
    rightLine->setFrameShadow(QFrame::Sunken);
    rightLine->setLineWidth(3);
    rightLine->setFixedWidth(3);
    rightLine->setStyleSheet("background-color: black;");
    rightLine->setGeometry(597, 50, 3, 250);

    QFrame* leftLine = new QFrame(widget);
    leftLine->setFrameShape(QFrame::VLine);
    leftLine->setFrameShadow(QFrame::Sunken);
    leftLine->setLineWidth(3);
    leftLine->setFixedWidth(3);
    leftLine->setStyleSheet("background-color: black;");
    leftLine->setGeometry(0, 50, 3, 250);

    // Создание кнопок интерфейса
    buttonStone = new QPushButton(widget);
    buttonStone->setIcon(QIcon(pixmap[0]));
    buttonStone->setIconSize(QSize(76,56));
    buttonStone->setDisabled(true);
    buttonStone->setCheckable(true);
    buttonStone->setChecked(true);
    buttonStone->setGeometry(10, 310, 80, 60);

    buttonScissors = new QPushButton(widget);
    buttonScissors->setIcon(QIcon(pixmap[1]));
    buttonScissors->setIconSize(QSize(76,56));
    buttonScissors->setDisabled(true);
    buttonScissors->setCheckable(true);
    buttonScissors->setChecked(false);
    buttonScissors->setGeometry(110, 310, 80, 60);

    buttonPaper = new QPushButton(widget);
    buttonPaper->setIcon(QIcon(pixmap[2]));
    buttonPaper->setIconSize(QSize(76,56));
    buttonPaper->setDisabled(true);
    buttonPaper->setCheckable(true);
    buttonPaper->setChecked(false);
    buttonPaper->setGeometry(210, 310, 80, 60);

    buttonBack = new QPushButton(widget);
    buttonBack->setIcon(QIcon(QPixmap(":/data/Back.png")));
    buttonBack->setIconSize(QSize(40,40));
    buttonBack->setGeometry(420, 320, 40, 40);

    buttonPause = new QPushButton(widget);
    buttonPause->setIcon(QIcon(QPixmap(":/data/stop.png")));
    buttonPause->setIconSize(QSize(40,40));
    buttonPause->setGeometry(480, 320, 40, 40);
    buttonPause->setDisabled(true);

    buttonStart = new QPushButton(widget);
    buttonStart->setIcon(QIcon(QPixmap(":/data/start.png")));
    buttonStart->setIconSize(QSize(40,40));
    buttonStart->setGeometry(540, 320, 40, 40);

    // Создание строки вывода результата игры
    result = new QLabel("Ожидание игры", widget);
    result->setGeometry(150, 0, 300, 50);
    result->setStyleSheet("font-size: 25px;");
    result->setAlignment(Qt::AlignCenter);
}

void Client::setUserPlayground(QVector<QWidget*>& widgets,int k) {

    playground->setGeometry(0,50,600,250); // Установка расположения игрового поля

    // Алгоритм размещения игровых профилей на поле
    QRandomGenerator *generator = QRandomGenerator::global();
    for(int i = 0,j = 0, x = 0, y = 0;;) {
        if(generator->bounded(0,2) == 0 && i < widgets.size()) {
            widgets[i]->move(x,y);
            if(x == 400) {
                if(y == 125) y = 0;
                else y = 125;
                x = 0;
            }
            else x+= 200;
            i++;
        }
        else if(i+j == k) break;
        else {
            if(j == k-widgets.size()) continue;
            QWidget* widget = new QWidget(playground);
            createWaitWidget(widget); // Создание "пустого слота" при наличии свободных мест
            widget->move(x,y);
            if(x == 400){
                if(y == 125) y = 0;
                else y = 125;
                x = 0;
            }
            else x+= 200;
            j++;
        }
    }
}

void Client::createUserWidget(QWidget* widget, Player player) {
    clearWidget(widget); // Очистка виджета перед созданием
    widget->resize(200,125);

    // Создание рамок виджета
    QFrame* rightLine = new QFrame(widget);
    rightLine->setFrameShape(QFrame::VLine);
    rightLine->setFrameShadow(QFrame::Sunken);
    rightLine->setLineWidth(3);
    rightLine->setFixedWidth(3);
    rightLine->setStyleSheet("background-color: black;");
    rightLine->setGeometry(197, 0, 3, 125);

    QFrame* leftLine = new QFrame(widget);
    leftLine->setFrameShape(QFrame::VLine);
    leftLine->setFrameShadow(QFrame::Sunken);
    leftLine->setLineWidth(3);
    leftLine->setFixedWidth(3);
    leftLine->setStyleSheet("background-color: black;");
    leftLine->setGeometry(0, 0, 3, 125);

    QFrame* topLine = new QFrame(widget);
    topLine->setFrameShape(QFrame::HLine);
    topLine->setFrameShadow(QFrame::Sunken);
    topLine->setLineWidth(3);
    topLine->setFixedHeight(3);
    topLine->setStyleSheet("background-color: black;");
    topLine->setGeometry(0, 0, 200, 3);

    QFrame* botLine = new QFrame(widget);
    botLine->setFrameShape(QFrame::HLine);
    botLine->setFrameShadow(QFrame::Sunken);
    botLine->setLineWidth(3);
    botLine->setFixedHeight(3);
    botLine->setStyleSheet("background-color: black;");
    botLine->setGeometry(0, 122, 200, 3);

    // Заполнение виджета данными
    QLabel* username = new QLabel(widget);
    username->setGeometry(0,0,160,28);
    if(user != nullptr && player.getName() == user->getName()) username->setText("<div style='color:green; font-size: 20px;'>" + player.getName() + "</div>");
    else if(user == nullptr && player.getName() == "Вы") username->setText("<div style='color:green; font-size: 20px;'>" + player.getName() + "</div>");
    else username->setText("<div style='color:black; font-size: 20px;'>" + player.getName() + "</div>");
    username->setAlignment(Qt::AlignCenter);

    QLabel* userHand = new QLabel(widget);
    userHand->setPixmap(pixmap[3]);
    userHand->setGeometry(3, 30, 170, 90);
    userHand->setAlignment(Qt::AlignCenter);

    QLabel* userScore = new QLabel(widget);
    userScore->setGeometry(165,0,35,35);
    userScore->setText("<div style='font-size: 20px;'>" + QString::number(player.getScore()) + "</div>");
    userScore->setAlignment(Qt::AlignCenter);

    QLabel* userReady = new QLabel(widget);
    userReady->setPixmap(pixmap[9]);
    userReady->setGeometry(173, 98, 22, 22);
    userReady->setAlignment(Qt::AlignCenter);
}

void Client::createWaitWidget(QWidget* widget) {
    clearWidget(widget);  // Очистка виджета перед созданием
    widget->resize(200,125);

    // Создание рамок виджета
    QFrame* rightLine = new QFrame(widget);
    rightLine->setFrameShape(QFrame::VLine);
    rightLine->setFrameShadow(QFrame::Sunken);
    rightLine->setLineWidth(3);
    rightLine->setFixedWidth(3);
    rightLine->setStyleSheet("background-color: black;");
    rightLine->setGeometry(197, 0, 3, 125);

    QFrame* leftLine = new QFrame(widget);
    leftLine->setFrameShape(QFrame::VLine);
    leftLine->setFrameShadow(QFrame::Sunken);
    leftLine->setLineWidth(3);
    leftLine->setFixedWidth(3);
    leftLine->setStyleSheet("background-color: black;");
    leftLine->setGeometry(0, 0, 3, 125);

    QFrame* topLine = new QFrame(widget);
    topLine->setFrameShape(QFrame::HLine);
    topLine->setFrameShadow(QFrame::Sunken);
    topLine->setLineWidth(3);
    topLine->setFixedHeight(3);
    topLine->setStyleSheet("background-color: black;");
    topLine->setGeometry(0, 0, 200, 3);

    QFrame* botLine = new QFrame(widget);
    botLine->setFrameShape(QFrame::HLine);
    botLine->setFrameShadow(QFrame::Sunken);
    botLine->setLineWidth(3);
    botLine->setFixedHeight(3);
    botLine->setStyleSheet("background-color: black;");
    botLine->setGeometry(0, 123, 200, 3);

    // Заполнение виджета
    QLabel* label = new QLabel(widget);
    label->setGeometry(0,0,200,85);
    label->setText("<div style='color:black; font-size: 30px;'>Пустой слот</div>");
    label->setAlignment(Qt::AlignCenter);

    QPushButton* clickMe = new QPushButton("Занять слот",widget);
    clickMe->setGeometry(18.5,90,163,30);

    // Подключение сигнала смены местополжения игрока
    connect(clickMe,&QPushButton::clicked,[=]() {
        clickMe->setDisabled(true);
        QPoint wait = widget->pos();

        if(tcpSocket != nullptr) sendMessage("ChangePlace\n" + QString::number(userProfile.id) + "\n" + QString::number(wait.x()) + "\n" + QString::number(wait.y()) + "\n");

        clickMe->setEnabled(true);
    });
}

void Client::updateUserWidget(QWidget* widget, playerProfile newUser) {
    clearWidget(widget);  // Очистка виджета перед созданием

    if(newUser.player.getName() == "") {
        createWaitWidget(widget); // Создание пустого виджета

        widget->setGeometry(newUser.x,newUser.y,200,125);
        widget->update();
        widget->show();

        for (auto child : widget->findChildren<QWidget*>()) child->show(); // Обновление показа виджета и его компонентов
        return;
    }

    widget->setGeometry(newUser.x,newUser.y,200,125);
    widget->update();

    // Создание рамок виджета
    QFrame* rightLine = new QFrame(widget);
    rightLine->setFrameShape(QFrame::VLine);
    rightLine->setFrameShadow(QFrame::Sunken);
    rightLine->setLineWidth(3);
    rightLine->setFixedWidth(3);
    rightLine->setStyleSheet("background-color: black;");
    rightLine->setGeometry(197, 0, 3, 125);

    QFrame* leftLine = new QFrame(widget);
    leftLine->setFrameShape(QFrame::VLine);
    leftLine->setFrameShadow(QFrame::Sunken);
    leftLine->setLineWidth(3);
    leftLine->setFixedWidth(3);
    leftLine->setStyleSheet("background-color: black;");
    leftLine->setGeometry(0, 0, 3, 125);

    QFrame* topLine = new QFrame(widget);
    topLine->setFrameShape(QFrame::HLine);
    topLine->setFrameShadow(QFrame::Sunken);
    topLine->setLineWidth(3);
    topLine->setFixedHeight(3);
    topLine->setStyleSheet("background-color: black;");
    topLine->setGeometry(0, 0, 200, 3);

    QFrame* botLine = new QFrame(widget);
    botLine->setFrameShape(QFrame::HLine);
    botLine->setFrameShadow(QFrame::Sunken);
    botLine->setLineWidth(3);
    botLine->setFixedHeight(3);
    botLine->setStyleSheet("background-color: black;");
    botLine->setGeometry(0, 123, 200, 3);

    // Заполнение виджета данными
    QLabel* username = new QLabel(widget);
    username->setGeometry(0,0,160,28);

    if(user != nullptr) {
        if(newUser.player.getName() == user->getName()) username->setText("<div style='color:green; font-size: 20px;'>" + newUser.player.getName() + "</div>");
        else username->setText("<div style='color:black; font-size: 20px;'>" + newUser.player.getName() + "</div>");
    }
    else {
        if(newUser.player.getName() == "Вы") username->setText("<div style='color:green; font-size: 20px;'>" + newUser.player.getName() + "</div>");
        else username->setText("<div style='color:black; font-size: 20px;'>" + newUser.player.getName() + "</div>");
    }
    username->setAlignment(Qt::AlignCenter);

    QLabel* userHand = new QLabel(widget);
    userHand->setPixmap(pixmap[newUser.player.getChoice() + 3]);
    userHand->setGeometry(3, 30, 170, 90);
    userHand->setAlignment(Qt::AlignCenter);

    QLabel* userScore = new QLabel(widget);
    userScore->setGeometry(165,0,35,35);
    userScore->setText("<div style='font-size: 20px;'>" + QString::number(newUser.player.getScore()) + "</div>");
    userScore->setAlignment(Qt::AlignCenter);

    QLabel* userReady = new QLabel(widget);
    if(newUser.player.getReady()) userReady->setPixmap(pixmap[10]);
    else userReady->setPixmap(pixmap[9]);
    userReady->setGeometry(173, 98, 22, 22);
    userReady->setAlignment(Qt::AlignCenter);

    widget->move(QPoint(newUser.x,newUser.y));
    for (auto child : widget->findChildren<QWidget*>()) child->show();  // Обновление показа виджета и его компонентов
}

void Client::updatePlayground() {
    playground->setGeometry(0,50,600,250);  // Установка местоположения игрового поля

    for(int i = 0; i < players.size();i++) updateUserWidget(profiles[i],playerProfiles[i]); // Обновление профилей игроков

    playground->update();
    playground->show();
}

void Client::changeLabel(std::string str) {
    if(!result) return;
    result->setText(QString::fromStdString(str));
}

void Client::pressedStart() {
    // Изменение состояний кнопок
    buttonStart->setDisabled(true);
    buttonPause->setEnabled(true);

    buttonStone->setEnabled(true);
    buttonScissors->setEnabled(true);
    buttonPaper->setEnabled(true);

    buttonStone->setChecked(true);
    buttonScissors->setChecked(false);
    buttonPaper->setChecked(false);
}

void Client::pressedPause() {
    // Изменение состояний кнопок
    buttonStart->setEnabled(true);
    buttonPause->setDisabled(true);

    buttonStone->setDisabled(true);
    buttonScissors->setDisabled(true);
    buttonPaper->setDisabled(true);
}

void Client::startGame(bool draw) {
    // Изменение состояний кнопок
    buttonStart->setDisabled(true);
    buttonPause->setEnabled(true);

    buttonStone->setEnabled(true);
    buttonScissors->setEnabled(true);
    buttonPaper->setEnabled(true);

    // Начало игры
    for(int i = 0; i < 3; i++) {
        if(draw) break;
        if(!start || (tcpSocket != nullptr && tcpSocket->bytesAvailable()>0)) return;

        changeLabel(choiceList[i].toStdString());
        myPause(750);
    }

    for(int i = 0; i < 3; i++) {
        if(!start || (tcpSocket != nullptr && tcpSocket->bytesAvailable()>0)) return;

        changeLabel(QString::number(i+1).toStdString());
        myPause(500);
    }

    // Отправка результатов по окончанию игры
    if(tcpSocket != nullptr) {
        if(buttonStone->isChecked()) user->selectChoice(0);
        if(buttonScissors->isChecked()) user->selectChoice(1);
        if(buttonPaper->isChecked()) user->selectChoice(2);
        sendMessage("SelectChoice\n" + QString::number(userProfile.id) + "\n" + QString::number(user->getChoice()) + "\n");
    }
    // Если хост - отправка сигнала об окончании игры
    if(server != nullptr) {
        myPause(1000);
        sendPause();
    }
}

//utilities
void Client::clearWidget(){
    if (!mainWidget) return;

    delete playground;
    playground = nullptr;
    delete interface;
    interface = nullptr;

    QList<QWidget*> childWidgets = mainWidget->findChildren<QWidget*>();
    //Очищение всех внутренних виджетов
    for (QWidget* child : childWidgets){
        child->setVisible(false);
        child->deleteLater();
    }

    // Очищение лэйаута
    if (mainWidget->layout()) {
        QLayout* layout = mainWidget->layout();
        while (QLayoutItem* item = layout->takeAt(0)) {
            if (QWidget* childWidget = item->widget()){
                childWidget->setVisible(false);
                childWidget->deleteLater();
            }
            delete item;
        }
        delete layout;
    }
}

void Client::clearWidget(QWidget* widget){
    if (!widget) return;

    QList<QWidget*> childWidgets = widget->findChildren<QWidget*>();
    //Очищение всех внутренних виджетов
    for (QWidget* child : childWidgets){
        child->setParent(nullptr);
        delete child;
    }

    //Очищение лэйаута
    if (widget->layout()) {
        QLayout* layout = widget->layout();
        while (QLayoutItem* item = layout->takeAt(0)) {
            if (QWidget* childWidget = item->widget()){
                delete childWidget;
            }
            delete item;
        }
        delete layout;
    }
}

void Client::setCentralWindow(int width, int height){
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();

    int x = (screenGeometry.width() - width) / 2;
    int y = (screenGeometry.height() - height) / 2;

    this->move(x, y);
}

void Client::myPause(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

void Client::aboutMyProgramm() {
    QMessageBox::about(this, "О программе",
                       "<h3>Камень-Ножницы-Бумага</h3>"
                       "<p>Версия: 1.0</p>"
                       "<p>Разработчик: Волков Никита</p>"
                       "<p>Лицензия: LGPL</p>"
                       "<p>Этот проект распространяется под лицензией GNU LGPL. Это означает, что вы можете "
                       "использовать и распространять программу свободно, при условии соблюдения условий LGPL.</p>"
                       "<p><a href='https://www.gnu.org/licenses/lgpl-3.0.html'>Подробнее о лицензии LGPL</a></p>"
                       );
}

void Client::playInfo() {
    QMessageBox::information(this, "Как играть",
                             "Правила игры:\n\n"
                             "1. Выберите один из вариантов: Камень, Ножницы или Бумага.\n"
                             "2. Камень побеждает Ножницы, Ножницы побеждают Бумагу, Бумага побеждает Камень.\n"
                             "3. Если оба игрока выбрали одно и то же, это ничья.\n\n"
                             "Удачи в игре!\n\n"
                             "Как играть по сети:\n"
                             "При нажатии на кнопку \"Мультиплеерная игра\" откроется меню с полем ввода вашего игрового имени и навигационными кнопками. Играть можно только по локальной сети (Radmin VPN вам в помощь, это не реклама).\n"
                             "\nКак создать игру?\n"
                             "Для создания игры необходимо нажать на кнопку \"Создать игру\", затем ввести желаемое название комнаты или оставить текущее, выбрать количество мест в комнате и нажать на кнопку \"Создать игру\".\n"
                             "\nКак присоединиться к игре?\n"
                             "Для присоединения к игре необходимо нажать на кнопку \"Присоединиться к игре\", выбрать необходимый сервер и нажать на кнопку \"Подключиться\" либо дважды кликнуть по выбранному серверу.\n"
                             "Если вы не нашли необходимый сервер попробуйте нажать на кнопку обновления списка серверов в правом нижнем углу, а также проверьте нахождение обоих устройств в локальной сети."
                             );
}
