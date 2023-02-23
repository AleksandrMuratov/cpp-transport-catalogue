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

5. Клонируйте проект транспортного справочника. Перед сборкой запишите в переменную `CMAKE_PREFIX_PATH` путь к пакету Protobuf.\
Через консоль команда будет выглядеть так: `cmake . -DCMAKE_PREFIX_PATH=/path/to/protobuf/package`
6. Сборка CMAKE проекта.

Описание приложения.

Приложение транспортного справочника спроектировано для работы в 2 режимах: режиме создания базы данных и режиме запросов к базе данных.

Для создания базы данных транспортного справочника с последующей ее сериализацией в файл необходимо запустить программу с параметром make_base. Входные данные поступают из stdin, поэтому можно переопределить источник данных, например, указав входной JSON-файл, из которого будет взята информация для наполнения базы данных вместо stdin. Пример:\
```transport_catalogue.exe make_base <input_data.json```

Для обработки запросов к созданной базе данных (сама база данных десериализуется из ранее созданного файла) необходимо запустить программу с параметром process_requests, указав входной JSON-файл, содержащий запрос(ы) к БД и выходной файл, который будет содержать ответы на запросы, также в формате JSON.
Пример:\
```transport_catalogue.exe process_requests <requests.json >output.txt```

Формат входных данных.

Входные данные принимаются из stdin в JSON формате. Структура верхнего уровня имеет следующий вид:
```
{
  "base_requests": [ ... ],
  "render_settings": { ... },
  "routing_settings": { ... },
  "serialization_settings": { ... },
  "stat_requests": [ ... ]
}
```
Каждый элемент является словарем, содержащим следующие данный:\
`base_requests` — описание автобусных маршрутов и остановок.\
`stat_requests` — запросы к транспортному справочнику.\
`render_settings` — настройки рендеринга карты в формате .SVG.\
`routing_settings` — настройки роутера для поиска кратчайших маршрутов.\
`serialization_settings` — настройки сериализации/десериализации данных.

Примеры входного файла и файла с запросом к справочнику прилагаются.
