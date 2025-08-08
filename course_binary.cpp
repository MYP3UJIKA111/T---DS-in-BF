#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>   // для strcpy, strcmp, memset, strncpy
#include <cstdio>    // для remove(...)
#include <cstdlib>   // для system("cls"), system("pause") под Windows
#include <vector>    // для сортировки строк в памяти
#include <algorithm> // std::sort для строк/векторов

//-----------------------------------------------------
// Структура заголовка файла (для двусвязного списка)
//-----------------------------------------------------
struct FileHeader {
    int head;  // позиция первого узла (-1, если список пуст)
    int tail;  // позиция последнего узла (-1, если список пуст)
    int size;  // число узлов в списке
};

/*
 * Формат УЗЛА (в общем случае T — POD или простой тип):
 *   [ int prev ][ int next ][ T data ]
 * Для string и подобного делаем отдельную специализацию,
 * т.к. у string переменная длина.
 */

 //-----------------------------------------------------
 // Пользовательский тип Person (POD для простоты)
 //-----------------------------------------------------
struct Person {
    char name[40];  // Имя (фиксированная длина 40 байт)
    int  age;

    Person() {
        std::memset(name, 0, sizeof(name));
        age = 0;
    }
    Person(const char* n, int a) : age(a) {
        std::memset(name, 0, sizeof(name));
        std::strncpy(name, n, sizeof(name) - 1);
    }

    // Для сортировки (лексикографически по name, затем по age)
    bool operator>(const Person& other) const {
        int c = std::strcmp(name, other.name);
        if (c > 0) return true;
        if (c < 0) return false;
        return (age > other.age);
    }
    bool operator<(const Person& other) const {
        int c = std::strcmp(name, other.name);
        if (c < 0) return true;
        if (c > 0) return false;
        return (age < other.age);
    }

    // Для удобного вывода в консоль
    friend std::ostream& operator<<(std::ostream& os, const Person& p) {
        os << p.name << " (age=" << p.age << ")";
        return os;
    }
};

//-----------------------------------------------------
//      1) Общий шаблон BinaryList<T> (для POD)
//-----------------------------------------------------
template <class T>
class BinaryList : public std::fstream {
private:
    FileHeader fh;         // Заголовок списка (в памяти)
    std::string fname;     // Имя файла
    int iterPos;           // Позиция для итератора (или -1)

public:
    // Конструктор/деструктор
    BinaryList(const std::string& filename);
    ~BinaryList();

    // Основные операции
    void push_back(const T& value);
    void insert(int index, const T& value);
    void erase(int index);
    T    get(int index);
    void update(int index, const T& value);
    void pop_back();
    void pop_front(); 
    void clear();
    void print();
    int  getSize() const;
    void sort();  // Пузырьковая сортировка в файле (для фиксированных по размеру T)

    // Итератор
    void initIterator();
    bool hasNext();
    T    next();

private:
    // Вспомогательные функции чтения/записи заголовка
    void readHeader();
    void writeHeader();
};

//-----------------------------------------------------
// Реализация общего шаблона BinaryList<T> (POD версий)
//-----------------------------------------------------
template <class T>
BinaryList<T>::BinaryList(const std::string& filename)
    : std::fstream(), fname(filename), iterPos(-1)
{
    // Открываем бинарный файл (без trunc), чтобы сохранялся между запусками
    open(fname.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    if (!is_open()) {
        // Если файла нет, создаём
        std::ofstream ff(fname.c_str(), std::ios::binary);
        ff.close();
        // И снова открываем на чтение+запись
        open(fname.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    }

    if (is_open()) {
        // Проверяем размер файла
        seekg(0, std::ios::end);
        std::streamoff sz = tellg();
        if (sz < (std::streamoff)sizeof(FileHeader)) {
            // Инициализируем заголовок пустого списка
            fh.head = -1;
            fh.tail = -1;
            fh.size = 0;
            writeHeader();
        }
        else {
            readHeader();
        }
    }
    else {
        // На случай, если открыть не удалось вообще
        fh.head = -1;
        fh.tail = -1;
        fh.size = 0;
    }
}

template <class T>
BinaryList<T>::~BinaryList() {
    if (is_open()) {
        close();
    }
}

template <class T>
void BinaryList<T>::readHeader() {
    seekg(0, std::ios::beg);
    read(reinterpret_cast<char*>(&fh), sizeof(FileHeader));
}

template <class T>
void BinaryList<T>::writeHeader() {
    seekp(0, std::ios::beg);
    write(reinterpret_cast<const char*>(&fh), sizeof(FileHeader));
}

// Добавить элемент в конец (push_back)
template <class T>
void BinaryList<T>::push_back(const T& value) {
    if (!is_open()) return; // Если файл не открыт, выходим.

    // Определяем, куда писать новый узел
    seekp(0, std::ios::end);
    long newPos = tellp();  // позиция в байтах (long)

    int prev = fh.tail; // Предыдущий элемент — текущий tail.
    int next = -1; // Следующего элемента нет.

    // Записываем сам узел: [prev][next][T data]
    seekp(newPos, std::ios::beg);
    write(reinterpret_cast<char*>(&prev), sizeof(int));
    write(reinterpret_cast<char*>(&next), sizeof(int));
    write(reinterpret_cast<const char*>(&value), sizeof(T));

    if (fh.size == 0) {
        // Если список был пуст
        fh.head = (int)newPos;
        fh.tail = (int)newPos;
        fh.size = 1;
        writeHeader();
    }
    else {
        // Обновляем next у бывшего tail
        if (fh.tail != -1) {
            seekp(fh.tail + sizeof(int), std::ios::beg); // tail + 4 => поле next
            int newPosInt = (int)newPos;
            write(reinterpret_cast<char*>(&newPosInt), sizeof(int));
        }
        fh.tail = (int)newPos;
        fh.size++;
        writeHeader();
    }
}

// Вставка по индексу (insert)
template <class T>
void BinaryList<T>::insert(int index, const T& value) {
    if (!is_open()) return;
    if (index < 0 || index > fh.size) {
        std::cout << "[T] Неверный индекс insert: " << index << "\n";
        return;
    }
    // Если вставка в конец, то это просто push_back
    if (index == fh.size) {
        push_back(value);
        return;
    }
    // Если вставка в начало
    if (index == 0) {
        // Создаём новый узел в конце файла
        seekp(0, std::ios::end);
        long newPos = tellp();
        int prev = -1;
        int next = fh.head;
        seekp(newPos, std::ios::beg);
        write(reinterpret_cast<char*>(&prev), sizeof(int));
        write(reinterpret_cast<char*>(&next), sizeof(int));
        write(reinterpret_cast<const char*>(&value), sizeof(T));

        // Старому head проставляем prev = newPos
        if (fh.head != -1) {
            seekp(fh.head, std::ios::beg);
            write(reinterpret_cast<char*>(&newPos), sizeof(int));
        }
        fh.head = (int)newPos; // Новый head — это новый узел.
        if (fh.size == 0) {
            fh.tail = (int)newPos; // Если список был пуст, tail тоже новый узел.
        }
        fh.size++;
        writeHeader(); // Обновляем заголовок.
        return;
    }

    // Иначе вставка «в середину»
    // Находим позицию узла, который сейчас на месте index
    int currentPos = fh.head;
    for (int i = 0; i < index; i++) {
        seekg(currentPos + sizeof(int), std::ios::beg); // currentPos+4 => поле next
        read(reinterpret_cast<char*>(&currentPos), sizeof(int));
    }
    // currentPos — это позиция узла, который будет стоять после вставляемого

    // Считываем его prev (старый предыдущий)
    int oldPrev;
    seekg(currentPos, std::ios::beg);
    read(reinterpret_cast<char*>(&oldPrev), sizeof(int)); // поле prev

    // Создаём новый узел (записываем в конец файла)
    seekp(0, std::ios::end);
    long newPos = tellp();
    int newPrev = oldPrev;
    int newNext = currentPos;
    write(reinterpret_cast<char*>(&newPrev), sizeof(int));
    write(reinterpret_cast<char*>(&newNext), sizeof(int));
    write(reinterpret_cast<const char*>(&value), sizeof(T));

    // Теперь у узла currentPos поле prev = newPos
    seekp(currentPos, std::ios::beg);
    write(reinterpret_cast<char*>(&newPos), sizeof(int));

    // У старого prev (если он не -1) поле next = newPos
    if (oldPrev != -1) {
        seekp(oldPrev + sizeof(int), std::ios::beg); // oldPrev + 4 => next
        int np = (int)newPos;
        write(reinterpret_cast<char*>(&np), sizeof(int));
    }

    fh.size++;
    writeHeader();
}

// Удаление по индексу (erase)
template <class T>
void BinaryList<T>::erase(int index) {
    if (!is_open()) return;
    if (index < 0 || index >= fh.size) {
        std::cout << "[T] Неверный индекс erase: " << index << "\n";
        return;
    }
    if (fh.size == 0) {
        std::cout << "[T] Список пуст\n";
        return;
    }

    // Ищем узел
    int currentPos = fh.head;
    for (int i = 0; i < index; i++) {
        seekg(currentPos + sizeof(int), std::ios::beg); // currentPos+4 => next
        read(reinterpret_cast<char*>(&currentPos), sizeof(int));
    }
    // Считаем prev, next из него
    int p, n;
    seekg(currentPos, std::ios::beg);
    read(reinterpret_cast<char*>(&p), sizeof(int));       // prev
    read(reinterpret_cast<char*>(&n), sizeof(int));       // next

    // Если удаляемый узел — это head
    if (currentPos == fh.head) {
        fh.head = n;
    }
    // Если удаляемый узел — это tail
    if (currentPos == fh.tail) {
        fh.tail = p;
    }
    // p->next = n
    if (p != -1) {
        seekp(p + sizeof(int), std::ios::beg);
        write(reinterpret_cast<char*>(&n), sizeof(int));
    }
    // n->prev = p
    if (n != -1) {
        seekp(n, std::ios::beg);
        write(reinterpret_cast<char*>(&p), sizeof(int));
    }

    fh.size--;
    writeHeader();
}

// Получить элемент по индексу (get)
template <class T>
T BinaryList<T>::get(int index) {
    T result{};
    if (!is_open()) return result;
    if (index < 0 || index >= fh.size) {
        std::cout << "[T] Неверный индекс get: " << index << "\n";
        return result;
    }
    // Идём по next'ам
    int cur = fh.head;
    for (int i = 0; i < index; i++) {
        seekg(cur + sizeof(int), std::ios::beg);
        read(reinterpret_cast<char*>(&cur), sizeof(int));
    }
    // Читаем данные
    seekg(cur + 2 * sizeof(int), std::ios::beg);
    read(reinterpret_cast<char*>(&result), sizeof(T));
    return result;
}

// Обновить элемент по индексу (update)
template <class T>
void BinaryList<T>::update(int index, const T& value) {
    if (!is_open()) return;
    if (index < 0 || index >= fh.size) {
        std::cout << "[T] Неверный индекс update: " << index << "\n";
        return;
    }
    // Идём до нужного узла
    int cur = fh.head;
    for (int i = 0; i < index; i++) {
        seekg(cur + sizeof(int), std::ios::beg);
        read(reinterpret_cast<char*>(&cur), sizeof(int));
    }
    seekp(cur + 2 * sizeof(int), std::ios::beg);
    write(reinterpret_cast<const char*>(&value), sizeof(T));
}

// Удалить последний элемент (pop_back)
template <class T>
void BinaryList<T>::pop_back() {
    if (fh.size == 0) {
        std::cout << "[T] Список пуст (pop_back)\n";
        return;
    }
    erase(fh.size - 1);
}

// Дополнительно: удалить первый элемент (pop_front)
// (если хотим логику очереди, где удаление идёт с "головы")
template <class T>
void BinaryList<T>::pop_front() {
    if (fh.size == 0) {
        std::cout << "[T] Список пуст (pop_front)\n";
        return;
    }
    erase(0);
}

// Очистить весь список (clear)
template <class T>
void BinaryList<T>::clear() {
    if (is_open()) {
        close();
    }
    std::remove(fname.c_str()); // удаляем файл
    // Создаём заново
    std::ofstream ff(fname.c_str(), std::ios::binary);
    ff.close();
    open(fname.c_str(), std::ios::in | std::ios::out | std::ios::binary);

    // Пустой заголовок
    fh.head = -1;
    fh.tail = -1;
    fh.size = 0;
    writeHeader();
}

// Печать всего списка (print)
template <class T>
void BinaryList<T>::print() {
    if (fh.size == 0) {
        std::cout << "[T] Список пуст.\n";
        return;
    }
    std::cout << "[T] Содержимое списка (size=" << fh.size << "):\n";
    int cur = fh.head;
    for (int i = 0; i < fh.size; i++) {
        seekg(cur, std::ios::beg);
        int p, n;
        T val{};
        read(reinterpret_cast<char*>(&p), sizeof(int));
        read(reinterpret_cast<char*>(&n), sizeof(int));
        read(reinterpret_cast<char*>(&val), sizeof(T));
        std::cout << "  [" << i << "]: " << val << "\n";
        cur = n;
    }
}

template <class T>
int BinaryList<T>::getSize() const {
    return fh.size;
}

// Сортировка (пузырьковая) прямо в файле — допустимо ТОЛЬКО для POD фиксированной длины!
template <class T>
void BinaryList<T>::sort() {
    if (fh.size <= 1) {
        std::cout << "[T] Список пуст или 1 элемент, сортировать нечего.\n";
        return;
    }
    bool swapped;
    do {
        swapped = false;
        int curPos = fh.head;
        for (int i = 0; i < fh.size - 1; i++) {
            // Считываем next
            seekg(curPos + sizeof(int), std::ios::beg);
            int nextPos;
            read(reinterpret_cast<char*>(&nextPos), sizeof(int));

            // читаем curVal, nextVal
            T curVal{}, nextVal{};
            seekg(curPos + 2 * sizeof(int), std::ios::beg);
            read(reinterpret_cast<char*>(&curVal), sizeof(T));
            seekg(nextPos + 2 * sizeof(int), std::ios::beg);
            read(reinterpret_cast<char*>(&nextVal), sizeof(T));

            if (curVal > nextVal) {
                // Меняем их местами прямо в файле
                seekp(curPos + 2 * sizeof(int), std::ios::beg);
                write(reinterpret_cast<char*>(&nextVal), sizeof(T));
                seekp(nextPos + 2 * sizeof(int), std::ios::beg);
                write(reinterpret_cast<char*>(&curVal), sizeof(T));
                swapped = true;
            }
            curPos = nextPos;
        }
    } while (swapped);
    std::cout << "[T] Список отсортирован.\n";
}

// Итератор
template <class T>
void BinaryList<T>::initIterator() {
    iterPos = fh.head;
}

template <class T>
bool BinaryList<T>::hasNext() {
    return (iterPos != -1);
}

template <class T>
T BinaryList<T>::next() {
    T res{};
    if (iterPos == -1) return res;
    seekg(iterPos, std::ios::beg);
    int p, n;
    read(reinterpret_cast<char*>(&p), sizeof(int));
    read(reinterpret_cast<char*>(&n), sizeof(int));
    read(reinterpret_cast<char*>(&res), sizeof(T));
    iterPos = n;
    return res;
}

//--------------------------------------------------------------
// 2) Полная специализация BinaryList<std::string>
//    (т.к. std::string имеет переменную длину)
//--------------------------------------------------------------
template <>
class BinaryList<std::string> : public std::fstream {
private:
    FileHeader fh;
    std::string fname;
    int iterPos;  // для итератора (позиция узла или -1)

public:
    BinaryList(const std::string& filename);
    ~BinaryList();

    void push_back(const std::string& value);
    void insert(int index, const std::string& value);
    void erase(int index);
    std::string get(int index);

    // ВАЖНО! update и sort для string делаем через «перечитывание всего в память»,
    // чтобы избежать проблем с "затиранием" соседних узлов при увеличении длины строки.
    void update(int index, const std::string& value);
    void sort();

    void pop_back();
    void pop_front();
    void clear();
    void print();
    int  getSize() const;

    // Итератор
    void initIterator();
    bool hasNext();
    std::string next();

private:
    void readHeader();
    void writeHeader();

    // Запись/чтение строки (сначала int len, потом len байт)
    void writeString(const std::string& s);
    std::string readString();

    // Вспомогательный метод: «перечитать всё, изменить, переписать файл»
    // Используется из update(...) и sort(...)
    void rewriteAllFromVector(const std::vector<std::string>& vec);
};

//-----------------------------------------------------
// Реализация BinaryList<std::string>
//-----------------------------------------------------
BinaryList<std::string>::BinaryList(const std::string& filename)
    : std::fstream(), fname(filename), iterPos(-1)
{
    open(fname.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    if (!is_open()) {
        // создаём
        std::ofstream ff(fname.c_str(), std::ios::binary);
        ff.close();
        open(fname.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    }
    if (is_open()) {
        seekg(0, std::ios::end);
        std::streamoff sz = tellg();
        if (sz < (std::streamoff)sizeof(FileHeader)) {
            fh.head = -1;
            fh.tail = -1;
            fh.size = 0;
            writeHeader();
        }
        else {
            readHeader();
        }
    }
    else {
        fh.head = -1;
        fh.tail = -1;
        fh.size = 0;
    }
}

BinaryList<std::string>::~BinaryList() {
    if (is_open()) {
        close();
    }
}

void BinaryList<std::string>::readHeader() {
    seekg(0, std::ios::beg);
    read(reinterpret_cast<char*>(&fh), sizeof(FileHeader));
}

void BinaryList<std::string>::writeHeader() {
    seekp(0, std::ios::beg);
    write(reinterpret_cast<const char*>(&fh), sizeof(FileHeader));
}

void BinaryList<std::string>::writeString(const std::string& s) {
    int len = (int)s.size();
    write(reinterpret_cast<char*>(&len), sizeof(int));
    write(s.c_str(), len);
}

std::string BinaryList<std::string>::readString() {
    int len;
    read(reinterpret_cast<char*>(&len), sizeof(int));
    if (len < 0 || len > 1000000) {
        // простой safeguard
        return "";
    }
    std::string temp(len, '\0');
    read(&temp[0], len);
    return temp;
}

// Добавляем в конец (push_back)
void BinaryList<std::string>::push_back(const std::string& value) {
    if (!is_open()) return;
    seekp(0, std::ios::end);
    long newPos = tellp();

    int prev = fh.tail;
    int next = -1;
    seekp(newPos, std::ios::beg);
    write(reinterpret_cast<char*>(&prev), sizeof(int));
    write(reinterpret_cast<char*>(&next), sizeof(int));
    writeString(value);

    if (fh.size == 0) {
        fh.head = (int)newPos;
        fh.tail = (int)newPos;
        fh.size = 1;
        writeHeader();
    }
    else {
        // обновляем next у прежнего tail
        if (fh.tail != -1) {
            seekp(fh.tail + sizeof(int), std::ios::beg);
            int np = (int)newPos;
            write(reinterpret_cast<char*>(&np), sizeof(int));
        }
        fh.tail = (int)newPos;
        fh.size++;
        writeHeader();
    }
}

// Вставка по индексу
void BinaryList<std::string>::insert(int index, const std::string& value) {
    if (!is_open()) return;
    if (index < 0 || index > fh.size) {
        std::cout << "[string] Неверный индекс insert: " << index << "\n";
        return;
    }
    // Если вставка в конец
    if (index == fh.size) {
        push_back(value);
        return;
    }
    // Если вставка в начало
    if (index == 0) {
        seekp(0, std::ios::end);
        long newPos = tellp();
        int prev = -1;
        int next = fh.head;
        seekp(newPos, std::ios::beg);
        write(reinterpret_cast<char*>(&prev), sizeof(int));
        write(reinterpret_cast<char*>(&next), sizeof(int));
        writeString(value);

        if (fh.head != -1) {
            // старому head -> prev = newPos
            seekp(fh.head, std::ios::beg);
            write(reinterpret_cast<char*>(&newPos), sizeof(int));
        }
        fh.head = (int)newPos;
        if (fh.size == 0) {
            fh.tail = (int)newPos;
        }
        fh.size++;
        writeHeader();
        return;
    }

    // Иначе вставка в середину
    int curPos = fh.head;
    for (int i = 0; i < index; i++) {
        seekg(curPos + sizeof(int), std::ios::beg); // curPos + 4 => next
        read(reinterpret_cast<char*>(&curPos), sizeof(int));
    }
    // curPos — позиция узла с индексом index (который сдвинется вправо)
    int oldPrev;
    seekg(curPos, std::ios::beg);
    read(reinterpret_cast<char*>(&oldPrev), sizeof(int)); // prev

    // Новый узел
    seekp(0, std::ios::end);
    long newN = tellp();
    int newPrev = oldPrev;
    int newNext = curPos;
    write(reinterpret_cast<char*>(&newPrev), sizeof(int));
    write(reinterpret_cast<char*>(&newNext), sizeof(int));
    writeString(value);

    // теперь у узла curPos поле prev = newN
    seekp(curPos, std::ios::beg);
    write(reinterpret_cast<char*>(&newN), sizeof(int));

    // у узла oldPrev поле next = newN
    if (oldPrev != -1) {
        seekp(oldPrev + sizeof(int), std::ios::beg);
        int tmp = (int)newN;
        write(reinterpret_cast<char*>(&tmp), sizeof(int));
    }
    fh.size++;
    writeHeader();
}

// Удаление по индексу
void BinaryList<std::string>::erase(int index) {
    if (!is_open()) return;
    if (index < 0 || index >= fh.size || fh.size == 0) {
        std::cout << "[string] Неверный индекс erase: " << index << "\n";
        return;
    }
    int curPos = fh.head;
    for (int i = 0; i < index; i++) {
        seekg(curPos + sizeof(int), std::ios::beg);
        read(reinterpret_cast<char*>(&curPos), sizeof(int));
    }
    int p, n;
    seekg(curPos, std::ios::beg);
    read(reinterpret_cast<char*>(&p), sizeof(int));  // prev
    read(reinterpret_cast<char*>(&n), sizeof(int));  // next

    // если удаляем head
    if (curPos == fh.head) {
        fh.head = n;
    }
    // если удаляем tail
    if (curPos == fh.tail) {
        fh.tail = p;
    }
    // p->next = n
    if (p != -1) {
        seekp(p + sizeof(int), std::ios::beg);
        write(reinterpret_cast<char*>(&n), sizeof(int));
    }
    // n->prev = p
    if (n != -1) {
        seekp(n, std::ios::beg);
        write(reinterpret_cast<char*>(&p), sizeof(int));
    }
    fh.size--;
    writeHeader();
}

// Получение элемента по индексу
std::string BinaryList<std::string>::get(int index) {
    if (!is_open()) return "";
    if (index < 0 || index >= fh.size) {
        std::cout << "[string] Неверный индекс get: " << index << "\n";
        return "";
    }
    int cur = fh.head;
    for (int i = 0; i < index; i++) {
        seekg(cur + sizeof(int), std::ios::beg);
        read(reinterpret_cast<char*>(&cur), sizeof(int));
    }
    // пропускаем поля prev и next
    seekg(cur + 2 * sizeof(int), std::ios::beg);
    return readString();
}

// ВАЖНО: update для string делаем «через вектор»
void BinaryList<std::string>::update(int index, const std::string& value) {
    if (!is_open()) return;
    if (index < 0 || index >= fh.size) {
        std::cout << "[string] Неверный индекс update: " << index << "\n";
        return;
    }
    // 1) Считаем всё в память
    std::vector<std::string> temp;
    temp.reserve(fh.size);
    int cur = fh.head;
    for (int i = 0; i < fh.size; i++) {
        seekg(cur, std::ios::beg);
        int p, n;
        read(reinterpret_cast<char*>(&p), sizeof(int));
        read(reinterpret_cast<char*>(&n), sizeof(int));
        std::string s = readString();
        temp.push_back(s);
        cur = n;
    }
    // 2) Меняем нужный элемент
    temp[index] = value;
    // 3) Полностью переписываем файл
    rewriteAllFromVector(temp);
}

// Сортировка «через вектор»
void BinaryList<std::string>::sort() {
    if (fh.size <= 1) {
        std::cout << "[string] Список пуст или 1 элемент, сортировать нечего.\n";
        return;
    }
    // 1) Считываем всё в память
    std::vector<std::string> temp;
    temp.reserve(fh.size);
    int cur = fh.head;
    for (int i = 0; i < fh.size; i++) {
        seekg(cur, std::ios::beg);
        int p, n;
        read(reinterpret_cast<char*>(&p), sizeof(int));
        read(reinterpret_cast<char*>(&n), sizeof(int));
        std::string s = readString();
        temp.push_back(s);
        cur = n;
    }
    // 2) Сортируем в памяти
    std::sort(temp.begin(), temp.end());
    // 3) Переписываем файл
    rewriteAllFromVector(temp);
    std::cout << "[string] Список отсортирован.\n";
}

// Вспомогательный метод: перезаписать всё из вектора
void BinaryList<std::string>::rewriteAllFromVector(const std::vector<std::string>& vec) {
    // Сначала clear()
    if (is_open()) {
        close();
    }
    std::remove(fname.c_str());
    {
        std::ofstream ff(fname.c_str(), std::ios::binary);
        ff.close();
    }
    open(fname.c_str(), std::ios::in | std::ios::out | std::ios::binary);

    // Пустой заголовок
    fh.head = -1;
    fh.tail = -1;
    fh.size = 0;
    writeHeader();

    // Теперь заново записываем все строки push_back-ом
    for (auto& s : vec) {
        push_back(s);
    }
}

// pop_back
void BinaryList<std::string>::pop_back() {
    if (fh.size == 0) {
        std::cout << "[string] Список пуст (pop_back)\n";
        return;
    }
    erase(fh.size - 1);
}

// pop_front
void BinaryList<std::string>::pop_front() {
    if (fh.size == 0) {
        std::cout << "[string] Список пуст (pop_front)\n";
        return;
    }
    erase(0);
}

// clear
void BinaryList<std::string>::clear() {
    if (is_open()) {
        close();
    }
    std::remove(fname.c_str());
    {
        std::ofstream ff(fname.c_str(), std::ios::binary);
        ff.close();
    }
    open(fname.c_str(), std::ios::in | std::ios::out | std::ios::binary);

    fh.head = -1;
    fh.tail = -1;
    fh.size = 0;
    writeHeader();
}

// print
void BinaryList<std::string>::print() {
    if (fh.size == 0) {
        std::cout << "[string] Список пуст.\n";
        return;
    }
    std::cout << "[string] Содержимое (size=" << fh.size << "):\n";
    int cur = fh.head;
    for (int i = 0; i < fh.size; i++) {
        seekg(cur, std::ios::beg);
        int p, n;
        read(reinterpret_cast<char*>(&p), sizeof(int));
        read(reinterpret_cast<char*>(&n), sizeof(int));
        std::string s = readString();
        std::cout << "  [" << i << "]: " << s << "\n";
        cur = n;
    }
}

int BinaryList<std::string>::getSize() const {
    return fh.size;
}

// Итератор
void BinaryList<std::string>::initIterator() {
    iterPos = fh.head;
}

bool BinaryList<std::string>::hasNext() {
    return (iterPos != -1);
}

std::string BinaryList<std::string>::next() {
    if (iterPos == -1) return "";
    seekg(iterPos, std::ios::beg);
    int p, n;
    read(reinterpret_cast<char*>(&p), sizeof(int));
    read(reinterpret_cast<char*>(&n), sizeof(int));
    std::string s = readString();
    iterPos = n;
    return s;
}

//-----------------------------------------------------
// Функции меню (для int, string, Person)
//-----------------------------------------------------
void menuInt();
void menuString();
void menuPerson();

//-----------------------------------------------------
// main
//-----------------------------------------------------
int main() {
   
    //setlocale(LC_ALL, "");  // русская локаль под Windows если требуется

    while (true) {
        system("cls");  // Очистка экрана (Windows). Если вы на *nix, уберите или замените.
        std::cout << "==== Главное Меню ====\n"
            << "1. int\n"
            << "2. string\n"
            << "3. Person\n"
            << "0. Выход\n"
            << "======================\n"
            << "Ваш выбор: ";
        int c;
        std::cin >> c;
        if (!std::cin.good()) {
            std::cin.clear();
            std::cin.ignore(9999, '\n');
            continue;
        }
        if (c == 0) {
            break;
        }
        switch (c) {
        case 1:
            menuInt();
            break;
        case 2:
            menuString();
            break;
        case 3:
            menuPerson();
            break;
        default:
            std::cout << "Пока не реализовано.\n";
            system("pause");
        }
    }

    std::cout << "\nПрограмма завершена.\n";
    return 0;
}

//-----------------------------------------------------
// Меню для int
//-----------------------------------------------------
void menuInt() {
    std::cout << "Введите имя файла (например, intList.bin): ";
    std::string fname;
    std::cin >> fname;

    // Создаём двусвязный список int
    BinaryList<int> list(fname);

    while (true) {
        system("cls");
        std::cout << "=== Меню (int) ===\n"
            << "1. push_back\n"
            << "2. insert(index)\n"
            << "3. erase(index)\n"
            << "4. get(index)\n"
            << "5. update(index)\n"
            << "6. pop_back\n"
            << "7. pop_front (новый)\n"
            << "8. clear\n"
            << "9. print\n"
            << "10. size\n"
            << "11. sort\n"
            << "12. итератор (пошаговый вывод)\n"
            << "0. Назад\n"
            << "Ваш выбор: ";
        int c;
        std::cin >> c;
        if (!std::cin.good()) {
            std::cin.clear();
            std::cin.ignore(9999, '\n');
            continue;
        }
        if (c == 0) {
            break;
        }
        switch (c) {
        case 1: {
            std::cout << "Введите число: ";
            int val;
            std::cin >> val;
            list.push_back(val);
            break;
        }
        case 2: {
            std::cout << "Введите index и число: ";
            int idx, val;
            std::cin >> idx >> val;
            list.insert(idx, val);
            system("pause");
            break;
        }
        case 3: {
            std::cout << "Введите index: ";
            int idx;
            std::cin >> idx;
            list.erase(idx);
            break;
        }
        case 4: {
            std::cout << "Введите index: ";
            int idx;
            std::cin >> idx;
            int g = list.get(idx);
            std::cout << "Получили: " << g << "\n";
            system("pause");
            break;
        }
        case 5: {
            std::cout << "Введите index и новое значение: ";
            int idx, val;
            std::cin >> idx >> val;
            list.update(idx, val);
            break;
        }
        case 6:
            list.pop_back();
            break;
        case 7:
            list.pop_front();
            break;
        case 8:
            list.clear();
            std::cout << "Список очищен.\n";
            system("pause");
            break;
        case 9:
            system("cls");
            list.print();
            system("pause");
            break;
        case 10:
            std::cout << "size = " << list.getSize() << "\n";
            system("pause");
            break;
        case 11:
            list.sort();
            system("pause");
            break;
        case 12: {
            list.initIterator();
            while (list.hasNext()) {
                int x = list.next();
                std::cout << x << " ";
            }
            std::cout << "\n";
            system("pause");
            break;
        }
        default:
            std::cout << "Неверный пункт.\n";
            system("pause");
        }
    }
}

//-----------------------------------------------------
// Меню для string
//-----------------------------------------------------
void menuString() {
    std::cout << "Введите имя файла (например, strList.bin): ";
    std::string fname;
    std::cin >> fname;

    // Создаём двусвязный список string
    BinaryList<std::string> list(fname);

    while (true) {
        system("cls");
        std::cout << "=== Меню (string) ===\n"
            << "1. push_back\n"
            << "2. insert(index)\n"
            << "3. erase(index)\n"
            << "4. get(index)\n"
            << "5. update(index)\n"
            << "6. pop_back\n"
            << "7. pop_front\n"
            << "8. clear\n"
            << "9. print\n"
            << "10. size\n"
            << "11. sort\n"
            << "12. итератор (пошаговый вывод)\n"
            << "0. Назад\n"
            << "Ваш выбор: ";
        int c;
        std::cin >> c;
        if (!std::cin.good()) {
            std::cin.clear();
            std::cin.ignore(9999, '\n');
            continue;
        }
        if (c == 0) {
            break;
        }
        switch (c) {
        case 1: {
            std::cout << "Введите строку (без пробелов): ";
            std::string val;
            std::cin >> val;
            list.push_back(val);
            break;
        }
        case 2: {
            std::cout << "Введите index и строку: ";
            int idx;
            std::string val;
            std::cin >> idx >> val;
            list.insert(idx, val);
            break;
        }
        case 3: {
            std::cout << "Введите index для удаления: ";
            int idx;
            std::cin >> idx;
            list.erase(idx);
            break;
        }
        case 4: {
            std::cout << "Введите index: ";
            int idx;
            std::cin >> idx;
            std::string got = list.get(idx);
            std::cout << "Получили: " << got << "\n";
            system("pause");
            break;
        }
        case 5: {
            std::cout << "Введите index и новую строку: ";
            int idx;
            std::string val;
            std::cin >> idx >> val;
            list.update(idx, val);
            break;
        }
        case 6:
            list.pop_back();
            break;
        case 7:
            list.pop_front();
            break;
        case 8:
            list.clear();
            std::cout << "Список очищен.\n";
            system("pause");
            break;
        case 9:
            system("cls");
            list.print();
            system("pause");
            break;
        case 10:
            std::cout << "size = " << list.getSize() << "\n";
            system("pause");
            break;
        case 11:
            list.sort();
            system("pause");
            break;
        case 12: {
            list.initIterator();
            while (list.hasNext()) {
                std::string s = list.next();
                std::cout << s << " ";
            }
            std::cout << "\n";
            system("pause");
            break;
        }
        default:
            std::cout << "Неверный пункт.\n";
            system("pause");
        }
    }
}

//-----------------------------------------------------
// Меню для Person
//-----------------------------------------------------
void menuPerson() {
    std::cout << "Введите имя файла (например, personList.bin): ";
    std::string fname;
    std::cin >> fname;

    // Двусвязный список Person (использует общий шаблон для T=Person)
    BinaryList<Person> list(fname);

    while (true) {
        system("cls");
        std::cout << "=== Меню (Person) ===\n"
            << "1. push_back\n"
            << "2. insert(index)\n"
            << "3. erase(index)\n"
            << "4. get(index)\n"
            << "5. update(index)\n"
            << "6. pop_back\n"
            << "7. pop_front\n"
            << "8. clear\n"
            << "9. print\n"
            << "10. size\n"
            << "11. sort\n"
            << "12. итератор (пошаговый вывод)\n"
            << "0. Назад\n"
            << "Ваш выбор: ";
        int c;
        std::cin >> c;
        if (!std::cin.good()) {
            std::cin.clear();
            std::cin.ignore(9999, '\n');
            continue;
        }
        if (c == 0) {
            break;
        }

        switch (c) {
        case 1: {
            std::cout << "Введите имя (без пробелов) и возраст:\n";
            char nm[40];
            int ag;
            std::cin >> nm >> ag;
            if (ag >= 0) {
                Person p(nm, ag);
                list.push_back(p);
            }
            else {
                std::cout << "Возраст не может быть отрицательным\n";
            }
            system("pause");
            break;
        }
        case 2: {
            std::cout << "Введите index, имя, возраст:\n";
            int idx, ag;
            char nm[40];
            std::cin >> idx >> nm >> ag;
            if (ag >= 0) {
                Person p(nm, ag);
                list.insert(idx, p);
            }
            else {
                std::cout << "Возраст не может быть отрицательным\n";
            }
            system("pause");
            break;
        }
        case 3: {
            std::cout << "Введите index для удаления:\n";
            int idx;
            std::cin >> idx;
            list.erase(idx);
            break;
        }
        case 4: {
            std::cout << "Введите index:\n";
            int idx;
            std::cin >> idx;
            Person p = list.get(idx);
            std::cout << "Получили: " << p << "\n";
            system("pause");
            break;
        }
        case 5: {
            std::cout << "Введите index, имя, возраст:\n";
            int idx, ag;
            char nm[40];
            std::cin >> idx >> nm >> ag;
            if (ag >= 0) {
                Person p(nm, ag);
                list.update(idx, p);
            }
            else {
                std::cout << "Возраст не может быть отрицательным\n";
            }
            system("pause");
            break;
        }
        case 6:
            list.pop_back();
            break;
        case 7:
            list.pop_front();
            break;
        case 8:
            list.clear();
            std::cout << "Список очищен.\n";
            system("pause");
            break;
        case 9:
            system("cls");
            list.print();
            system("pause");
            break;
        case 10:
            std::cout << "size = " << list.getSize() << "\n";
            system("pause");
            break;
        case 11:
            list.sort();
            system("pause");
            break;
        case 12: {
            list.initIterator();
            while (list.hasNext()) {
                Person p = list.next();
                std::cout << p << "\n";
            }
            system("pause");
            break;
        }
        default:
            std::cout << "Неверный пункт.\n";
            system("pause");
        }
    }
}
