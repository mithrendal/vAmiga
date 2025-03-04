// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "SerialPortTypes.h"
#include "SubComponent.h"

namespace vamiga {

#define TXD_MASK (1 << 2)
#define RXD_MASK (1 << 3)
#define RTS_MASK (1 << 4)
#define CTS_MASK (1 << 5)
#define DSR_MASK (1 << 6)
#define CD_MASK  (1 << 8)
#define DTR_MASK (1 << 20)
#define RI_MASK  (1 << 22)

class SerialPort : public SubComponent {

    // Current configuration
    SerialPortConfig config = {};

    // Result of the latest inspection
    mutable SerialPortInfo info = {};

    // The current values of the port pins
    u32 port = 0;

    
    //
    // Initializing
    //

public:

    using SubComponent::SubComponent;

    
    //
    // Methods from AmigaObject
    //
    
private:
    
    const char *getDescription() const override { return "SerialPort"; }
    void _dump(Category category, std::ostream& os) const override;
    
    
    //
    // Methods from AmigaComponent
    //
    
private:
    
    void _reset(bool hard) override { RESET_SNAPSHOT_ITEMS(hard) };
    void _inspect() const override;
    
    template <class T>
    void applyToPersistentItems(T& worker)
    {
        worker

        << config.device;
    }

    template <class T>
    void applyToResetItems(T& worker, bool hard = true)
    {
        worker

        << port;
    }

    isize _size() override { COMPUTE_SNAPSHOT_SIZE }
    u64 _checksum() override { COMPUTE_SNAPSHOT_CHECKSUM }
    isize _load(const u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    isize _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }

    
    //
    // Configuring
    //
    
public:

    const SerialPortConfig &getConfig() const { return config; }
    void resetConfig() override;

    i64 getConfigItem(Option option) const;
    void setConfigItem(Option option, i64 value);
    

    //
    // Analyzing
    //
    
public:

    SerialPortInfo getInfo() const { return AmigaComponent::getInfo(info); }


    //
    // Accessing
    //

public:

    // Reads the current value of a certain port pin
    bool getPin(isize nr) const;

    // Modifies the value of a certain port pin
    void setPin(isize nr, bool value);

    // Convenience wrappers
    bool getTXD() const { return getPin(2); }
    bool getRXD() const { return getPin(3); }
    bool getRTS() const { return getPin(4); }
    bool getCTS() const { return getPin(5); }
    bool getDSR() const { return getPin(6); }
    bool getCD () const { return getPin(8); }
    bool getDTR() const { return getPin(20); }
    bool getRI () const { return getPin(22); }

    void setTXD(bool value) { setPin(2, value); }
    void setRXD(bool value) { setPin(3, value); }
    void setRTS(bool value) { setPin(4, value); }
    void setCTS(bool value) { setPin(5, value); }
    void setDSR(bool value) { setPin(6, value); }
    void setCD (bool value) { setPin(8, value); }
    void setDTR(bool value) { setPin(20, value); }
    void setRI (bool value) { setPin(22, value); }

private:

    void setPort(u32 mask, bool value);
};

}
