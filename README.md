# Поисковый сервер
## Описание
_Обучающий проект_. Реализует функционал поискового сервера. 
На вход сервера подаются стоп-слова, документы, запрос. Документ и запросы преставляют собой строки. Запрос может содержать минус-слова, т.е. слова, которые должны исключаться из поиска. Сервер позволяет вернуть топ документов, соответствующих запросу и удовлетворяющих условию.

Использующиеся технологии
- string_view,
- параллельные версии структур данных и алгоритмов из STL,
- ConcurrentMap (хэш-таблица, допускающая параллельную обработку).

## Модули
concurent_map - параллельная версия хеш-таблицы.

document - структура данных дескриптора документа.

paginator - класс, позволяющий выдавать результаты поиска страницами.

process_query - содержит функции (параллельную и последовательную весрсии) обработки очереди запросов к поисковому серверу.

remove_duplicates - удаление дубликатов из списка документов поискового сервера.

request_queue - класс очереди запросов к поисковому серверу.

search_server - класс поискового сервера.

read_input_functions - вспомогательные функции чтения данных из потоков.

string_processing - вспомогательные функции обработки строк.

test_example_functions - фреймворк для тестирования.

log_duration - фреймворк для измерения длительности выполнения кода.

## Системные требования
C++17



