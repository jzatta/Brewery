#pragma once
/*
 * WiFi: ESP-AT client on top of any RxTx<Port> transport (header-only, no heap,
 * no virtuals). Everything is blocking/polling.
 *
 *   Uart uart(&huart6);
 *   WiFi wifi(uart);                       // Port deduced by CTAD
 *   wifi.begin();
 *   wifi.connect("ssid", "password");
 *   HttpResponse r;
 *   if (wifi.httpGet("http://httpbin.org/get", r)) printf("%s", r.data);
 */
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "rxtx.hpp"

enum class HttpMethod  : uint8_t { Head = 1, Get = 2, Post = 3, Put = 4, Delete = 5 };
enum class ContentType : uint8_t { Form = 0, Json = 1, Multipart = 2, Xml = 3 };

struct HttpResponse {
    const char* data   = nullptr;  /* NUL-terminated, points into WiFi's RX buffer */
    size_t      length = 0;        /* bytes actually available in data              */
    size_t      size   = 0;        /* size announced by the module                  */
    bool truncated() const { return length < size; }
};

template <typename Port, size_t RxBufSize = 1024>
class WiFi {
public:
    explicit WiFi(RxTx<Port>& port) : io_(port) {}

    /* Checks that the module answers and disables command echo. */
    bool begin()
    {
        for (int i = 0; i < 3; ++i)
            if (transact("AT", 500) == Result::Ok) return transact("ATE0", 1000) == Result::Ok;
        return false;
    }

    /* Station mode + join access point. Blocks until GOT IP / FAIL / timeout. */
    bool connect(const char* ssid, const char* password, uint32_t timeout_ms = 20000)
    {
        if (transact("AT+CWMODE=1", 1000) != Result::Ok) return false;
        Cmd c;
        c.put("AT+CWJAP=\"").putEscaped(ssid).put("\",\"").putEscaped(password).put("\"");
        return c && transact(c.str(), timeout_ms) == Result::Ok;
    }

    bool disconnect() { return transact("AT+CWQAP", 3000) == Result::Ok; }

    /* state 2 = connected and got an IP */
    bool isConnected()
    {
        return transact("AT+CWSTATE?", 2000) == Result::Ok
            && std::strstr(buf_, "+CWSTATE:2") != nullptr;
    }

    /* Generic HTTP request through AT+HTTPCLIENT. http:// -> TCP, https:// -> TLS.
     * The whole AT command must stay below kCmdSize (module limit: 256 bytes). */
    bool httpRequest(HttpMethod method, const char* url, HttpResponse& resp,
                     const char* body = nullptr, ContentType ct = ContentType::Form,
                     uint32_t timeout_ms = 15000)
    {
        resp = HttpResponse{};
        const bool tls = std::strncmp(url, "https://", 8) == 0;
        Cmd c;
        c.put("AT+HTTPCLIENT=").putInt(static_cast<int>(method))
         .put(",").putInt(static_cast<int>(ct))
         .put(",\"").putEscaped(url).put("\",\"\",\"\",").putInt(tls ? 2 : 1);
        if (body) c.put(",\"").putEscaped(body).put("\"");
        if (!c || transact(c.str(), timeout_ms) != Result::Ok) return false;
        return parseHttp(resp);
    }

    bool httpGet(const char* url, HttpResponse& resp, uint32_t timeout_ms = 15000)
    { return httpRequest(HttpMethod::Get, url, resp, nullptr, ContentType::Form, timeout_ms); }

    bool httpPost(const char* url, const char* body, HttpResponse& resp,
                  ContentType ct = ContentType::Json, uint32_t timeout_ms = 15000)
    { return httpRequest(HttpMethod::Post, url, resp, body, ct, timeout_ms); }

    /* Raw text of the last exchange, handy for debugging. */
    const char* lastRaw() const { return buf_; }

private:
    static constexpr size_t kCmdSize = 320;
    enum class Result { Ok, Error, Timeout };

    /* Small bounded command builder; escapes , " \ as required by AT strings. */
    class Cmd {
    public:
        Cmd& put(const char* s) { while (*s) append(*s++); return *this; }
        Cmd& putEscaped(const char* s)
        {
            for (; *s; ++s) {
                if (*s == '"' || *s == '\\' || *s == ',') append('\\');
                append(*s);
            }
            return *this;
        }
        Cmd& putInt(int v) { char t[12]; std::snprintf(t, sizeof t, "%d", v); return put(t); }
        explicit operator bool() const { return !overflow_; }
        const char* str() const { return buf_; }
    private:
        void append(char ch)
        {
            if (len_ + 1 < sizeof buf_) { buf_[len_++] = ch; buf_[len_] = '\0'; }
            else overflow_ = true;
        }
        char buf_[kCmdSize] = {};
        size_t len_ = 0;
        bool overflow_ = false;
    };

    /* Sends cmd, collects the reply in buf_ until OK / ERROR / FAIL / timeout. */
    Result transact(const char* cmd, uint32_t timeout_ms)
    {
        len_ = 0; buf_[0] = '\0'; truncated_ = false;
        std::memset(tail_, 0, sizeof tail_);

        io_.flush();
        io_.write(cmd, std::strlen(cmd));
        io_.write("\r\n", 2);

        const uint32_t t0 = io_.millis();
        while (io_.millis() - t0 < timeout_ms) {
            uint8_t ch;
            if (io_.read(&ch, 1, 2) != 1) continue;

            if (len_ < RxBufSize - 1) { buf_[len_++] = static_cast<char>(ch); buf_[len_] = '\0'; }
            else                      { truncated_ = true; }

            std::memmove(tail_, tail_ + 1, sizeof tail_ - 1);
            tail_[sizeof tail_ - 1] = static_cast<char>(ch);

            if (tailIs("\r\nOK\r\n"))                       return Result::Ok;
            if (tailIs("ERROR\r\n") || tailIs("FAIL\r\n"))  return Result::Error;
        }
        return Result::Timeout;
    }

    bool tailIs(const char* s) const
    {
        const size_t n = std::strlen(s);
        return std::memcmp(tail_ + sizeof tail_ - n, s, n) == 0;
    }

    /* Reply format: +HTTPCLIENT:<size>,<data>\r\n\r\nOK\r\n */
    bool parseHttp(HttpResponse& r)
    {
        char* p = std::strstr(buf_, "+HTTPCLIENT:");
        if (!p) return false;
        p += 12;
        size_t size = 0;
        while (*p >= '0' && *p <= '9') size = size * 10 + static_cast<size_t>(*p++ - '0');
        if (*p != ',') return false;
        ++p;

        const char* end = buf_ + (truncated_ || len_ < 6 ? len_ : len_ - 6); /* drop "\r\nOK\r\n" */
        const size_t avail = end > p ? static_cast<size_t>(end - p) : 0;

        r.size   = size;
        r.length = avail < size ? avail : size;
        r.data   = p;
        p[r.length] = '\0';
        return true;
    }

    RxTx<Port>& io_;
    char   buf_[RxBufSize] = {};
    size_t len_ = 0;
    bool   truncated_ = false;
    char   tail_[8] = {};
};

template <typename P>
WiFi(RxTx<P>&) -> WiFi<P>;
