// Host-side test of WiFi<> with a fake transport.
// Build: g++ -std=c++17 -Wall -Wextra -I Core/Inc tests/host_test.cpp -o /tmp/host_test && /tmp/host_test
#include <cassert>
#include <cstdio>
#include <deque>
#include <string>
#include "wifi.hpp"

class FakePort : public RxTx<FakePort> {
public:
    std::string lastCmd;
private:
    friend class RxTx<FakePort>;
    std::string line_;
    std::deque<char> rx_;
    uint32_t now_ = 0;

    bool writeImpl(const uint8_t* d, size_t n, uint32_t) {
        for (size_t i = 0; i < n; ++i) {
            line_ += char(d[i]);
            if (line_.size() >= 2 && line_.compare(line_.size() - 2, 2, "\r\n") == 0) {
                lastCmd = line_.substr(0, line_.size() - 2);
                line_.clear();
                reply(lastCmd);
            }
        }
        return true;
    }
    size_t readImpl(uint8_t* b, size_t n, uint32_t) {
        size_t i = 0;
        while (i < n && !rx_.empty()) { b[i++] = uint8_t(rx_.front()); rx_.pop_front(); }
        if (i == 0) ++now_;               // idle read advances time
        return i;
    }
    void flushImpl() { rx_.clear(); }
    uint32_t millisImpl() { return now_; }

    void push(const std::string& s) { rx_.insert(rx_.end(), s.begin(), s.end()); }
    void reply(const std::string& c) {
        if (c == "AT" || c == "ATE0" || c == "AT+CWMODE=1") push("\r\nOK\r\n");
        else if (c.rfind("AT+CWJAP=\"good", 0) == 0) push("WIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n");
        else if (c.rfind("AT+CWJAP=", 0) == 0)        push("+CWJAP:3\r\n\r\nFAIL\r\n");
        else if (c == "AT+CWSTATE?")                  push("+CWSTATE:2,\"good\"\r\n\r\nOK\r\n");
        else if (c.rfind("AT+HTTPCLIENT=2", 0) == 0)  push("+HTTPCLIENT:12,hello world!\r\n\r\nOK\r\n");
        else if (c.rfind("AT+HTTPCLIENT=3", 0) == 0)  push("+HTTPCLIENT:2,ok\r\n\r\nOK\r\n");
        else if (c.rfind("AT+HTTPCLIENT=5", 0) == 0)  push("ERROR\r\n");
        // anything else: silence -> timeout
    }
};

int main()
{
    FakePort port;
    WiFi wifi(port);                                  // CTAD

    assert(wifi.begin());
    assert(!wifi.connect("bad", "x", 500));
    assert(wifi.connect("good,ssid", "pa\"ss", 500));
    assert(port.lastCmd == "AT+CWJAP=\"good\\,ssid\",\"pa\\\"ss\"");   // escaping
    assert(wifi.isConnected());

    HttpResponse r;
    assert(wifi.httpGet("http://httpbin.org/get", r));
    assert(port.lastCmd == "AT+HTTPCLIENT=2,0,\"http://httpbin.org/get\",\"\",\"\",1");
    assert(r.size == 12 && r.length == 12 && std::string(r.data) == "hello world!");

    assert(wifi.httpPost("https://x.io/p", "{\"a\":1}", r));
    assert(port.lastCmd == "AT+HTTPCLIENT=3,1,\"https://x.io/p\",\"\",\"\",2,\"{\\\"a\\\":1}\"");
    assert(std::string(r.data) == "ok");

    assert(!wifi.httpRequest(HttpMethod::Delete, "http://x/y", r));   // ERROR
    assert(!wifi.httpRequest(HttpMethod::Put, "http://x/y", r, nullptr, ContentType::Form, 200)); // timeout

    // reply bigger than RX buffer -> truncated but still terminated correctly
    WiFi<FakePort, 24> small(port);
    assert(small.httpGet("http://httpbin.org/get", r));
    assert(r.size == 12 && r.length < 12 && r.truncated());

    // command too long -> rejected before sending
    std::string longUrl = "http://h/" + std::string(400, 'a');
    assert(!wifi.httpGet(longUrl.c_str(), r));

    std::puts("all tests passed");
}
