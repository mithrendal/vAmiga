// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

import CoreGraphics

extension MyController: NSWindowDelegate {
        
    public func windowDidBecomeMain(_ notification: Notification) {
        
        guard let window = notification.object as? NSWindow else { return }
        
        // Inform the application delegate
        myAppDelegate.windowDidBecomeMain(window)
        
        // Restart the emulator if it was paused when the window lost focus
        if pref.pauseInBackground && pauseInBackgroundSavedState { try? amiga.run() }

        // Register to receive mouse movement events
        window.acceptsMouseMovedEvents = true
        
        // Make sure the aspect ratio is correct
        adjustWindowSize()

        // Update the status bar
        refreshStatusBar()
    }
    
    public func windowDidResignMain(_ notification: Notification) {
        
        // Stop emulator if it is configured to pause in background
        if amiga != nil {
            pauseInBackgroundSavedState = amiga.running
            if pref.pauseInBackground { amiga.pause() }
        }
    }
    
    public func windowShouldClose(_ sender: NSWindow) -> Bool {
        
        debug(.lifetime)

        if proceedWithUnsavedFloppyDisks() {
            return true
        } else {
            return false
        }
    }

    public func windowWillClose(_ notification: Notification) {
        
        debug(.lifetime)
                
        debug(.shutdown, "Stopping renderer...")
        renderer.halt()
        
        debug(.shutdown, "Stopping timers...")
        snapshotTimer?.invalidate()
        snapshotTimer = nil

        debug(.shutdown, "Closing auxiliary windows...")
        configurator?.amiga = nil
        configurator?.close()
        inspector?.amiga = nil
        inspector?.close()
        monitor?.amiga = nil
        monitor?.close()
        virtualKeyboard?.amiga = nil
        virtualKeyboard?.close()

        debug(.shutdown, "Shutting down the audio backend...")
        macAudio.shutDown()

        debug(.shutdown, "Disconnecting gaming devices...")
        gamePadManager.shutDown()
        
        debug(.shutdown, "Shutting down the emulator...")
        amiga.halt()
    }
    
    func shutDown() {
                
        debug(.shutdown, "Removing proxy...")
        
        amiga.kill()
        amiga = nil
    }
    
    public func windowWillEnterFullScreen(_ notification: Notification) {

        debug(.lifetime)

        renderer.fullscreen = true
        showStatusBar(false)
    }
    
    public func windowDidEnterFullScreen(_ notification: Notification) {

        debug(.lifetime)
        renderer.monitors.updateMonitorPositions()
    }
    
    public func windowWillExitFullScreen(_ notification: Notification) {

        debug(.lifetime)
        renderer.fullscreen = false
        showStatusBar(true)
    }
    
    public func windowDidExitFullScreen(_ notification: Notification) {

        debug(.lifetime)
        renderer.monitors.updateMonitorPositions()
    }
    
    public func window(_ window: NSWindow, willUseFullScreenPresentationOptions proposedOptions: NSApplication.PresentationOptions = []) -> NSApplication.PresentationOptions {
        
        debug(.lifetime)
        
        let autoHideToolbar = NSApplication.PresentationOptions.autoHideToolbar
        var options = NSApplication.PresentationOptions(rawValue: autoHideToolbar.rawValue)
        options.insert(proposedOptions)
        return options
    }
    
    public func window(_ window: NSWindow, willUseFullScreenContentSize proposedSize: NSSize) -> NSSize {

        var myRect = metal.bounds
        myRect.size = proposedSize
        return proposedSize
    }
    
    // Fixes a NSSize to match our desired aspect ration
    func fixSizeX(window: NSWindow, size: NSSize) -> NSSize {
        
        // Get some basic parameters
        let windowFrame = window.frame
        let deltaX = size.width - windowFrame.size.width
        let deltaY = size.height - windowFrame.size.height
        
        // How big would the metal view become?
        let metalFrame = metal.frame
        let metalX = metalFrame.size.width + deltaX
        let metalY = metalFrame.size.height + deltaY
        
        // We want to achieve an aspect ratio of 4:3
        let newMetalX  = metalY * (4.0 / 3.0)
        let dx = newMetalX - metalX
        
        return NSSize(width: size.width + dx, height: size.height)
    }

    func fixSizeY(window: NSWindow, size: NSSize) -> NSSize {

        // Get some basic parameters
        let windowFrame = window.frame
        let deltaX = size.width - windowFrame.size.width
        let deltaY = size.height - windowFrame.size.height

        // How big would the metal view become?
        let metalFrame = metal.frame
        let metalX = metalFrame.size.width + deltaX
        let metalY = metalFrame.size.height + deltaY

        // We want to achieve an aspect ratio of 4:3
        let newMetalY  = metalX * (3.0 / 4.0)
        let dy = newMetalY - metalY

        return NSSize(width: size.width, height: size.height + dy)
    }

    // Fixes a NSRect to match our desired aspect ration
    func fixRect(window: NSWindow, rect: NSRect) -> NSRect {
        
        let newSize = fixSizeX(window: window, size: rect.size)
        let newOriginX = (rect.width - newSize.width) / 2.0
        
        return NSRect(x: newOriginX, y: 0, width: newSize.width, height: newSize.height)
    }
    
    public func windowWillResize(_ sender: NSWindow, to frameSize: NSSize) -> NSSize {
        
        return fixSizeX(window: sender, size: frameSize)
    }
    
    public func windowDidResize(_ notification: Notification) {
        
        renderer?.console.resize()
    }
    
    public func windowWillUseStandardFrame(_ window: NSWindow,
                                           defaultFrame newFrame: NSRect) -> NSRect {

        return fixRect(window: window, rect: newFrame)
    }

    public func windowDidChangeScreen(_ notification: Notification) {

        debug(.vsync)
    }

    public func windowDidChangeScreenProfile(_ notification: Notification) {

        debug(.vsync)
    }
}

extension MyController {
    
    func adjustWindowSize(dy: CGFloat = 0.0) {

        // Only proceed in window mode
        if renderer?.fullscreen == true { return }
        
        // Get window frame
        guard var frame = window?.frame else { return }
        
        // Modify the frame height
        frame.origin.y -= dy
        frame.size.height += dy

        // Compute the size correction
        let newsize = windowWillResize(window!, to: frame.size)
        let yCorrection = newsize.height - frame.size.height
        
        // Adjust frame
        frame.origin.y -= yCorrection
        frame.size = newsize
        
        window!.setFrame(frame, display: true)
    }

    func adjustWindowSize(height: CGFloat) {

        // Only proceed in window mode
        if renderer?.fullscreen == true { return }

        // Get window frame
        guard var frame = window?.frame else { return }
        let yCorrection = height - metal.frame.height

        // Modify the frame height
        let borderHeight = frame.height - metal.frame.height
        frame.size.height = round(height) + borderHeight

        // Compute the size correction
        let newSize = fixSizeX(window: window!, size: frame.size)

        // Adjust frame
        frame.origin.y -= yCorrection
        frame.size = newSize

        window!.setFrame(frame, display: true)
    }
}
