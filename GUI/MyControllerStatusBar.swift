// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

extension MyController {

    func cmdKeyIcon(_ state: Bool) -> NSImage? {

        return NSImage(named: state ? "amigaKeyTemplate" : "cmdKeyTemplate")
    }

    var hourglassIcon: NSImage? {
        
        switch pref.warpMode {
        case .auto where amiga.warpMode:
            return NSImage(named: "hourglass3Template")
        case .auto:
            return NSImage(named: "hourglass1Template")
        case .off:
            return NSImage(named: "warpOffTemplate")
        case .on:
            return NSImage(named: "warpOnTemplate")
        }
    }
    
    func refreshStatusBar() {
        
        let running = amiga.running
        let halted = amiga.cpu.halted
        let warp = amiga.warpMode

        // Df0 - Df3
        for n in 0...3 where drv[n] != nil {
            
            let dfn = amiga.df(n)!
            
            refreshStatusBar(drive: n, led: dfn.ledIcon)
            refreshStatusBar(drive: n, cylinder: dfn.currentCyl)
            refreshStatusBar(drive: n, icon: dfn.templateIcon, toolTip: dfn.toolTip)
        }
        
        // Hd0 - Hd3
        for n in 4...7 where drv[n] != nil {
            
            let hdn = amiga.hd(n - 4)!
            
            refreshStatusBar(drive: n, led: hdn.ledIcon)
            refreshStatusBar(drive: n, cylinder: hdn.currentCyl)
            refreshStatusBar(drive: n, icon: hdn.templateIcon, toolTip: hdn.toolTip)
        }

        // Command key icon
        let mapLeft = myAppDelegate.mapLeftCmdKey
        let mapRight = myAppDelegate.mapRightCmdKey
        cmdLeftIcon.image = cmdKeyIcon(mapLeft)
        cmdRightIcon.image = cmdKeyIcon(mapRight)

        // Remote server icon
        debugIcon.image = amiga.remoteManager.icon
        
        // Warp mode icon
        warpIcon.image = hourglassIcon
        
        // Visibility
        let items: [NSView: Bool] = [
            
            powerLED: true,
            
            ledSlot0: true,
            ledSlot1: true,
            letSlot2: true,
            ledSlot3: true,
            iconSlot0: true,
            iconSlot1: true,
            iconSlot2: true,
            iconSlot3: true,
            cylSlot0: true,
            cylSlot1: true,
            cylSlot2: true,
            cylSlot3: true,

            haltIcon: halted,
            cmdLeftIcon: false, // mapLeft || mapRight,
            cmdRightIcon: false, // mapLeft || mapRight,
            debugIcon: true,
            muteIcon: warp || muted,

            warpIcon: running,
            activityType: running,
            activityInfo: running,
            activityBar: running
        ]
        
        for (item, visible) in items {
            item.isHidden = !visible || !statusBar
        }
    }
    
    fileprivate func refreshStatusBar(drive n: Int, led: NSImage?) {
        
        if let widget = drvLED[n] {

            widget.image = led
        }
    }
    
    func refreshStatusBar(drive n: Int, cylinder: Int) {
        
        if let widget = drvCyl[n] {
            
            widget.integerValue = cylinder
        }
    }

    func refreshStatusBar(drive n: Int, icon: NSImage?, toolTip: String?) {
    
        if let widget = drvIcon[n] {
        
            widget.image = icon
            widget.toolTip = toolTip
        }
    }

    func refreshStatusBar(writing: Bool?) {
        
        for n in 0...3 where drv[n] != nil {
            
            let dfn = amiga.df(n)!
            refreshStatusBar(drive: n, led: dfn.ledIcon)
        }            
    }
            
    func assignSlots() {
    
        // Wipe out the slot assignment table
        drv = Array(repeating: nil, count: 8)
        var nr = 0

        // Update slot assignments for Df0 - Df3
        if amiga.df0.isConnected { drv[0] = nr; nr += 1 }
        if amiga.df1.isConnected { drv[1] = nr; nr += 1 }
        if amiga.df2.isConnected { drv[2] = nr; nr += 1 }
        if amiga.df3.isConnected { drv[3] = nr; nr += 1 }

        // Update slot assignments for Hd0 - Hd3
        if amiga.hd0.isConnected, nr < 4 { drv[4] = nr; nr += 1 }
        if amiga.hd1.isConnected, nr < 4 { drv[5] = nr; nr += 1 }
        if amiga.hd2.isConnected, nr < 4 { drv[6] = nr; nr += 1 }
        if amiga.hd3.isConnected, nr < 4 { drv[7] = nr; nr += 1 }
        
        // Update reference tables
        for device in 0...7 {
            
            if let nr = drv[device] {
                                
                drvLED[device] = ledSlot[nr]
                drvCyl[device] = cylSlot[nr]
                drvIcon[device] = iconSlot[nr]
                            
            } else {
                
                drvLED[device] = nil
                drvCyl[device] = nil
                drvIcon[device] = nil
            }
        }
        
        // Clean up unused slots
        for slot in nr ..< 4 {
            
            ledSlot[slot].image = nil
            cylSlot[slot].stringValue = ""
            iconSlot[slot].image = nil
        }
    }
    
    func showStatusBar(_ value: Bool) {
        
        if statusBar != value {
            
            if value {
                
                metal.adjustHeight(-26.0)
                window?.setContentBorderThickness(26.0, for: .minY)
                adjustWindowSize(dy: 26.0)

            } else {
                
                metal.adjustHeight(26.0)
                window?.setContentBorderThickness(0.0, for: .minY)
                adjustWindowSize(dy: -26.0)
            }
            
            statusBar = value
            refreshStatusBar()
        }
    }
    
    func updateSpeedometer() {

        func setColor(color: [NSColor]) {

            let min = activityBar.minValue
            let max = activityBar.maxValue
            let cur = (activityBar.doubleValue - min) / (max - min)

            let index =
            cur < 0.15 ? 0 :
            cur < 0.40 ? 1 :
            cur < 0.60 ? 2 :
            cur < 0.85 ? 3 : 4

            activityBar.fillColor = color[index]
        }

        let clock = amiga.cpu.clock

        speedometer.updateWith(cycle: clock,
                               emuFrame: Int64(amiga.agnus.frameCount),
                               gpuFrame: renderer.frames)

        // Set value
        switch activityType.selectedTag() {

        case 0:
            let mhz = speedometer.mhz
            activityBar.doubleValue = 10 * mhz
            activityInfo.stringValue = String(format: "%.2f MHz", mhz)
            setColor(color: [.systemRed, .systemYellow, .systemGreen, .systemYellow, .systemRed])

        case 1:
            let fps = speedometer.emuFps
            activityBar.doubleValue = fps
            activityInfo.stringValue = String(format: "%d Hz", Int(fps))
            setColor(color: [.systemRed, .systemYellow, .systemGreen, .systemYellow, .systemRed])

        case 2:
            let cpu = amiga.cpuLoad
            activityBar.integerValue = cpu
            activityInfo.stringValue = String(format: "%d%% CPU", cpu)
            setColor(color: [.systemGreen, .systemGreen, .systemGreen, .systemYellow, .systemRed])

        case 3:
            let fps = speedometer.gpsFps
            activityBar.doubleValue = fps
            activityInfo.stringValue = String(format: "%d FPS", Int(fps))
            setColor(color: [.systemRed, .systemYellow, .systemGreen, .systemYellow, .systemRed])

        case 4:
            let fill = amiga.paula.muxerStats.fillLevel * 100.0
            activityBar.doubleValue = fill
            activityInfo.stringValue = String(format: "Fill level %d%%", Int(fill))
            setColor(color: [.systemRed, .systemYellow, .systemGreen, .systemYellow, .systemRed])

        default:
            activityBar.integerValue = 0
            activityInfo.stringValue = "???"
        }
    }
    
    //
    // Action methods
    //

    @IBAction func activityTypeAction(_ sender: NSPopUpButton!) {
        
        var min, max, warn, crit: Double
        
        switch sender.selectedTag() {
        
        case 0: min = 0; max = 140; warn = 77; crit = 105
        case 1: min = 0; max = 120; warn = 75; crit = 100
        case 2: min = 0; max = 100; warn = 50; crit = 75
        case 3: min = 0; max = 120; warn = 75; crit = 100
        case 4: min = 0; max = 100; warn = 85; crit = 95

        default:
            fatalError()
        }
        
        activityBar.minValue = min
        activityBar.maxValue = max
        activityBar.warningValue = warn
        activityBar.criticalValue = crit

        refreshStatusBar()
    }
}
