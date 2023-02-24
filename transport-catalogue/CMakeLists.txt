cmake_minimum_required(VERSION 3.10)

project(transport_router CXX)
set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_PREFIX_PATH "C:/protobuf")
set(MY_HEADER_FILES 
	domain.h 
	geo.h 
	graph.h 
	json.h 
	json_builder.h 
	json_reader.h 
	map_renderer.h 
	ranges.h 
	router.h 
	serialization.h 
	svg.h 
	transport_catalogue.h 
	transport_router.h)

set(MY_SOURSE_FILES 
	domain.cpp 
	geo.cpp 
	json.cpp 
	json_builder.cpp 
	json_reader.cpp 
	map_renderer.cpp 
	serialization.cpp 
	svg.cpp 
	transport_catalogue.cpp 
	transport_router.cpp)

# Эта команда найдёт собранный нами пакет Protobuf.
# REQUIRED означает, что библиотека обязательна.
# Путь для поиска укажем в параметрах команды cmake.
find_package(Protobuf REQUIRED)
# Помимо Protobuf, понадобится библиотека Threads
find_package(Threads REQUIRED)

# Команда вызова protoc. 
# Ей переданы названия переменных, в которые будут сохранены 
# списки сгенерированных файлов, а также сам proto-файл.
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto graph.proto map_renderer.proto svg.proto transport_router.proto)

# добавляем цель
add_executable(${PROJECT_NAME} ${PROTO_SRCS} ${PROTO_HDRS} ${MY_HEADER_FILES} ${MY_SOURSE_FILES} main.cpp)

# find_package определила переменную Protobuf_INCLUDE_DIRS,
# которую нужно использовать как include-путь.
# Также нужно добавить как include-путь директорию, куда
# protoc положит сгенерированные файлы.
target_include_directories(${PROJECT_NAME} PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Также find_package определила Protobuf_LIBRARY.
# Protobuf зависит от библиотеки Threads. Добавим и её при компоновке.

target_link_libraries(${PROJECT_NAME} ${Protobuf_LIBRARY} Threads::Threads)