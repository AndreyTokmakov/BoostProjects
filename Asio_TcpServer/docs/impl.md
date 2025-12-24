Отлично, идём **строго практично**:
**Boost.Asio TCP сервер с минимальными / нулевыми аллокациями на hot-path**
(реальный стиль для HFT / gateways / market-data).

---

# 🎯 Цель

* ❌ никаких `new/delete` в runtime
* ❌ никаких `std::string` / `vector` в I/O
* ❌ никаких временных объектов в handler’ах
* ✅ preallocated buffers
* ✅ фиксированные сессии
* ✅ one `io_context`, один thread (для простоты)

---

# 🧱 Архитектура

```text
acceptor
  ↓
SessionPool (fixed)
  ↓
Session
  ├── socket
  ├── rx_buffer [N]
  ├── tx_buffer [N]
  └── async_read / async_write
```

---

# 1️⃣ Fixed buffer (stack or pool)

```cpp
constexpr size_t RX_SIZE = 4096;
constexpr size_t TX_SIZE = 4096;
```

---

# 2️⃣ Session — НИ ОДНОЙ динамической аллокации

```cpp
struct Session {
    boost::asio::ip::tcp::socket socket;

    alignas(64) std::array<char, RX_SIZE> rx;
    alignas(64) std::array<char, TX_SIZE> tx;

    size_t rx_used = 0;
    size_t tx_used = 0;

    explicit Session(boost::asio::io_context& io)
        : socket(io) {}

    void start() {
        do_read();
    }

    void do_read() {
        socket.async_read_some(
            boost::asio::buffer(rx.data() + rx_used,
                                 rx.size() - rx_used),
            [this](boost::system::error_code ec, size_t n) {
                if (ec) return close();
                rx_used += n;
                on_data();
                do_read();
            });
    }

    void on_data() {
        // пример: echo без аллокаций
        std::memcpy(tx.data(), rx.data(), rx_used);
        tx_used = rx_used;
        rx_used = 0;
        do_write();
    }

    void do_write() {
        boost::asio::async_write(
            socket,
            boost::asio::buffer(tx.data(), tx_used),
            [this](boost::system::error_code ec, size_t) {
                if (ec) close();
            });
    }

    void close() {
        boost::system::error_code _;
        socket.close(_);
    }
};
```

✔️ нет `std::string`
✔️ нет аллокаций в handlers
✔️ socket живёт внутри Session

---

# 3️⃣ Fixed Session Pool (без new)

```cpp
template<size_t N>
class SessionPool {
    std::array<std::optional<Session>, N> sessions;
    boost::asio::io_context& io;

public:
    explicit SessionPool(boost::asio::io_context& io) : io(io) {}

    Session* acquire() {
        for (auto& s : sessions) {
            if (!s.has_value()) {
                s.emplace(io);
                return &*s;
            }
        }
        return nullptr;
    }

    void release(Session* session) {
        for (auto& s : sessions) {
            if (&*s == session) {
                s.reset();
                return;
            }
        }
    }
};
```

❗ аллокаций **нет** (optional внутри массива)

---

# 4️⃣ Accept loop

```cpp
class Server {
    boost::asio::io_context& io;
    boost::asio::ip::tcp::acceptor acceptor;
    SessionPool<128> pool;

public:
    Server(boost::asio::io_context& io, uint16_t port)
        : io(io),
          acceptor(io, {boost::asio::ip::tcp::v4(), port}),
          pool(io) {}

    void start() {
        do_accept();
    }

private:
    void do_accept() {
        Session* s = pool.acquire();
        if (!s) return; // overload policy

        acceptor.async_accept(
            s->socket,
            [this, s](boost::system::error_code ec) {
                if (!ec) {
                    s->start();
                } else {
                    pool.release(s);
                }
                do_accept();
            });
    }
};
```

---

# 5️⃣ `main()`

```cpp
int main() {
    boost::asio::io_context io;

    Server srv(io, 9000);
    srv.start();

    io.run();
}
```

---

# 🔬 Где могут быть скрытые аллокации

⚠️ **важно знать**:

| Источник        | Как убрать                               |
| --------------- | ---------------------------------------- |
| `std::function` | Asio использует type-erased handler → OK |
| Exceptions      | использовать `boost::system::error_code` |
| `std::cout`     | убрать                                   |
| DNS resolver    | не использовать                          |
| Logging         | ring-buffer                              |

---

# 🚀 Дополнительные оптимизации (реальные)

### 1️⃣ Custom handler allocator (Asio feature)

```cpp
struct HandlerAllocator {
    alignas(64) char storage[1024];
    bool used = false;

    void* allocate(size_t n) {
        if (!used && n <= sizeof(storage)) {
            used = true;
            return storage;
        }
        return ::operator new(n);
    }

    void deallocate(void* p, size_t) {
        if (p == storage) used = false;
        else ::operator delete(p);
    }
};
```

Подключается через `asio_handler_allocate`.

---

### 2️⃣ `TCP_NODELAY`

```cpp
socket.set_option(boost::asio::ip::tcp::no_delay(true));
```

---

### 3️⃣ `SO_REUSEPORT`, `SO_RCVBUF`

---

# 🧠 Что дальше логично

Следующие шаги (очень логичные):

1. 🔥 Zero-copy parser поверх `rx`
2. 🔥 Fixed message framing (length-prefix)
3. 🔥 Lock-free queue NIC → Strategy
4. 🔥 Multi-core: `io_context` per core
5. 🔥 `boost::asio::experimental::channel`

---

Если хочешь — следующим шагом:

* **binary protocol parser без копий**
* **FIX-подобный length-prefix**
* **SPSC pipeline поверх Asio**
* **Asio + intrusive order pool**

Скажи, куда идём дальше.
