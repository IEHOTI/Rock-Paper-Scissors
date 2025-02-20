# Rock-Paper-Scissors

# Необходимые компоненты
1. Установленный Qt версии 5 и выше
2. Установленный Visual Studio 17 2022 и выше
3. Установленный CMake версии 3.16 и выше

# Инструкция по установке
Открываете командную строку в папке и вписываете следующие команды
1. mkdir build
2. cd build
3. cmake -G <версия Visual studio> -DCMAKE_PREFIX_PATH="<путь до установленной Qt>\msvc20xx_64" .. 
Пример команды к шагу 3: cmake -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64" .. 

# Сборка проекта
Переходите в новосозданную папку build и открываете файл RockPaperScissors.sln. После этого в Visual studio производите сборку проекта.
После сборки перейдите в ...\build\Debug или ...\build\Release (в зависимости от способа сборки проекта) и открывайте RockPaperScissors.exe файл.
Готово! Игра установлена и готова к эксплуатации.
