# SystemInfoTool

В данной работе есть свои критерии для соблюдения лучших практик программирования, включая чистый код, модульность, эффективное управление версиями и обработку ошибок. Проект **SystemInfoTool** представляет собой программу для сбора информации о системе на Windows с использованием WMI (Windows Management Instrumentation).

## **Функциональность**

- **Получение версии Windows**: Определение версии и сборки операционной системы.
- **Информация о CPU**: Сбор данных о процессоре, включая производителя, модель, семейство и количество ядер.
- **Информация о материнской плате**: Получение информации о производителе, продукте, серийном номере и версии материнской платы.
- **Информация о GPU**: Сбор данных о видеокарте, включая название, описание, производителя и версию драйвера.
- **Информация об оперативной памяти**: Получение информации о модулях памяти, включая производителя, объём и скорость.
- **Информация о PnP-устройствах**: Сбор данных о подключённых PnP-устройствах.
- **Логирование выполнения программы**: Ведение логов для отслеживания работы программы и диагностики ошибок.

## **Структура проекта**

Проект разделён на несколько файлов для улучшения читаемости и поддержки:

- **main.cpp**: Точка входа в программу, которая координирует выполнение всех функций и выводит результаты.
- **SystemInfo.h / SystemInfo.cpp**: Реализация функций для получения информации о системе.
- **WMIHelper.h / WMIHelper.cpp**: Вспомогательные функции для работы с WMI.
- **Logger.h / Logger.cpp**: Класс для логирования действий программы.
- **.gitignore**: Файл, определяющий, какие файлы и папки должны быть проигнорированы Git.
- **README.md**: Описание проекта и инструкции по его использованию.

## **Лучшие практики**

- **Рефакторинг кода**: Разделение кода на модули и файлы для улучшения структуры и удобства поддержки.
- **Улучшенная обработка ошибок**: Более информативные сообщения об ошибках и проверка результатов выполнения функций для повышения надёжности программы.
- **Оптимизация WMI-запросов**: Уточнение запросов для минимизации объёма получаемых данных и повышения производительности.
- **Логирование**: Внедрение системы логирования для отслеживания выполнения программы и упрощения диагностики ошибок.

## **Сборка**

### **Требования**

- Компилятор C++ (например, Visual Studio)
- Поддержка C++11 и выше
- Подключение библиотеки `wbemuuid.lib`

### **Инструкции**

1. Откройте проект в вашей C++ IDE (например, Visual Studio).
2. Убедитесь, что подключены необходимые библиотеки (`wbemuuid.lib`).
3. Скомпилируйте и запустите программу.

## **Использование**

Запустите программу, и она выведет информацию о вашей системе в консоль. Также будет создан файл логов `SystemInfoTool.log` с записью выполнения программы.

## **Логирование**

Логи записываются в файл `SystemInfoTool.log` и содержат информацию о инициализации WMI, успешном выполнении запросов и любых ошибках, возникших во время работы программы.