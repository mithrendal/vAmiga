// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

import AppKit

public extension MetalView {

    //
    // Keyboard events
    //
    
    override func keyDown(with event: NSEvent) {

        parent.keyboard.keyDown(with: event)
    }
    
    override func keyUp(with event: NSEvent) {

        parent.keyboard.keyUp(with: event)
    }
    
    override func flagsChanged(with event: NSEvent) {
        
        parent.keyboard.flagsChanged(with: event)
    }
    
    //
    // Mouse events
    //
    
    func retainMouse() {
        
        NSCursor.hide()
        CGAssociateMouseAndMouseCursorPosition(boolean_t(truncating: false))
        gotMouse = true
    }
    
    func releaseMouse() {
        
        NSCursor.unhide()
        CGAssociateMouseAndMouseCursorPosition(boolean_t(truncating: true))
        gotMouse = false
    }
    
    override func updateTrackingAreas() {

        let options: NSTrackingArea.Options = [ .activeInKeyWindow, .mouseEnteredAndExited ]
        
        if trackingArea != nil {
            removeTrackingArea(trackingArea!)
        }
        trackingArea = NSTrackingArea(rect: self.bounds, options: options, owner: self, userInfo: nil)
        addTrackingArea(trackingArea!)
    }
    
    override func mouseEntered(with event: NSEvent) {
        
        insideTrackingArea = true
        
        // Check if we need to retain the mouse
        if prefs.retainMouseByEntering {
   
            // Only retain if the user didn't shake the mouse recently
            if DispatchTime.diffMilliSec(lastShake) > UInt64(500) {
                retainMouse()
            } else {
                debug(.events, "Last shake too recent")
            }
        }
    }
    
    override func mouseExited(with event: NSEvent) {
        
        insideTrackingArea = false
        releaseMouse()
    }
    
    override func mouseDown(with event: NSEvent) {
        
        if !gotMouse {
            if prefs.retainMouseByClick && insideTrackingArea { retainMouse() }
            return
        }
        
        if mouse2 == nil || event.deviceID != 0 {
            mouse1!.processMouseEvents(events: [.PRESS_LEFT])
        } else {
            mouse2!.processMouseEvents(events: [.PRESS_LEFT])
        }
    }
    
    override func mouseUp(with event: NSEvent) {
        
        if !gotMouse { return }

        if mouse2 == nil || event.deviceID != 0 {
            mouse1!.processMouseEvents(events: [.RELEASE_LEFT])
        } else {
            mouse2!.processMouseEvents(events: [.RELEASE_LEFT])
        }
    }
    
    override func rightMouseDown(with event: NSEvent) {

        if !gotMouse { return }

        if mouse2 == nil || event.deviceID != 0 {
            mouse1!.processMouseEvents(events: [.PRESS_RIGHT])
        } else {
            mouse2!.processMouseEvents(events: [.PRESS_RIGHT])
        }
    }

    override func rightMouseUp(with event: NSEvent) {
        
        if !gotMouse { return }
        
        if mouse2 == nil || event.deviceID != 0 {
            mouse1!.processMouseEvents(events: [.RELEASE_RIGHT])
        } else {
            mouse2!.processMouseEvents(events: [.RELEASE_RIGHT])
        }
    }

    override func otherMouseDown(with event: NSEvent) {

        if !gotMouse { return }

        if mouse2 == nil || event.deviceID != 0 {
            mouse1!.processMouseEvents(events: [.PRESS_MIDDLE])
        } else {
            mouse2!.processMouseEvents(events: [.PRESS_MIDDLE])
        }
    }

    override func otherMouseUp(with event: NSEvent) {

        if !gotMouse { return }

        if mouse2 == nil || event.deviceID != 0 {
            mouse1!.processMouseEvents(events: [.RELEASE_MIDDLE])
        } else {
            mouse2!.processMouseEvents(events: [.RELEASE_MIDDLE])
        }
    }

    override func mouseMoved(with event: NSEvent) {
        
        if !gotMouse { return }
        
        // Determine delta movement steps
        let dx = event.deltaX
        let dy = event.deltaY
        
        // Make coordinate independent of the actual window size
        let scaleX = (256.0 * 400.0) / frame.width / 128.0
        let scaleY = (256.0 * 300.0) / frame.height / 128.0
        let dxdy = NSPoint(x: dx * scaleX, y: dy * scaleY)
        
        // Report the new location to the Amiga mouse
        if mouse2 == nil || event.deviceID != 0 {
            mouse1!.processMouseEvents(delta: dxdy)
        } else {
            mouse2!.processMouseEvents(delta: dxdy)
        }
    }
    
    override func mouseDragged(with event: NSEvent) {

        mouseMoved(with: event)
    }
    
    override func rightMouseDragged(with event: NSEvent) {

        mouseMoved(with: event)
    }
    
    func checkForMouseKeys(with event: NSEvent) -> Bool {
                
        if !gotMouse && prefs.retainMouseWithKeys {
            
            switch prefs.retainMouseKeyComb {
              
            case 0 where event.modifierFlags.contains([.option, .command]),
                 1 where event.modifierFlags.contains([.option, .control]):
                
                retainMouse()
                return true
                
            default: break
            }

        } else if gotMouse && prefs.releaseMouseWithKeys {
            
            switch prefs.releaseMouseKeyComb {
                
            case 0 where event.modifierFlags.contains([.option, .command]),
                 1 where event.modifierFlags.contains([.option, .control]):
                
                releaseMouse()
                return true
                
            default: break
            }
        }
        return false
    }
}
