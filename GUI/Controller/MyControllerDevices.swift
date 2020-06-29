// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

extension MyController {
    
    //
    // Keyboard
    //
    
    // Keyboard events are handled by the emulator window.
    // If they are handled here, some keys such as 'TAB' don't trigger an event.
    
    //
    //  Game ports
    //
    
    var gamePad1: GamePad? { return gamePadManager.gamePads[config.gameDevice1] }
    var gamePad2: GamePad? { return gamePadManager.gamePads[config.gameDevice2] }

    // Feeds game pad actions into the port associated with a certain game pad
    @discardableResult
    func emulateEventsOnGamePort(slot: Int, events: [GamePadAction]) -> Bool {
        
        if slot == config.gameDevice1 {
            return emulateEventsOnGamePort1(events)
        }
        if slot == config.gameDevice2 {
            return emulateEventsOnGamePort2(events)
        }
        return false
    }
    
    // Feeds game pad actions into the Amiga's first control port
    func emulateEventsOnGamePort1(_ events: [GamePadAction]) -> Bool {
        
        for event in events {
            amiga.joystick1.trigger(event)
            amiga.mouse1.trigger(event)
        }
        return events != []
    }
    
    // Feeds game pad actions into the Amiga's second control port
    func emulateEventsOnGamePort2(_ events: [GamePadAction]) -> Bool {
        
        for event in events {
            amiga.joystick2.trigger(event)
            amiga.mouse2.trigger(event)
        }
        return events != []
    }
}
