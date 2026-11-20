#pragma once
/*
 * RxTx: static (CRTP) interface for a byte-oriented transport.
 * No virtual functions -> no vtable, calls are resolved and inlined at compile time.
 *
 * A transport implements:
 *   bool     writeImpl(const uint8_t* data, size_t len, uint32_t timeout_ms);
 *   size_t   readImpl(uint8_t* buf, size_t len, uint32_t timeout_ms);
 *   void     flushImpl();          // discard pending RX data
 *   uint32_t millisImpl();         // monotonic ms tick
 * and derives as:  class Uart : public RxTx<Uart> { friend class RxTx<Uart>; ... };
 */
#include <cstddef>
#include <cstdint>

template <typename Derived>
class RxTx {
public:
    /* true if all bytes were sent */
    bool write(const void* data, size_t len, uint32_t timeout_ms = 1000)
    { return self().writeImpl(static_cast<const uint8_t*>(data), len, timeout_ms); }

    /* Reads up to len bytes; returns as soon as len bytes arrived or
     * no byte shows up for timeout_ms. Returns the number of bytes read. */
    size_t read(void* buf, size_t len, uint32_t timeout_ms = 100)
    { return self().readImpl(static_cast<uint8_t*>(buf), len, timeout_ms); }

    void flush()          { self().flushImpl(); }
    uint32_t millis()     { return self().millisImpl(); }

private:
    RxTx() = default;
    friend Derived;       /* only RxTx<X> can be the base of X */
    Derived& self()       { return static_cast<Derived&>(*this); }
};
