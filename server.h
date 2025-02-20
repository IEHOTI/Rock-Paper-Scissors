#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QHostAddress>
#include <QList>
#include <QTimer>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QEventLoop>
#include <QQueue>

#include "player.h"
// Структура для хранения информации о сервере
struct ServerInfo{
    QString name;           // Имя сервера
    int currentUsers;       // Текущее количество пользователей
    int maxUsers;           // Максимальное количество пользователей
};

class Server : public QObject {
    Q_OBJECT
public:
    Server();
    Server(ServerInfo newServer);
    ~Server();

    QHostAddress getIP(); // Получение ip сервера

private slots:
    void sendUdpBroadcast(); // Отправка широковещательных сообщений по UDP
    void sendUdpInfo(); // Отправка информации о сервере через UDP

    void onNewConnection(); // Обработка нового подключения клиента

    // Отправка сообщений клиенту (перегрузки функции для разных типов данных)
    void sendMessage(QTcpSocket* client, const QString &message); // Отправка текстовых сообщений
    void sendMessage(QTcpSocket* client, const QVector<playerProfile> &profiles); // Отправка профилей игроков
    void onReadyRead(); // Обработка чтения данных от клиента

    // Вспомогательные функции
    int findPlayer(int id); // Поиск игрока по ID
    void swapProfile(int index, int x, int y); // Перестановка профиля игрока
    void checkResult(); // Проверка результатов игры или действия

private:
    QTcpServer *tcpServer; // Объект для работы с TCP-сервером
    QUdpSocket *udpSocket; // Объект для работы с UDP-сокетом
    QList<QTcpSocket*> clients; // Список подключенных клиентов
    QVector<playerProfile> profiles; // Вектор с профилями игроков
    quint16 serverPort; // Порт сервера
    ServerInfo server; // Информация о сервере
};
#endif // SERVER_H
