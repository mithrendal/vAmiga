// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

import AppKit

class ConfigurationController: DialogController {

    var config: Configuration { return parent.config }
    
    @IBOutlet weak var prefTabView: NSTabView!
        
    //
    // Rom preferences
    //
        
    // Rom info
    @IBOutlet weak var romDropView: RomDropView!
    @IBOutlet weak var romTitle: NSTextField!
    @IBOutlet weak var romSubtitle: NSTextField!
    @IBOutlet weak var romSubsubtitle: NSTextField!
    @IBOutlet weak var romModel: NSTextField!
    @IBOutlet weak var romDeleteButton: NSButton!

    @IBOutlet weak var extDropView: ExtRomDropView!
    @IBOutlet weak var extTitle: NSTextField!
    @IBOutlet weak var extSubtitle: NSTextField!
    @IBOutlet weak var extSubsubtitle: NSTextField!
    @IBOutlet weak var extModel: NSTextField!
    @IBOutlet weak var extDeleteButton: NSButton!
    @IBOutlet weak var extMapText: NSTextField!
    @IBOutlet weak var extMapAddr: NSPopUpButton!

    @IBOutlet weak var arosButton: NSButton!

    // Lock
    @IBOutlet weak var romLockImage: NSButton!
    @IBOutlet weak var romLockText: NSTextField!
    @IBOutlet weak var romLockSubText: NSTextField!
   
    // Buttons
    @IBOutlet weak var romArosButton: NSButton!
    @IBOutlet weak var romOKButton: NSButton!
    @IBOutlet weak var romPowerButton: NSButton!

    //
    // Chipset preferences
    //
    
    // CPU
    @IBOutlet weak var csCpuRevision: NSPopUpButton!
    @IBOutlet weak var csCpuInfo1: NSTextField!
    @IBOutlet weak var csCpuInfo2: NSTextField!
    @IBOutlet weak var csCpuInfo3: NSTextField!

    // Agnus
    @IBOutlet weak var csAgnusRevision: NSPopUpButton!
    @IBOutlet weak var csAgnusInfo1: NSTextField!
    @IBOutlet weak var csAgnusInfo2: NSTextField!
    @IBOutlet weak var csAgnusInfo3: NSTextField!

    // Denise
    @IBOutlet weak var csDeniseRevision: NSPopUpButton!
    @IBOutlet weak var csDeniseInfo1: NSTextField!
    @IBOutlet weak var csDeniseInfo2: NSTextField!
    @IBOutlet weak var csDeniseInfo3: NSTextField!

    // CIAs
    @IBOutlet weak var csCiaRevision: NSPopUpButton!
    @IBOutlet weak var csCiaInfo1: NSTextField!
    @IBOutlet weak var csCiaInfo2: NSTextField!
    @IBOutlet weak var csCiaInfo3: NSTextField!

    // RT clock
    @IBOutlet weak var csRtcIcon: NSButton!
    @IBOutlet weak var csRtcRevision: NSPopUpButton!
    @IBOutlet weak var csRtcInfo1: NSTextField!
    @IBOutlet weak var csRtcInfo2: NSTextField!
    @IBOutlet weak var csRtcInfo3: NSTextField!
    
    // Lock
    @IBOutlet weak var csLockImage: NSButton!
    @IBOutlet weak var csLockText: NSTextField!
    @IBOutlet weak var csLockSubText: NSTextField!

    // Buttons
    @IBOutlet weak var csFactorySettingsPopup: NSPopUpButton!
    @IBOutlet weak var csOKButton: NSButton!
    @IBOutlet weak var csPowerButton: NSButton!

    //
    // Memory preferences
    //
    
    // RAM
    @IBOutlet weak var memChipRamPopup: NSPopUpButton!
    @IBOutlet weak var memSlowRamPopup: NSPopUpButton!
    @IBOutlet weak var memFastRamPopup: NSPopUpButton!
    @IBOutlet weak var memRamInitPattern: NSPopUpButton!
    @IBOutlet weak var memBankMap: NSPopUpButton!
    @IBOutlet weak var memUnmappingType: NSPopUpButton!
    
    // Chipset features
    @IBOutlet weak var memSlowRamMirror: NSButton!
    @IBOutlet weak var memSlowRamDelay: NSButton!

    // Warning
    @IBOutlet weak var memWarnImage: NSButton!
    @IBOutlet weak var memWarnInfo1: NSTextField!
    @IBOutlet weak var memWarnInfo2: NSTextField!

    // Lock
    @IBOutlet weak var memLockImage: NSButton!
    @IBOutlet weak var memLockText: NSTextField!

    // Buttons
    @IBOutlet weak var memFactorySettingsPopup: NSPopUpButton!
    @IBOutlet weak var memOKButton: NSButton!
    @IBOutlet weak var memPowerButton: NSButton!

    //
    // Peripherals preferences
    //

    // Drive
    @IBOutlet weak var perDf0Type: NSPopUpButton!
    @IBOutlet weak var perDf1Connect: NSButton!
    @IBOutlet weak var perDf1Type: NSPopUpButton!
    @IBOutlet weak var perDf2Connect: NSButton!
    @IBOutlet weak var perDf2Type: NSPopUpButton!
    @IBOutlet weak var perDf3Connect: NSButton!
    @IBOutlet weak var perDf3Type: NSPopUpButton!

    // Disks
    @IBOutlet weak var perDriveBlankDiskFormat: NSPopUpButton!
    @IBOutlet weak var perBootCode: NSPopUpButton!
    
    // Ports
    @IBOutlet weak var perGameDevice1: NSPopUpButton!
    @IBOutlet weak var perGameDevice2: NSPopUpButton!
    @IBOutlet weak var perSerialDevice: NSPopUpButton!
    @IBOutlet weak var perSerialPort: NSTextField!
    @IBOutlet weak var perSerialPortText: NSTextField!

    // Lock
    @IBOutlet weak var perLockImage: NSButton!
    @IBOutlet weak var perLockText: NSTextField!
    @IBOutlet weak var perLockSubText: NSTextField!
    
    // Buttons
    @IBOutlet weak var perFactorySettingsPopup: NSPopUpButton!
    @IBOutlet weak var perOKButton: NSButton!
    @IBOutlet weak var perPowerButton: NSButton!

    //
    // Compatibility preferences
    //

    // Graphics
    @IBOutlet weak var compClxSprSpr: NSButton!
    @IBOutlet weak var compClxSprPlf: NSButton!
    @IBOutlet weak var compClxPlfPlf: NSButton!

    // Blitter
    @IBOutlet weak var compBltAccuracy: NSSlider!
    @IBOutlet weak var compBltLevel2: NSTextField!
    @IBOutlet weak var compBltLevel1: NSTextField!

    // Chipset features
    @IBOutlet weak var compTodBug: NSButton!

    // Timing
    @IBOutlet weak var compEClockSyncing: NSButton!
    
    // Disk controller
    @IBOutlet weak var compDriveSpeed: NSPopUpButton!
    @IBOutlet weak var compMechanicalDelays: NSButton!
    @IBOutlet weak var compLockDskSync: NSButton!
    @IBOutlet weak var compAutoDskSync: NSButton!

    // Keyboard
    @IBOutlet weak var compAccurateKeyboard: NSButton!

    // Lock
    @IBOutlet weak var compLockText: NSTextField!
    @IBOutlet weak var compLockSubText: NSTextField!

    // Buttons
    @IBOutlet weak var compOKButton: NSButton!
    @IBOutlet weak var compPowerButton: NSButton!
    
    //
    // Audio preferences
    //
    
    // In
    @IBOutlet weak var audVol0: NSSlider!
    @IBOutlet weak var audVol1: NSSlider!
    @IBOutlet weak var audVol2: NSSlider!
    @IBOutlet weak var audVol3: NSSlider!
    @IBOutlet weak var audPan0: NSSlider!
    @IBOutlet weak var audPan1: NSSlider!
    @IBOutlet weak var audPan2: NSSlider!
    @IBOutlet weak var audPan3: NSSlider!
    
    // Out
    @IBOutlet weak var audVolL: NSSlider!
    @IBOutlet weak var audVolR: NSSlider!
    @IBOutlet weak var audSamplingMethod: NSPopUpButton!

    // Drive volumes
    @IBOutlet weak var audDf0Pan: NSSlider!
    @IBOutlet weak var audDf1Pan: NSSlider!
    @IBOutlet weak var audDf2Pan: NSSlider!
    @IBOutlet weak var audDf3Pan: NSSlider!
    @IBOutlet weak var audStepVolume: NSSlider!
    @IBOutlet weak var audPollVolume: NSSlider!
    @IBOutlet weak var audEjectVolume: NSSlider!
    @IBOutlet weak var audInsertVolume: NSSlider!

    // Filter
    @IBOutlet weak var audFilterType: NSPopUpButton!
    @IBOutlet weak var audFilterAlwaysOn: NSButton!

    // Buttons
    @IBOutlet weak var audOKButton: NSButton!
    @IBOutlet weak var audPowerButton: NSButton!

    //
    // Video preferences
    //
    
    // Palette
    @IBOutlet weak var vidPalettePopUp: NSPopUpButton!
    @IBOutlet weak var vidBrightnessSlider: NSSlider!
    @IBOutlet weak var vidContrastSlider: NSSlider!
    @IBOutlet weak var vidSaturationSlider: NSSlider!
    
    // Effects
    @IBOutlet weak var vidEnhancerPopUp: NSPopUpButton!
    @IBOutlet weak var vidUpscalerPopUp: NSPopUpButton!
    @IBOutlet weak var vidBlurPopUp: NSPopUpButton!
    @IBOutlet weak var vidBlurRadiusSlider: NSSlider!
    
    @IBOutlet weak var vidBloomPopUp: NSPopUpButton!
    @IBOutlet weak var vidBloomRadiusSlider: NSSlider!
    @IBOutlet weak var vidBloomBrightnessSlider: NSSlider!
    @IBOutlet weak var vidBloomWeightSlider: NSSlider!

    @IBOutlet weak var vidFlickerPopUp: NSPopUpButton!
    @IBOutlet weak var vidFlickerWeightSlider: NSSlider!

    @IBOutlet weak var vidDotMaskPopUp: NSPopUpButton!
    @IBOutlet weak var vidDotMaskBrightnessSlider: NSSlider!
    
    @IBOutlet weak var vidScanlinesPopUp: NSPopUpButton!
    @IBOutlet weak var vidScanlineBrightnessSlider: NSSlider!
    @IBOutlet weak var vidScanlineWeightSlider: NSSlider!
    
    @IBOutlet weak var vidMisalignmentPopUp: NSPopUpButton!
    @IBOutlet weak var vidMisalignmentXSlider: NSSlider!
    @IBOutlet weak var vidMisalignmentYSlider: NSSlider!
    
    // Geometry
    @IBOutlet weak var vidHCenter: NSSlider!
    @IBOutlet weak var vidVCenter: NSSlider!
    @IBOutlet weak var vidHZoom: NSSlider!
    @IBOutlet weak var vidVZoom: NSSlider!

    // Buttons
    @IBOutlet weak var vidOKButton: NSButton!
    @IBOutlet weak var vidPowerButton: NSButton!

    var bootable: Bool {
        
        do { try amiga.isReady() } catch { return false }
        return amiga.poweredOff
    }
    
    // The tab to open first
    var firstTab = ""
    
    func showSheet(tab: String) {

        firstTab = tab
        showSheet()
    }

    override func awakeFromNib() {

        super.awakeFromNib()
        awakeVideoPrefsFromNib()
    }

    override func sheetWillShow() {
        
        if firstTab != "" { prefTabView?.selectTabViewItem(withIdentifier: firstTab) }
        refresh()
    }

    func refresh() {
        
        if let id = prefTabView.selectedTabViewItem?.identifier as? String {
            
            switch id {
            case "Roms": refreshRomTab()
            case "Chipset": refreshChipsetTab()
            case "Memory": refreshMemoryTab()
            case "Peripherals": refreshPeripheralsTab()
            case "Compatibility": refreshCompatibilityTab()
            case "Audio": refreshAudioTab()
            case "Video": refreshVideoTab()
            default: fatalError()
            }
        }
    }
    
    @IBAction func unlockAction(_ sender: Any!) {

        amiga.pause()
        amiga.powerOff()
        refresh()
    }
    
    @IBAction override func okAction(_ sender: Any!) {
        
        hideSheet()
    }

    @IBAction func powerAction(_ sender: Any!) {
        
        hideSheet()
        amiga.powerOn()
        try? amiga.run()
    }
}

extension ConfigurationController: NSTabViewDelegate {

    func tabView(_ tabView: NSTabView, didSelect tabViewItem: NSTabViewItem?) {

        refresh()
    }
}
