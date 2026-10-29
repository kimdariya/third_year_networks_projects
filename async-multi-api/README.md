# Ghibli Async Client

Консольное приложение для поиска информации о фильмах студии Ghibli.
Программа демонстрирует асинхронное сетевое взаимодействие без блокировок.

---

## Структура

```
async-multi-api/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── App.hpp              
│   ├── FilmService.hpp     
│   ├── Future.hpp          
│   ├── GhibliApi.hpp      
│   ├── HttpClient.hpp       
│   ├── Models.hpp          
│   └── WikipediaApi.hpp    
├── src/
│   ├── App.cpp
│   ├── FilmService.cpp
│   ├── GhibliApi.cpp
│   ├── HttpClient.cpp
│   ├── Models.cpp
│   ├── WikipediaApi.cpp
│   └── main.cpp
└── build/
```

---

## Сборка

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## Запуск

```
./ghibli
```

### Поиск

В строке поиска введите часть названия фильма (`Ponyo`, `Totoro`, `Spirited`, `Kaguya`, ...).
Поиск нечувствителен к регистру и ищет совпадения как в английском названии, так и в ромадзи
(транскрипции японского оригинала).  Для выхода из программы на этом уровне используется `:q`.

### Меню списка

После поиска показывается список совпадений (с годом и рейтингом) и доступны команды:

| Команда | Действие |
|---|---|
| `<номер>` | открыть карточку фильма с этим номером |
| `?` | открыть случайный фильм из текущего списка |
| `y <от> <до>` | фильтр по году выпуска, например `y 1990 2005` |
| `r <мин>` | фильтр по минимальному рейтингу Rotten Tomatoes, например `r 90` |
| `c` | сбросить все фильтры |
| `s` | вернуться к новому поиску |
| `q` | выйти из программы |

---

## Пример работы

```
$ ./ghibli

Search Ghibli film (:q - quit): o

Matches:
  [0] Castle in the Sky (Tenkū no shiro Rapyuta) 1986, RT 95%
  [1] Grave of the Fireflies (Hotaru no haka) 1988, RT 97%
  ...
  [14] Ponyo (Gake no ue no Ponyo) 2008, RT 92%
  ...
Commands:
  <number>      open film card
  ?             open random film from the list
  y <from> <to> filter by release year (e.g. y 1990 2005)
  r <min>       filter by min RT score (e.g. r 90)
  c             clear filters
  s             new search
  q             quit
> r 95

Matches:
  [0] Castle in the Sky (Tenkū no shiro Rapyuta) 1986, RT 95%
  [1] Grave of the Fireflies (Hotaru no haka) 1988, RT 97%
  [2] Kiki's Delivery Service (Majo no takkyūbin) 1989, RT 96%
  ...
> 0

=== Castle in the Sky (1986) ===
Original : 天空の城ラピュタ / Tenkū no shiro Rapyuta
Director : Hayao Miyazaki
Producer : Isao Takahata
Runtime  : 124 min
RT score : 95%
Poster   : https://upload.wikimedia.org/wikipedia/ru/.../poster.jpg

Description:
The orphan Sheeta inherited a mysterious crystal that links her to the
mythical sky-kingdom of Laputa...

Wikipedia extract:
Castle in the Sky, also known as Laputa: Castle in the Sky, is a 1986
Japanese animated fantasy adventure film written and directed by Hayao Miyazaki...

Matches:
  ...
> s

Search Ghibli film (:q - quit): :q
```
---

## API-эндпоинты

| Сервис | Назначение | URL |
|---|---|---|
| Ghibli API | Список всех фильмов | `https://ghibliapi.vercel.app/films` |
| Ghibli API | Карточка одного фильма | `https://ghibliapi.vercel.app/films/{id}` |
| Wikipedia (ru) | Поиск статьи по названию | `ru.wikipedia.org/w/api.php?list=search&srsearch=...` |
| Wikipedia (ru) | Постер статьи | `ru.wikipedia.org/w/api.php?prop=pageimages&titles=...` |
| Wikipedia (en) | Текст вступления | `en.wikipedia.org/w/api.php?prop=extracts&titles=...` |
