#include "player.h"

Player::Player(){
    name = "";
    score = -1;
    choice = -1;
    ready = false;
}

Player::Player(QString username){
    name = username;
    score = 0;
    choice = 0;
    ready = false;
}
Player::Player(const Player& other){
    name = other.name;
    score = other.score;
    choice = other.choice;
    ready = other.ready;
}

Player::~Player(){}

Player& Player::operator=(const Player& other){
    if (this == &other) return *this;

    name = other.name;
    score = other.score;
    choice = other.choice;
    ready = other.ready;
    return *this;
}

bool Player::getReady() const {return ready;}

int Player::getChoice() const {return choice;}

int Player::getScore() const {return score;}

QString Player::getName() const {return name;}

void Player::addScore(int count) {score += count;}

void Player::setScore(int count) {score = count;}

void Player::setName(const QString& username) {name = username;}

void Player::selectChoice(int select) const {choice = select;}

void Player::changeReady(bool state) {ready = state;}

