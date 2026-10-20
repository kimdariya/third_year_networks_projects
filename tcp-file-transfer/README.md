# tcp-file-transfer

Приложение передаёт произвольный файл с одного компьютера на другой по TCP. Сервер сохраняет полученный файл в поддиректорию `uploads/` своей текущей директории и раз в 3 секунды печатает мгновенную и среднюю скорость приёма для каждого активного клиента.

Поддерживает параллельную работу с несколькими клиентами через `std::thread`.

---

## Структура

```
tcp-file-transfer/
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yml
├── .dockerignore
├── README.md
├── include/
│   ├── socket.h          
│   ├── protocol.h       
│   ├── speed_meter.h    
│   ├── uploads.h        
│   ├── server.h         
│   └── client.h         
├── src/
│   ├── socket.cpp
│   ├── protocol.cpp
│   ├── speed_meter.cpp
│   ├── uploads.cpp
│   ├── server.cpp
│   ├── client.cpp
│   ├── server_main.cpp
│   └── client_main.cpp
├── files/              # файлы для отправки (тестовые)
└── uploads/            # принятые файлы (создаётся автоматически)
```

---

## Сборка

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Исполняемые файлы — `build/server` и `build/client`.

Или открыть проект в CLion и нажать **Build**.

---

## Запуск

### Сервер

```
./server <порт> 
```

| Аргумент | Обязательный | Описание |
|---|---|---|
| `<порт>` | да | TCP-порт (1..65535), на котором сервер ждёт входящих соединений |

### Клиент

```
./client <файл> <хост> <порт>
```

| Аргумент | Обязательный | Описание |
|---|---|---|
| `<файл>` | да | Относительный или абсолютный путь к отправляемому файлу |
| `<хост>` | да | DNS-имя или IP-адрес сервера |
| `<порт>` | да | TCP-порт сервера (1..65535) |


Запустим сервер в одном терминале, а клиентов — в других. Каждый клиент отправит свой файл, сервер напечатает скорость по каждому.

Код возврата клиента: `0` — сервер подтвердил успех, `1` — неуспех или ошибка, `2` — неверные аргументы.

---

## Запуск через Docker

Требуется установленный Docker Desktop.

**1.** Положить файлы для отправки в `files/`:

```
files/test.pdf
files/big.bin
```

**2.** Запустить сервер и двух клиентов:

```bash
docker compose up --build
```

**3.** Принятые файлы появятся в `uploads/` на хост-машине:

```bash
ls uploads/
# test.pdf  big.bin
```

**Остановить:**

```bash
docker compose down
```

Сервисы и порты:

| Сервис | Образ | Порт | Что делает |
|---|---|---|---|
| `server` | `tcp-ft` | `4449:4449` | Принимает файлы, пишет в `./uploads` |
| `client-big` | `tcp-ft` | — | Через 2 с отправляет `files/big.bin` (200 МБ) |
| `client-small` | `tcp-ft` | — | Через 4 с отправляет `files/test.pdf` (90 МБ) |

Клиенты подключаются к серверу по DNS-имени `server` внутри docker-сети. Порт `4449` проброшен на хост, поэтому можно также подключаться локально: `./client file.pdf 127.0.0.1 4449`.

---

## Пример работы

Сервер (запущен из директории `build/`):

```
$ ./server 4449      
[server] listening on port 4449, uploads -> /Users/WinEleven/Desktop/Seti/third_year_networks_projects/tcp-file-transfer/build/uploads
[server] accepted 127.0.0.1:50864 (fd=4)
[server] 127.0.0.1:50864: start sending 'big.bin' (209.72 MB)
[server] 127.0.0.1:50864: finished sending 'big.bin' (209.72 MB) -> ok
[server][127.0.0.1:50864] final: instant=1.44 GB/s avg=1.44 GB/s total=209.72 MB
```

Клиент:

```
$ ./client big.bin 127.0.0.1 4449 
[client] big.bin: OK (209.72 MB)
```
- `instant` — скорость за интервал с прошлого отчёта (около 3 секунд);
- `avg` — средняя за весь сеанс;
- `total` — суммарно принято байт;
- `final` — отчёт по клиенту, который завершился быстрее чем за 3 секунды;
- при ошибке сервер пишет причину (например, `size mismatch`, `recv: peer closed`) и удаляет неполный файл.
---

## Константы

| Константа | Значение | Где | Назначение |
|---|---|---|---|
| `kChunk` | 64 КиБ | `server.cpp`, `client.cpp` | Размер буфера чтения/записи |
| `kReportPeriod` | 3 с | `server.cpp` | Интервал печати скорости |
| `kTick` | 100 мс | `server.cpp` | Шаг сна в `reporter_loop` и пауза после ошибки `accept` |
| `kRecvTimeoutSec` | 60 с | `server.cpp` | Таймаут на приём и отправку (`SO_RCVTIMEO`/`SO_SNDTIMEO`) на стороне сервера |
| `kIoTimeoutSec` | 60 с | `client.cpp` | Таймаут на приём и отправку (`SO_RCVTIMEO`/`SO_SNDTIMEO`) на стороне клиента |
| `kMaxClients` | 16 | `server.cpp` | Максимум одновременных клиентов; лишний получает ack «неуспех» |
| `kNAME_MAX` | 255 | `uploads.cpp` | Запасной лимит длины имени файла, если `pathconf(_PC_NAME_MAX)` не сработал |
| `FileHeader::kMaxNameBytes` | 4096 | `protocol.h` | Максимальная длина имени |
| `FileHeader::kMaxFileSize` | 1 ТиБ | `protocol.h` | Максимальный размер файла |
| `SOMAXCONN` | системное | `socket.cpp` | Размер очереди `listen` |