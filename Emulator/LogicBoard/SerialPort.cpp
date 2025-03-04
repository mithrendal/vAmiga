// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "SerialPort.h"
#include "IOUtils.h"
#include "Amiga.h"

namespace vamiga {

void
SerialPort::resetConfig()
{
    assert(isPoweredOff());
    auto &defaults = amiga.defaults;

    std::vector <Option> options = {
        
        OPT_SERIAL_DEVICE
    };

    for (auto &option : options) {
        setConfigItem(option, defaults.get(option));
    }
}

i64
SerialPort::getConfigItem(Option option) const
{
    switch (option) {
            
        case OPT_SERIAL_DEVICE:  return (i64)config.device;

        default:
            fatalError;
    }
}

void
SerialPort::setConfigItem(Option option, i64 value)
{
    switch (option) {
            
        case OPT_SERIAL_DEVICE:
            
            if (!SerialPortDeviceEnum::isValid(value)) {
                throw VAError(ERROR_OPT_INVARG, SerialPortDeviceEnum::keyList());
            }
            
            config.device = (SerialPortDevice)value;
            return;

        default:
            fatalError;
    }
}

void
SerialPort::_inspect() const
{
    {   SYNCHRONIZED
        
        info.port = port;
        info.txd = getTXD();
        info.rxd = getRXD();
        info.rts = getRTS();
        info.cts = getCTS();
        info.dsr = getDSR();
        info.cd = getCD();
        info.dtr = getDTR();
    }
}

void
SerialPort::_dump(Category category, std::ostream& os) const
{
    using namespace util;
    
    if (category == Category::Config) {
        
        os << tab("device");
        os << SerialPortDeviceEnum::key(config.device) << std::endl;
    }
    
    if (category == Category::Inspection) {
        
        os << tab("Port pins");
        os << hex(port);
        os << tab("TXD");
        os << dec(getTXD());
        os << tab("RXD");
        os << dec(getRXD());
        os << tab("RTS");
        os << dec(getRTS());
        os << tab("CTS");
        os << dec(getCTS());
        os << tab("DSR");
        os << dec(getDSR());
        os << tab("CD");
        os << dec(getCD());
        os << tab("DTR");
        os << dec(getDTR());
        os << tab("RI");
        os << dec(getRI());
    }
}

bool
SerialPort::getPin(isize nr) const
{
    assert(nr >= 1 && nr <= 25);

    bool result = GET_BIT(port, nr);

    // debug(SER_DEBUG, "getPin(%d) = %d port = %X\n", nr, result, port);
    return result;
}

void
SerialPort::setPin(isize nr, bool value)
{
    // debug(SER_DEBUG, "setPin(%d,%d)\n", nr, value);
    assert(nr >= 1 && nr <= 25);

    setPort(1 << nr, value);
}

void
SerialPort::setPort(u32 mask, bool value)
{
    u32 oldPort = port;

    /* Emulate the loopback cable (if connected)
     *
     *     Connected pins: A: 2 - 3       (TXD - RXD)
     *                     B: 4 - 5 - 6   (RTS - CTS - DSR)
     *                     C: 8 - 20 - 22 (CD - DTR - RI)
     */
    if (config.device == SPD_LOOPBACK) {

        u32 maskA = TXD_MASK | RXD_MASK;
        u32 maskB = RTS_MASK | CTS_MASK | DSR_MASK;
        u32 maskC = CD_MASK | DTR_MASK | RI_MASK;

        if (mask & maskA) mask |= maskA;
        if (mask & maskB) mask |= maskB;
        if (mask & maskC) mask |= maskC;
    }

    // Change the port pins
    if (value) port |= mask; else port &= ~mask;

    // Inform the UART if RXD has changed
    if ((oldPort ^ port) & RXD_MASK) uart.rxdHasChanged(value);
}

}
