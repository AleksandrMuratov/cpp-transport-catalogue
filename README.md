# cpp-transport-catalogue
Транспортный справочник / маршрутизатор. Учебный проект.

Система хранения транспортных маршрутов и обработки запросов к ней:
* Входные данные и ответ в JSON формате
* Выходной JSON-файл может содержать визуализацию карты маршрута(ов) в формате SVG-файла.
* Поиск кратчайшего маршрута
* Сериализация базы данных и настроек справочника при помощи Google Protobuf.
* Объекты JSON поддерживают цепочки вызовов (method chaining) при конструировании, превращая ошибки применения данных формата JSON в ошибки компиляции.

Использованные идеомы, технологии и элементы языка:
* OOP: inheritance, abstract interfaces
* STL smart pointers
* JSON load / unload
* SVG image format embedded inside XML output
* Curiously Recurring Template Pattern (CRTP)
* Method chaining
* Directed Weighted Graph data structure for Router module
* Google Protocol Buffers for data serialization
* Static libraries .LIB/.A
* CMake generated project and dependency files

Сборка проекта:
1. Скачать архив Google Protobuf с [репозитория на GitHub](https://github.com/protocolbuffers/protobuf/releases) и разархивировать его на вашем компьютере в папку, не содержащую русских символов и пробелов в имени и/или пути к ней. В данном проекте использовался Protobuf 21.12.
2. Создадим папки build-debug и build-release для сборки двух конфигураций Protobuf. Если вы используете Visual Studio, будет достаточно одной папки build. А если CLion или QT Creator, IDE автоматически создаст папки при открытии файла CMakeLists.txt.
3. Создадим папку, в которой разместим пакет Protobuf. Будем называть её /path/to/protobuf/package. 
4. Если вы собираете не через IDE, в папке build-debug выполните следующие команды:
```
cmake <путь к protobuf>/cmake -DCMAKE_BUILD_TYPE=Debug \
      -Dprotobuf_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=/path/to/protobuf/package
cmake --build .
cmake --install .
```
Для Visual Studio команды немного другие. Конфигурация указывается не при генерации, а при сборке:
```
cmake <путь к protobuf>/cmake ^
      -Dprotobuf_BUILD_TESTS=OFF ^
      -DCMAKE_INSTALL_PREFIX=/path/to/protobuf/package ^
      -Dprotobuf_MSVC_STATIC_RUNTIME=OFF
cmake --build . --config Debug
cmake --install . --config Debug
```
Аналогично выполните команды в папке build-release поменяв конфигурацию на Release
4. Клонируйте проект. Перед сборкой запишите в переменную `CMAKE_PREFIX_PATH` путь к пакету Protobuf.\
Через консоль команда будет выглядеть так: 'cmake . -DCMAKE_PREFIX_PATH=/path/to/protobuf/package`

