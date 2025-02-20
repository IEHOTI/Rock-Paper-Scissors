#include "client.h"

void Client::sendUdpRequest() {
    if (isConnected) return; // Не отправляем запросы, если уже подключены

    QByteArray request = "ClientScan";
    udpSocket->writeDatagram(request, QHostAddress::Broadcast, 45455); // Шлём запрос
    udpSocket->flush();

}

void Client::processPendingDatagrams() {
    if (isConnected) return; // Не обрабатываем UDP-запросы, если уже подключены

    bool foundNewServer = false;

    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QString message = QString::fromUtf8(datagram);
        if (message.startsWith("ServerRequest")) {  // Проверка на запрос сервера
            if (!foundServers.contains(sender)) {
                foundServers.append(sender);
                foundNewServer = true;
            }
        }
    }

    if (foundNewServer) QTimer::singleShot(1000, this, &Client::processPendingDatagrams); //Если один сервер найден, ждём ещё секунду на получение остальных серверов
}

void Client::connectToServer(const QHostAddress &address) {
    if (isConnected) return; // Не пытаемся подключиться, если уже подключены

    // Создание всплывающего окна о подключении
    QMessageBox* message = new QMessageBox(this);
    message->setStandardButtons(QMessageBox::NoButton);
    message->show();

    tcpSocket = new QTcpSocket(this); // Создание сокета

    // Подключение сигналов к сокету
    connect(tcpSocket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(tcpSocket, &QTcpSocket::connected, [this,&message](){
        message->close();
        message->deleteLater();
    });
    connect(tcpSocket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    tcpSocket->connectToHost(address, 45454);

    for(int i = 0, k = 0; !isConnected ; i++) {
        if(i == 0) message->setText("Подключение");
        else if(i == 1) message->setText("Подключение .");
        else if(i == 2) message->setText("Подключение ..");
        else if(i == 3) message->setText("Подключение ...");
        else i = 0;
        myPause(500);
        k++;
        if(k == 50) {
            // Если не удалось подключиться за 25 секунд к серверу
            message->close();
            message->deleteLater();
            QMessageBox errorBox(this);
            errorBox.setIcon(QMessageBox::Critical);  // Устанавливаем иконку ошибки
            errorBox.setText("Ошибка подключения. Проверьте соединение и попробуйте ещё раз."); // Основной текст ошибки
            errorBox.setStandardButtons(QMessageBox::Ok); // Кнопка для закрытия окна
            errorBox.exec(); // Показываем окно ошибки
            return;
        }
    }
}

void Client::sendMessage(const QString &message) {
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << static_cast<quint32>(0) << static_cast<quint8>(0) << message; // Записываем команду
    quint32 size = data.size();
    out.device()->seek(0);  // Возвращаемся в начало пакета
    out << size;  // Записываем реальный размер пакета

    tcpSocket->write(data); // Отправка данных
    tcpSocket->flush();
}

void Client::sendMessage(const playerProfile &profile) {
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << static_cast<quint32>(0) << static_cast<quint8>(1) << profile; // Записываем команду

    quint32 size = data.size();
    out.device()->seek(0);  // Возвращаемся в начало пакета
    out << size;  // Записываем реальный размер пакета

    tcpSocket->write(data);  // Отправка данных
    tcpSocket->flush();
}

void Client::sendPause() {
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << static_cast<quint32>(0) << static_cast<quint8>(2) << "Pause";  // Записываем команду

    quint32 size = data.size();
    out.device()->seek(0);  // Возвращаемся в начало пакета
    out << size;  // Записываем реальный размер пакета

    tcpSocket->write(data); // Отправка данных
    tcpSocket->flush();
}

void Client::onConnected() {
    isConnected = true;
    scanTimer->stop();
    // сделать логику при подключении?

}

void Client::onReadyRead() {
    QByteArray buffer;  // Буфер для накопления данных
    while (tcpSocket->bytesAvailable()) {
        buffer.append(tcpSocket->readAll());
        // Пока в буфере есть хотя бы 4 байта (для заголовка размера пакета)
        while (buffer.size() >= sizeof(quint32)) {
            QDataStream in(&buffer, QIODevice::ReadOnly);

            quint32 packetSize;
            in >> packetSize;  // Чтение размера пакета

            if (buffer.size() >= packetSize) {
                quint8 dataType;
                in >> dataType;

                if (dataType == 0) { // Получили команду, считываем её и обрабатываем
                    QString message;
                    in >> message;

                    QStringList parts = message.split('\n', Qt::SkipEmptyParts);
                    if (!parts.isEmpty()) {
                        if(parts[0] == "USER"){
                            int id = parts[1].toInt();
                            int index = Client::findPlayer(id);
                            if(parts[2] == "READY"){
                                playerProfiles[index].player.changeReady(parts[3].toInt());
                                if(playerProfiles[index].player.getName() == user->getName()) pressedPause();
                            }
                        }
                        if(parts[0] == "ALL"){
                            if(parts[1] == "RESULT"){
                                if(parts[2]=="WAIT") {
                                    changeLabel("Ожидание начала игры...");
                                    start = false;
                                    updatePlayground();
                                    if(user->getReady() == true) playground->setDisabled(true);
                                    else playground->setEnabled(true);
                                } else if(parts[2] == "DRAW") {
                                    changeLabel("Ничья");
                                    myPause(3000);
                                    for(const auto& tempPlayer : playerProfiles) tempPlayer.player.selectChoice(0); // Выставляем всем по умолчанию камень
                                    pressedStart();
                                    myPause(500);
                                    updatePlayground();
                                    startGame(true);
                                } else if(parts[2]=="GO") {
                                    start = true;
                                    pressedStart();
                                    for(const auto& tempPlayer : playerProfiles) tempPlayer.player.selectChoice(0); // Выставляем всем по умолчанию камень
                                    updatePlayground();
                                    startGame(false);
                                } else if(parts[2] == "WIN") {
                                    changeLabel("Победа");
                                    updatePlayground();
                                    myPause(2000);
                                    start = false;
                                    pressedPause();
                                    sendMessage("ChangeReady\n" + QString::number(userProfile.id) + "\n" + QString::number(0) + "\n");
                                } else if(parts[2] == "LOSE") {
                                    changeLabel("Поражение");
                                    updatePlayground();
                                    myPause(2000);
                                    start = false;
                                    pressedPause();
                                    sendMessage("ChangeReady\n" + QString::number(userProfile.id) + "\n" + QString::number(0) + "\n");
                                }
                            }
                        }
                    }
                } else if (dataType == 1) { // Переполнение сервера
                    QMessageBox errorBox(this);
                    errorBox.setIcon(QMessageBox::Critical);  // Устанавливаем иконку ошибки
                    errorBox.setText("Ошибка подключения. Проверьте соединение и попробуйте ещё раз."); // Основной текст ошибки
                    errorBox.setStandardButtons(QMessageBox::Ok); // Кнопка для закрытия окна
                    errorBox.exec(); // Показываем окно ошибки
                    disconnectSocket(); // Отсоединяем сокет
                } else if (dataType == 2) { // Получили список игровых профилей
                    // Очистка старых данных
                    players.clear();
                    profiles.clear();
                    playerProfiles.clear();
                    // Обновление данных
                    in >> playerProfiles;
                    for(const auto& profile : playerProfiles) {
                        if(profile.player.getName() == user->getName()) userProfile = profile;
                        players.append(profile.getPlayer());
                    }
                    // Обновление игрового поля
                    delete playground;
                    playground = new QWidget(interface);

                    for(const auto& player : players) {
                        QWidget* enemy = new QWidget(playground);
                        if(player.getName() == "") createWaitWidget(enemy);
                        else createUserWidget(enemy,player);
                        profiles.append(enemy);
                    }

                    updatePlayground();
                    if(user->getReady() == true) playground->setDisabled(true);
                }
                // Убираем обработанный пакет из буфера
                buffer.remove(0, packetSize);
            } else {
                // Если данных недостаточно для полного пакета, ждем следующие
                break;
            }
        }
    }
}

void Client::onDisconnected() {
    isConnected = false;
    foundServers.clear();  // Очистить список серверов после отключения
    udpSocket->close();
    delete udpSocket; // Пересоздать сокет во избежания ошибок
    udpSocket = new QUdpSocket(this);
    if (!udpSocket->bind(QHostAddress::AnyIPv4, 45455, QUdpSocket::ShareAddress)) {
        QMessageBox errorBox(this);
        errorBox.setIcon(QMessageBox::Critical);  // Устанавливаем иконку ошибки
        errorBox.setText("Ошибка подключения. Перезапустите игру.");    // Основной текст ошибки
        errorBox.setStandardButtons(QMessageBox::Ok); // Кнопка для закрытия окна
        errorBox.exec(); // Показываем окно ошибки
        return;
    }
    connect(udpSocket, &QUdpSocket::readyRead, this, &Client::processPendingDatagrams);

    scanTimer->start(10); // Запускаем сканирование снова после отключения

    clearWidget(); // Возврат в многопользовательское меню
    setMultiMenu();

    QMessageBox* message = new QMessageBox(this);
    message->setText("Вы были отключены от сервера.");
    message->setStandardButtons(QMessageBox::Ok);
    message->exec();

}

int Client::findPlayer(int id) {
    for(int i = 0; i < playerProfiles.size();i++) if(playerProfiles[i].id == id) return i; // Если игрок найден - возвращаем индекс

    return -1; // Если игрок не найден
}

void Client::disconnectSocket() {
    if (tcpSocket != nullptr) { // Проверяем, существует ли объект
        if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
            tcpSocket->disconnectFromHost(); // Отключаемся от сервера
            if (tcpSocket->state() != QAbstractSocket::UnconnectedState) {
                tcpSocket->waitForDisconnected(3000); // Ждём отключения (по желанию)
            }
        }
        delete tcpSocket;  // Освобождаем память
        tcpSocket = nullptr; // Обнуляем указатель
    }
}
