#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTimer>
#include <QPixmap>
#include <QLabel>
#include <QListWidget>
#include <QCheckBox>
#include <QScreen>
#include <QGuiApplication>
#include <QRandomGenerator>
#include <QEventLoop>
#include <QStringList>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QHostAddress>
#include <QFontMetrics>
#include <QApplication>
#include <string>

#include "player.h"
#include "server.h"

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    ~Client();

    //general
    void setMenuPage(); // Создаёт главное меню

    void setPlayground(QWidget* widget); // Создаёт игровое поле
    void setUserPlayground(QVector<QWidget*>& widgets,int k = 2); // Размещение игроков на поле, по умолчанию 2 игрока
    void updatePlayground(); // Обновляет игровое поле

    void createUserWidget(QWidget* widget, Player player); // Создает профиль выбранного игрока
    void createWaitWidget(QWidget* widget); // Создаёт пустой профиль
    void updateUserWidget(QWidget* widget, playerProfile newUser); // Обновляет профиль выбранного игрока

    void pressedStart(); // Прожата кнопка старта
    void pressedPause(); // Прожата кнопка паузы
    void startGame(bool draw = false); // Начинается игра

    //utilities
    void changeLabel(std::string str); // Меняет строку результата
    void setCentralWindow(int width,int height); // Центрирует окно
    void clearWidget(); // Очищает главный виджет
    void clearWidget(QWidget* widget); // Очищает выбранный виджет
    void myPause(int milliseconds = 1000); // Пауза
    void playInfo(); // Как играть
    void aboutMyProgramm(); // О программе

    //singleplayer
    void setSinglePage(); // Создание одиночной игры против компьютера

    //multiplayer
    void setMultiMenu(); // Создание меню многопользовательской игры
    void setCreateMenu(); // Создание меню создания игровой сессии
    void setJoinMenu(); // Создание меню выбора игровой сессии
    void createRoom(); // Создание игровой комнаты
    void receiveServerNames(QListWidget* list); // Получение списка серверов
    void swapUsers(QPoint user1,QString username1,  QPoint user2, QString username2); // Перемещение пользователя на выбранны свободный слот
    int findPlayer(int id); // Нахождение индекса игрока по его id

    //Сетевые функции
    void sendUdpRequest();
    void processPendingDatagrams();
    void connectToServer(const QHostAddress &address);
    void sendMessage(const QString& message);
    void sendMessage(const playerProfile &profile);
    void sendPause();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void disconnectSocket();

private:
    QVector<QPixmap> pixmap; // Коллекция используемых изображений
    QVector<QString> choiceList; // Коллекция текста для результатной строки
    QVector<QWidget*> profiles; // Коллекция виджет-профилей игроков
    QVector<Player> players; // Коллекция аккаунтов игроков
    QVector<playerProfile> playerProfiles; // Коллекция игровых профилей игроков

    Player* user; // Локальный пользователь
    playerProfile userProfile; // Локальный игровой профиль
    QWidget* playground; // Виджет игрового поля
    QWidget* interface; // Виджет игрового интерфейса

    QWidget* mainWidget; // Основной виджет
    QLabel* result; // Строка результата игры

    QPushButton* buttonStone; // Кнопка камня
    QPushButton* buttonScissors; // Кнопка ножниц
    QPushButton* buttonPaper; // Кнопка бумаги

    QPushButton* buttonBack; // Кнопка "Домой"
    QPushButton* buttonStart; // Кнопка старта игры
    QPushButton* buttonPause; // Кнопка паузы

    bool start; // Флаг для начала игры

    //Сетевое
    QUdpSocket *udpSocket;
    QTcpSocket *tcpSocket;
    QTimer *scanTimer;
    QList<QHostAddress> foundServers; // Коллекция найденных серверов
    bool isConnected = false; // Флаг, указывающий, подключен ли клиент к серверу

    Server* server; // Локальный сервер
};
#endif // CLIENT_H
