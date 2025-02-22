# Rock-Paper-Scissors 🎮

Добро пожаловать в Rock-Paper-Scissors – классическую игру «Камень, ножницы, бумага», реализованную с использованием Qt и C++!

## 📌 Необходимые компоненты

Перед началом установки убедитесь, что у вас установлены следующие компоненты:

Qt версии 5 и выше

Visual Studio 2022 (версии 17 и выше)

CMake версии 3.16 и выше

## 🔧 Установка и сборка

1️⃣ Подготовка к сборке

Откройте командную строку в корневой папке проекта и выполните следующие команды:

mkdir build

cd build

2️⃣ Генерация проекта с CMake

Выполните следующую команду, указав правильные версии Visual Studio и путь до установленного Qt:

cmake -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64" .. 

Примечание:

Замените C:\Qt\6.8.0\msvc2022_64 на актуальный путь к вашей установке Qt.

Если используете другую версию Visual Studio, замените "Visual Studio 17 2022" на соответствующее значение (например, "Visual Studio 16 2019").

3️⃣ Сборка проекта

Перейдите в папку build.

Откройте сгенерированный файл RockPaperScissors.sln в Visual Studio.

Запустите сборку проекта (Ctrl + Shift + B).

4️⃣ Запуск игры

После успешной сборки выполните следующие шаги:

Перейдите в папку build\Debug или build\Release (в зависимости от конфигурации сборки).

Запустите исполняемый файл RockPaperScissors.exe.

Наслаждайтесь игрой! 🎉

## 📜 Лицензия

Этот проект распространяется под (L)GPL лицензией.

## 📩 Обратная связь

Если у вас есть вопросы или предложения, создайте issue или отправьте pull request в репозиторий. Удачной игры! 🚀
