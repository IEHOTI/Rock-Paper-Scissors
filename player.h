#ifndef PLAYER_H
#define PLAYER_H

#include <QString>
#include <QDataStream>

class Player
{
public:
    Player();
    Player(QString username);
    Player(const Player& other);
    ~Player();

    Player& operator=(const Player& other);

    bool getReady() const;
    int getScore() const;
    int getChoice() const;
    QString getName() const;

    void addScore(int count = 1);
    void setScore(int count);
    void setName(const QString& username);
    void selectChoice(int select = 0) const;
    void changeReady(bool state);

    friend QDataStream &operator<<(QDataStream &out, const Player &obj) {
        out << obj.getName() << obj.getScore() << obj.getChoice() << obj.getReady();
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, Player &obj) {
        QString str;
        int score;
        bool ready;
        int choice;
        in >> str >> score >> choice >> ready;
        obj.selectChoice(choice);
        obj.setName(str);
        obj.changeReady(ready);
        obj.setScore(score);
        return in;
    }

private:
    QString name;
    int score;
    mutable int choice;
    bool ready;
};

struct playerProfile
{
    int id; // Игровой id
    int x,y; // Позиция пользовательского виджета
    Player player; // Пользователь

    const Player& getPlayer() const {return player;}

    friend QDataStream &operator<<(QDataStream &out, const playerProfile &obj) {
        out << obj.id <<obj.x << obj.y << obj.player;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, playerProfile &obj) {
        in >> obj.id >> obj.x >> obj.y >> obj.player;
        return in;
    }
};

#endif // PLAYER_H
