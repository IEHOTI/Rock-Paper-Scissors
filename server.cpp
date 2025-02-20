#include "server.h"

Server::Server() {
    server = {"",0,0};

    // Запускаем TCP сервер на любом свободном порту
    tcpServer = new QTcpServer(this);

    if (!tcpServer->listen(QHostAddress::Any, 45454)) return;

    serverPort = tcpServer->serverPort();
    connect(tcpServer, &QTcpServer::newConnection, this, &Server::onNewConnection);

    // Создаем UDP сокет для рассылки
    udpSocket = new QUdpSocket(this);

    // Таймер отправки UDP-запросов
    QTimer *udpTimer = new QTimer(this);
    connect(udpTimer, &QTimer::timeout, this, &Server::sendUdpBroadcast);
    udpTimer->start(10); // 10 ms
}

Server::Server(ServerInfo newServer) {
    server = {newServer.name,newServer.currentUsers,newServer.maxUsers};

    // Запускаем TCP сервер на любом свободном порту
    tcpServer = new QTcpServer(this);

    if (!tcpServer->listen(QHostAddress::Any, 45454)) return;

    serverPort = tcpServer->serverPort();
    connect(tcpServer, &QTcpServer::newConnection, this, &Server::onNewConnection);

    // Создаем UDP сокет для рассылки
    udpSocket = new QUdpSocket(this);

    // Таймер отправки UDP-запросов
    QTimer *udpTimer = new QTimer(this);
    connect(udpTimer, &QTimer::timeout, this, &Server::sendUdpBroadcast);
    connect(udpTimer,&QTimer::timeout, this, &Server::sendUdpInfo);
    udpTimer->start(10); // 10 ms

    // Алгоритм начального размещения пользователей
    for(int i = 0,x = 0,y = 0; i < newServer.maxUsers;i++) {
        profiles.push_back({i,x,y,Player()});
        if(x == 400) {
            if(y == 125) y = 0;
            else y = 125;
            x = 0;
        }
        else x+= 200;
    }
}

Server::~Server() {
    // Закрываем TCP-сервер
    if (tcpServer) {
        foreach (QTcpSocket *client, clients) {
            if (client) {
                client->abort();
                client->deleteLater(); // Отправляем на удаление
            }
        }
        clients.clear();
        tcpServer->close();
        delete tcpServer;
        tcpServer = nullptr;
    }

    // Закрываем и удаляем UDP-сокет
    if (udpSocket) {
        udpSocket->close();
        delete udpSocket;
        udpSocket = nullptr;
    }

    // Очищаем коллекцию профилей
    profiles.clear();
}

void Server::sendUdpBroadcast() {
    QByteArray request = QByteArray("ServerRequest ") + QByteArray::number(serverPort);
    udpSocket->writeDatagram(request, QHostAddress::Broadcast, 45455);  // Применяем Broadcast
}

void Server::sendUdpInfo() {
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << server.name << server.currentUsers << server.maxUsers; // Отправка информации о сервере
    udpSocket->writeDatagram(data,QHostAddress::Broadcast, 45456);
}

void Server::onNewConnection() {
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();

    if(clients.size() == server.maxUsers) { // Если на сервере нет свободного места
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out << static_cast<quint8>(1) << "SERVER IS FULL";
        clientSocket->write(data);
        clientSocket->disconnectFromHost();
        return;
    }
    clients.append(clientSocket);

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, [this, clientSocket] {
        int i = -1;
        for(int k = 0; k < clients.size();k++) if (clients[k] == clientSocket) i = k; // Находим пользователя который отключился
        if(i == -1) return;
        clients.removeAll(clientSocket); // Удаляем пользователя
        clientSocket->deleteLater();
        playerProfile temp = {profiles[i].id,profiles[i].x,profiles[i].y,Player()};
        profiles.removeAt(i);
        profiles.insert(i,temp);
        for(const auto& client : clients) sendMessage(client,profiles); // Отправляем обновленный список пользователей
    });
}

void Server::sendMessage(QTcpSocket *client,const QString &message) {
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << static_cast<quint32>(0) <<  static_cast<quint8>(0) << message; // Запись команды
    quint32 size = data.size();
    out.device()->seek(0);  // Возвращаемся в начало пакета
    out << size;  // Записываем реальный размер пакета

    client->write(data); // Отправка данных
    client->flush();
}
void Server::sendMessage(QTcpSocket* client, const QVector<playerProfile> &profiles) {
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << static_cast<quint32>(0) << static_cast<quint8>(2) << profiles; // Запись пользователей

    quint32 size = data.size();
    out.device()->seek(0);  // Возвращаемся в начало пакета
    out << size;  // Записываем реальный размер пакета

    client->write(data); // Отправка данных
    client->flush();
}

void Server::onReadyRead() {
    QQueue<QByteArray> delayedPackets; // Коллекция отложенных пакетов
    QByteArray buffer;  // Буфер для накопления данных

    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    while (clientSocket->bytesAvailable()) {
        buffer.append(clientSocket->readAll());

        // Пока в буфере есть хотя бы 4 байта (для заголовка размера пакета)
        while (buffer.size() >= sizeof(quint32)) {
            QDataStream in(&buffer, QIODevice::ReadOnly);

            quint32 packetSize;
            in >> packetSize;  // Чтение размера пакета

            if (buffer.size() >= packetSize) {
                quint8 dataType;
                in >> dataType;

                if (dataType == 0) { // Получение и обработка команды
                    QString message;
                    in >> message;
                    // Разделяем строку по символу '\n'
                    QStringList parts = message.split('\n', Qt::SkipEmptyParts);
                    // Проверяем, есть ли хотя бы одна часть
                    if (!parts.isEmpty()) {
                        if (parts[0] == "ChangeReady") {
                            int id = parts[1].toInt();
                            int index = findPlayer(id);
                            int k = 0;
                            if(index == -1) return;
                            profiles[index].player.changeReady(parts[2].toInt());
                            for(const auto& client : clients) sendMessage(client,("USER\n" + parts[1] + "\nREADY\n" + parts[2] + "\n"));
                            for(int i = 0; i < profiles.size();i++) if(profiles[i].player.getReady()) k++;
                            if(k == profiles.size()) for(const auto& client : clients) sendMessage(client,"ALL\nRESULT\nGO\n");
                            else for(const auto& client : clients) sendMessage(client,"ALL\nRESULT\nWAIT\n");
                        } else if (parts[0] == "ChangePlace") {
                            int id = parts[1].toInt();
                            int index = findPlayer(id);
                            int changeX = parts[2].toInt();
                            int changeY = parts[3].toInt();
                            if(index == -1) return;
                            swapProfile(index, changeX,changeY);
                            for(const auto& client : clients) sendMessage(client,profiles);
                        } else if(parts[0] == "SelectChoice") {
                            int id = parts[1].toInt();
                            int index = findPlayer(id);
                            if(index == -1) return;
                            profiles[index].player.selectChoice(parts[2].toInt());
                        }
                    }
                } else if (dataType == 1) {
                    playerProfile profile;
                    in >> profile;
                    int max = profiles.size();
                    for(int i = 0; i < max;i++) {
                        if(profiles[i].player.getName() == "") { // Заменяем свободный слот на нового игрока
                            profile.id = profiles[i].id;
                            profile.x = profiles[i].x;
                            profile.y = profiles[i].y;
                            profiles.removeAt(i);
                            profiles.insert(i,profile);
                            break;
                        }
                    }
                    for(const auto& client : clients) sendMessage(client,profiles);
                } else if (dataType == 2) {
                    // Откладываем обработку Pause, пока не закончим с остальными пакетами
                    QByteArray pausePacket = buffer.left(packetSize);
                    delayedPackets.enqueue(pausePacket);
                }
                // Убираем обработанный пакет из буфера
                buffer.remove(0, packetSize);
            } else {
                // Если данных недостаточно для полного пакета, ждем следующие
                break;
            }
        }
    }

    while (!delayedPackets.isEmpty()) {
        QByteArray packet = delayedPackets.dequeue();
        QDataStream in(&packet, QIODevice::ReadOnly);

        quint32 packetSize;
        in >> packetSize;

        quint8 dataType;
        in >> dataType;

        if (dataType == 2) checkResult();  // Подсчёт результатов игры
    }
}

int Server::findPlayer(int id) {
    for(int i = 0; i < profiles.size();i++) if(profiles[i].id == id) return i;

    return -1;
}

void Server::swapProfile(int index, int x, int y) {
    int i = 0;
    for(; i < profiles.size();i++) {
        if(profiles[i].x == x)
            if(profiles[i].y == y)
                break; // Нужный игрок найден
    }
    if(i == profiles.size()) return; // Если игрок не был найден
    int userX = profiles[index].x;
    int userY = profiles[index].y;
    profiles[index].x = x;
    profiles[index].y = y;
    profiles[i].x = userX;
    profiles[i].y = userY;
}

void Server::checkResult(){
    bool rock = false;
    bool scissors = false;
    bool paper = false;

    // Проверка на ничью
    for(int i = 0; i < profiles.size();i++){
        if(profiles[i].player.getChoice() == 0) rock = true;
        else if(profiles[i].player.getChoice() == 1) scissors = true;
        else if(profiles[i].player.getChoice() == 2) paper = true;
    }

    if((rock && paper && scissors) || (!rock && !paper) || (!rock && !scissors) || (!paper && !scissors)) {
        for(const auto& client : clients) {sendMessage(client,profiles); sendMessage(client,"ALL\nRESULT\nDRAW\n");}
        return;
    }

    // Подсчёт побед и поражений
    else if(rock && paper) for(int i  = 0; i < profiles.size(); i++){
            if(profiles[i].player.getChoice() == 2) {
                profiles[i].player.addScore();
                for(const auto& client : clients) sendMessage(client,profiles);
                sendMessage(clients[i],"ALL\nRESULT\nWIN\n");
            } else {
                for(const auto& client : clients) sendMessage(client,profiles);
                sendMessage(clients[i],"ALL\nRESULT\nLOSE\n");
            }
        }
    else if(rock && scissors)
            for(int i  = 0; i < profiles.size(); i++){
                if(profiles[i].player.getChoice() == 0) {
                profiles[i].player.addScore();
                sendMessage(clients[i],"ALL\nRESULT\nWIN\n");
                } else {
                    for(const auto& client : clients) sendMessage(client,profiles);
                    sendMessage(clients[i],"ALL\nRESULT\nLOSE\n");
                }
            }
    else if(scissors && paper)
        for(int i  = 0; i < profiles.size(); i++){
            if(profiles[i].player.getChoice() == 1) {
                profiles[i].player.addScore();
                sendMessage(clients[i],"ALL\nRESULT\nWIN\n");
            } else {
                for(const auto& client : clients) sendMessage(client,profiles);
                sendMessage(clients[i],"ALL\nRESULT\nLOSE\n");
            }
        }

    for(const auto& profile : profiles) profile.player.selectChoice(0); // Сброс выбора всех пользователей
}

QHostAddress Server::getIP() {
    // Получаем имя хоста
    QString hostName = QHostInfo::localHostName();

    // Получаем информацию о хосте
    QHostInfo hostInfo = QHostInfo::fromName(hostName);

    // Перебираем все адреса
    for (const QHostAddress& address : hostInfo.addresses()) {
        // Проверяем, что это IPv4-адрес и не является loopback
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) {
            return address; // Возвращаем первый подходящий адрес
        }
    }

    // Если ничего не найдено, возвращаем 0.0.0.0
    return QHostAddress::Any;
}
