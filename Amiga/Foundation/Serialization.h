// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _SERIALIZATION_INC
#define _SERIALIZATION_INC

#include "Event.h"
#include "Beam.h"
#include "RegisterChange.h"
#include "ChangeRecorder.h"

//
// Basic memory buffer I/O
//

inline uint8_t read8(uint8_t *& buffer)
{
    uint8_t result = *buffer;
    buffer += 1;
    return result;
}

inline uint16_t read16(uint8_t *& buffer)
{
    uint16_t result = ntohs(*((uint16_t *)buffer));
    buffer += 2;
    return result;
}

inline uint32_t read32(uint8_t *& buffer)
{
    uint32_t result = ntohl(*((uint32_t *)buffer));
    buffer += 4;
    return result;
}

inline uint64_t read64(uint8_t *& buffer)
{
    uint32_t hi = read32(buffer);
    uint32_t lo = read32(buffer);
    return ((uint64_t)hi << 32) | lo;
}

inline void write8(uint8_t *& buffer, uint8_t value)
{
    *buffer = value;
    buffer += 1;
}

inline void write16(uint8_t *& buffer, uint16_t value)
{
    *((uint16_t *)buffer) = htons(value);
    buffer += 2;
}

inline void write32(uint8_t *& buffer, uint32_t value)
{
    *((uint32_t *)buffer) = htonl(value);
    buffer += 4;
}

inline void write64(uint8_t *& buffer, uint64_t value)
{
    write32(buffer, (uint32_t)(value >> 32));
    write32(buffer, (uint32_t)(value));
}

//
// Counter (determines the state size)
//

#define COUNT(type) \
SerCounter& operator&(type& v) \
{ \
count += sizeof(type); \
return *this; \
}

#define COUNT_STRUCT(type) \
SerCounter& operator&(type& v) \
{ \
v.applyToItems(*this); \
return *this; \
}

class SerCounter
{
public:

    size_t count;

    SerCounter() { count = 0; }

    COUNT(const bool)
    COUNT(const char)
    COUNT(const signed char)
    COUNT(const unsigned char)
    COUNT(const short)
    COUNT(const unsigned short)
    COUNT(const int)
    COUNT(const unsigned int)
    COUNT(const long)
    COUNT(const unsigned long)
    COUNT(const long long)
    COUNT(const unsigned long long)
    COUNT(const float)
    COUNT(const double)

    COUNT(const AmigaModel)
    COUNT(const MemorySource)
    COUNT(const EventID)
    COUNT(const BusOwner)
    COUNT(const SprDMAState)
    COUNT(const FilterType)
    COUNT(const SerialPortDevice)
    COUNT(const DriveType)
    COUNT(const DriveState)
    COUNT(const KeyboardState)
    COUNT(const DrawingMode)
    COUNT(const DiskType)

    COUNT_STRUCT(Event)
    COUNT_STRUCT(Beam)
    COUNT_STRUCT(Change)
    template <uint16_t capacity> COUNT_STRUCT(ChangeRecorder<capacity>)
    COUNT_STRUCT(RegisterChange)
    COUNT_STRUCT(ChangeHistory)

    template <class T, size_t N>
    SerCounter& operator&(T (&v)[N])
    {
        for(size_t i = 0; i < N; ++i) {
            *this & v[i];
        }
        return *this;
    }
};


//
// Reader (Deserializer)
//

#define DESERIALIZE(type,function) \
SerReader& operator&(type& v) \
{ \
v = (type)function(ptr); \
return *this; \
}

#define DESERIALIZE8(type)  static_assert(sizeof(type) == 1); DESERIALIZE(type,read8)
#define DESERIALIZE16(type) static_assert(sizeof(type) == 2); DESERIALIZE(type,read16)
#define DESERIALIZE32(type) static_assert(sizeof(type) == 4); DESERIALIZE(type,read32)
#define DESERIALIZE64(type) static_assert(sizeof(type) == 8); DESERIALIZE(type,read64)

#define DESERIALIZE_STRUCT(type) \
SerReader& operator&(type& v) \
{ \
v.applyToItems(*this); \
return *this; \
}

class SerReader
{
public:

    uint8_t *ptr;

    SerReader(uint8_t *p) : ptr(p)
    {
    }

    DESERIALIZE8(bool)
    DESERIALIZE8(char)
    DESERIALIZE8(signed char)
    DESERIALIZE8(unsigned char)
    DESERIALIZE16(short)
    DESERIALIZE16(unsigned short)
    DESERIALIZE32(int)
    DESERIALIZE32(unsigned int)
    DESERIALIZE64(long)
    DESERIALIZE64(unsigned long)
    DESERIALIZE64(long long)
    DESERIALIZE64(unsigned long long)
    DESERIALIZE32(float)
    DESERIALIZE64(double)
    DESERIALIZE64(AmigaModel)
    DESERIALIZE32(MemorySource)
    DESERIALIZE64(EventID)
    DESERIALIZE8(BusOwner)
    DESERIALIZE32(SprDMAState)
    DESERIALIZE64(FilterType)
    DESERIALIZE64(SerialPortDevice)
    DESERIALIZE64(DriveType)
    DESERIALIZE32(DriveState)
    DESERIALIZE32(KeyboardState)
    DESERIALIZE32(DrawingMode)
    DESERIALIZE64(DiskType)

    DESERIALIZE_STRUCT(Event)
    DESERIALIZE_STRUCT(Beam)
    DESERIALIZE_STRUCT(Change)
    template <uint16_t capacity> DESERIALIZE_STRUCT(ChangeRecorder<capacity>)
    DESERIALIZE_STRUCT(RegisterChange)
    DESERIALIZE_STRUCT(ChangeHistory)

    template <class T, size_t N>
    SerReader& operator&(T (&v)[N])
    {
        for(size_t i = 0; i < N; ++i) {
            *this & v[i];
        }
        return *this;
    }

    void copy(void *dst, size_t n)
    {
        memcpy(dst, (void *)ptr, n);
        ptr += n;
    }
};


//
// Writer (Serializer)
//

#define SERIALIZE(type,function,cast) \
SerWriter& operator&(type& v) \
{ \
function(ptr, (cast)v); \
return *this; \
}

#define SERIALIZE8(type)  static_assert(sizeof(type) == 1); SERIALIZE(type,write8,uint8_t)
#define SERIALIZE16(type) static_assert(sizeof(type) == 2); SERIALIZE(type,write16,uint16_t)
#define SERIALIZE32(type) static_assert(sizeof(type) == 4); SERIALIZE(type,write32,uint32_t)
#define SERIALIZE64(type) static_assert(sizeof(type) == 8); SERIALIZE(type,write64,uint64_t)

#define SERIALIZE_STRUCT(type) \
SerWriter& operator&(type& v) \
{ \
v.applyToItems(*this); \
return *this; \
}

class SerWriter
{
public:

    uint8_t *ptr;

    SerWriter(uint8_t *p) : ptr(p)
    {
    }

    SERIALIZE8(const bool)
    SERIALIZE8(const char)
    SERIALIZE8(const signed char)
    SERIALIZE8(const unsigned char)
    SERIALIZE16(const short)
    SERIALIZE16(const unsigned short)
    SERIALIZE32(const int)
    SERIALIZE32(const unsigned int)
    SERIALIZE64(const long)
    SERIALIZE64(const unsigned long)
    SERIALIZE64(const long long)
    SERIALIZE64(const unsigned long long)
    SERIALIZE32(const float)
    SERIALIZE64(const double)
    SERIALIZE64(const AmigaModel)
    SERIALIZE32(const MemorySource)
    SERIALIZE64(const EventID)
    SERIALIZE8(const BusOwner)
    SERIALIZE32(const SprDMAState)
    SERIALIZE64(const FilterType)
    SERIALIZE64(const SerialPortDevice)
    SERIALIZE64(const DriveType)
    SERIALIZE32(const DriveState)
    SERIALIZE32(const KeyboardState)
    SERIALIZE32(const DrawingMode)
    SERIALIZE64(const DiskType)

    SERIALIZE_STRUCT(Event)
    SERIALIZE_STRUCT(Beam)
    SERIALIZE_STRUCT(Change)
    template <uint16_t capacity> SERIALIZE_STRUCT(ChangeRecorder<capacity>)
    SERIALIZE_STRUCT(RegisterChange)
    SERIALIZE_STRUCT(ChangeHistory)

    template <class T, size_t N>
    SerWriter& operator&(T (&v)[N])
    {
        for(size_t i = 0; i < N; ++i) {
            *this & v[i];
        }
        return *this;
    }

    void copy(const void *src, size_t n)
    {
        memcpy((void *)ptr, src, n);
        ptr += n;
    }

};


//
// Resetter
//

#define RESET(type) \
SerResetter& operator&(type& v) \
{ \
v = (type)0; \
return *this; \
}

#define RESET_STRUCT(type) \
SerResetter& operator&(type& v) \
{ \
v.applyToItems(*this); \
return *this; \
}

class SerResetter
{
public:

    SerResetter()
    {
    }

    RESET(bool)
    RESET(char)
    RESET(signed char)
    RESET(unsigned char)
    RESET(short)
    RESET(unsigned short)
    RESET(int)
    RESET(unsigned int)
    RESET(long)
    RESET(unsigned long)
    RESET(long long)
    RESET(unsigned long long)
    RESET(float)
    RESET(double)
    RESET(AmigaModel)
    RESET(MemorySource)
    RESET(EventID)
    RESET(BusOwner)
    RESET(SprDMAState)
    RESET(FilterType)
    RESET(SerialPortDevice)
    RESET(DriveType)
    RESET(DriveState)
    RESET(KeyboardState)
    RESET(DrawingMode)

    RESET_STRUCT(Event)
    RESET_STRUCT(Beam)
    RESET_STRUCT(Change)
    template <uint16_t capacity> RESET_STRUCT(ChangeRecorder<capacity>)
    RESET_STRUCT(RegisterChange)
    RESET_STRUCT(ChangeHistory)

    template <class T, size_t N>
    SerResetter& operator&(T (&v)[N])
    {
        for(size_t i = 0; i < N; ++i) {
            *this & v[i];
        }
        return *this;
    }
};

#endif
