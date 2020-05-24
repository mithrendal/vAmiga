// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Amiga.h"

/* A central element in the emulation of an Amiga is the accurate modeling of
* the DMA timeslot allocation table (Fig. 6-9 im the HRM, 3rd revision). All
* bitplane related events are managed in the BPL_SLOT. All disk, audio, and
* sprite related events are managed in the DAS_SLOT.
*
* vAmiga utilizes two event tables to schedule events in the DAS_SLOT and
* BPL_SLOT. Assuming that sprite DMA is enabled and Denise draws 6 bitplanes
* in lores mode starting at 0x28, the tables would look like this:
*
*     bplEvent[0x00] = EVENT_NONE   dasEvent[0x00] = EVENT_NONE
*     bplEvent[0x01] = EVENT_NONE   dasEvent[0x01] = BUS_REFRESH
*         ...                           ...
*     bplEvent[0x28] = EVENT_NONE   dasEvent[0x28] = EVENT_NONE
*     bplEvent[0x29] = BPL_L4       dasEvent[0x29] = DAS_S5_1
*     bplEvent[0x2A] = BPL_L6       dasEvent[0x2A] = EVENT_NONE
*     bplEvent[0x2B] = BPL_L2       dasEvent[0x2B] = DAS_S5_2
*     bplEvent[0x2C] = EVENT_NONE   dasEvent[0x2C] = EVENT_NONE
*     bplEvent[0x2D] = BPL_L3       dasEvent[0x2D] = DAS_S6_1
*     bplEvent[0x2E] = BPL_L5       dasEvent[0x2E] = EVENT_NONE
*     bplEvent[0x2F] = BPL_L1       dasEvent[0x2F] = DAS_S6_2
*         ...                           ...
*     bplEvent[0xE2] = BPL_EOL      dasEvent[0xE2] = BUS_REFRESH
*
* The BPL_EOL event doesn't perform DMA. It concludes the current line.
*
* All events in the BPL_SLOT can be superimposed by two drawing flags (bit 0
* and bit 1) that trigger the transfer of the data registers into the shift
* registers at the correct DMA cycle. Bit 0 controls the odd bitplanes and
* bit 1 controls the even bitplanes. Settings these flags changes the
* scheduled event, e.g.:
*
*     BPL_L4  becomes  BPL_L4_ODD   if bit 0 is set
*     BPL_L4  becomes  BPL_L4_EVEN  if bit 1 is set
*     BPL_L4  becomes  BPL_L4_ODD_EVEN  if both bits are set
*
* Each event table is accompanied by a jump table that points to the next
* event. Given the example tables above, the jump tables would look like this:
*
*     nextBplEvent[0x00] = 0x29     nextDasEvent[0x00] = 0x01
*     nextBplEvent[0x01] = 0x29     nextDasEvent[0x01] = 0x03
*           ...                           ...
*     nextBplEvent[0x28] = 0x29     nextDasEvent[0x28] = 0x29
*     nextBplEvent[0x29] = 0x2A     nextDasEvent[0x29] = 0x2B
*     nextBplEvent[0x2A] = 0x2B     nextDasEvent[0x2A] = 0x2B
*     nextBplEvent[0x2B] = 0x2D     nextDasEvent[0x2B] = 0x2D
*     nextBplEvent[0x2C] = 0x2D     nextDasEvent[0x2C] = 0x2D
*     nextBplEvent[0x2D] = 0x2E     nextDasEvent[0x2D] = 0x2F
*     nextBplEvent[0x2E] = 0x2F     nextDasEvent[0x2E] = 0x2F
*     nextBplEvent[0x2F] = 0x31     nextDasEvent[0x2F] = 0x31
*           ...                           ...
*     nextBplEvent[0xE2] = 0x00     nextDasEvent[0xE2] = 0x00
*
* Whenever one the DMA tables is modified, the corresponding jump table
* has to be updated, too.
*
* To quickly setup the event tables, vAmiga utilizes two static lookup
* tables. Depending on the current resoution, BPU value, or DMA status,
* segments of these lookup tables are copied to the event tables.
*
*      Table: bitplaneDMA[Resolution][Bitplanes][Cycle]
*
*             (Bitplane DMA events in a single rasterline)
*
*             Resolution : 0 or 1        (0 = LORES / 1 = HIRES)
*              Bitplanes : 0 .. 6        (Bitplanes in use, BPU)
*                  Cycle : 0 .. HPOS_MAX (DMA cycle)
*
*      Table: dasDMA[dmacon]
*
*             (Disk, Audio, and Sprite DMA events in a single rasterline)
*
*                 dmacon : Bits 0 .. 5 of register DMACON
*/

void
Agnus::initLookupTables()
{
    initBplEventTableLores();
    initBplEventTableHires();
    initDasEventTable();
}

void
Agnus::initBplEventTableLores()
{
    memset(bplDMA[0], 0, sizeof(bplDMA[0]));

    for (int bpu = 0; bpu < 7; bpu++) {

        EventID *p = &bplDMA[0][bpu][0];

        // Iterate through all 22 fetch units
        for (int i = 0; i <= 0xD8; i += 8, p += 8) {

            switch(bpu) {
                case 6: p[2] = BPL_L6;
                case 5: p[6] = BPL_L5;
                case 4: p[1] = BPL_L4;
                case 3: p[5] = BPL_L3;
                case 2: p[3] = BPL_L2;
                case 1: p[7] = BPL_L1;
            }
        }

        assert(bplDMA[0][bpu][HPOS_MAX] == EVENT_NONE);
        bplDMA[0][bpu][HPOS_MAX] = BPL_EOL;
    }
}

void
Agnus::initBplEventTableHires()
{
    memset(bplDMA[1], 0, sizeof(bplDMA[1]));

    for (int bpu = 0; bpu < 7; bpu++) {

        EventID *p = &bplDMA[1][bpu][0];

        for (int i = 0; i <= 0xD8; i += 8, p += 8) {

            switch(bpu) {
                case 6:
                case 5:
                case 4: p[0] = p[4] = BPL_H4;
                case 3: p[2] = p[6] = BPL_H3;
                case 2: p[1] = p[5] = BPL_H2;
                case 1: p[3] = p[7] = BPL_H1;
            }
        }

        assert(bplDMA[1][bpu][HPOS_MAX] == EVENT_NONE);
        bplDMA[1][bpu][HPOS_MAX] = BPL_EOL;
    }
}

void
Agnus::initDasEventTable()
{
    memset(dasDMA, 0, sizeof(dasDMA));

    for (int dmacon = 0; dmacon < 64; dmacon++) {

        EventID *p = dasDMA[dmacon];

        p[0x01] = DAS_REFRESH;

        if (dmacon & DSKEN) {
            p[0x07] = DAS_D0;
            p[0x09] = DAS_D1;
            p[0x0B] = DAS_D2;
        }
        
        /*
        if (dmacon & AUD0EN) p[0x0D] = DAS_A0;
        if (dmacon & AUD1EN) p[0x0F] = DAS_A1;
        if (dmacon & AUD2EN) p[0x11] = DAS_A2;
        if (dmacon & AUD3EN) p[0x13] = DAS_A3;
        */
        // Audio DMA is possible even in lines where the DMACON bits are false
        p[0x0D] = DAS_A0;
        p[0x0F] = DAS_A1;
        p[0x11] = DAS_A2;
        p[0x13] = DAS_A3;
        
        if (dmacon & SPREN) {
            p[0x15] = DAS_S0_1;
            p[0x17] = DAS_S0_2;
            p[0x19] = DAS_S1_1;
            p[0x1B] = DAS_S1_2;
            p[0x1D] = DAS_S2_1;
            p[0x1F] = DAS_S2_2;
            p[0x21] = DAS_S3_1;
            p[0x23] = DAS_S3_2;
            p[0x25] = DAS_S4_1;
            p[0x27] = DAS_S4_2;
            p[0x29] = DAS_S5_1;
            p[0x2B] = DAS_S5_2;
            p[0x2D] = DAS_S6_1;
            p[0x2F] = DAS_S6_2;
            p[0x31] = DAS_S7_1;
            p[0x33] = DAS_S7_2;
        }

        p[0xDF] = DAS_SDMA;
    }
}

u16
Agnus::peekDMACONR()
{
    u16 result = dmacon;

    assert((result & ((1 << 14) | (1 << 13))) == 0);

    if (blitter.isBusy()) result |= (1 << 14);
    if (blitter.isZero()) result |= (1 << 13);
    
    return result;
}

void
Agnus::pokeDMACON(u16 value)
{
    debug(DMA_DEBUG, "pokeDMACON(%X)\n", value);

    // Record the change
    recordRegisterChange(DMA_CYCLES(2), REG_DMACON, value);
}

void
Agnus::setDMACON(u16 oldValue, u16 value)
{
    debug(DMA_DEBUG, "setDMACON(%x, %x)\n", oldValue, value);

    // Compute new value
    u16 newValue;
    if (value & 0x8000) {
        newValue = (dmacon | value) & 0x07FF;
    } else {
        newValue = (dmacon & ~value) & 0x07FF;
    }

    if (oldValue == newValue) return;

    dmacon = newValue;

    // Update variable dmaconAtDDFStrt if DDFSTRT has not been reached yet
    if (pos.h + 2 < ddfstrtReached) dmaconAtDDFStrt = newValue;

    // Check the lowest 5 bits
    bool oldDMAEN = (oldValue & DMAEN);
    bool oldBPLEN = (oldValue & BPLEN) && oldDMAEN;
    bool oldCOPEN = (oldValue & COPEN) && oldDMAEN;
    bool oldBLTEN = (oldValue & BLTEN) && oldDMAEN;
    bool oldSPREN = (oldValue & SPREN) && oldDMAEN;
    bool oldDSKEN = (oldValue & DSKEN) && oldDMAEN;
    bool oldAUD0EN = (oldValue & AUD0EN) && oldDMAEN;
    bool oldAUD1EN = (oldValue & AUD1EN) && oldDMAEN;
    bool oldAUD2EN = (oldValue & AUD2EN) && oldDMAEN;
    bool oldAUD3EN = (oldValue & AUD3EN) && oldDMAEN;

    bool newDMAEN = (newValue & DMAEN);
    bool newBPLEN = (newValue & BPLEN) && newDMAEN;
    bool newCOPEN = (newValue & COPEN) && newDMAEN;
    bool newBLTEN = (newValue & BLTEN) && newDMAEN;
    bool newSPREN = (newValue & SPREN) && newDMAEN;
    bool newDSKEN = (newValue & DSKEN) && newDMAEN;
    bool newAUD0EN = (newValue & AUD0EN) && newDMAEN;
    bool newAUD1EN = (newValue & AUD1EN) && newDMAEN;
    bool newAUD2EN = (newValue & AUD2EN) && newDMAEN;
    bool newAUD3EN = (newValue & AUD3EN) && newDMAEN;

    bool toggleBPLEN = oldBPLEN ^ newBPLEN;
    bool toggleCOPEN = oldCOPEN ^ newCOPEN;
    bool toggleBLTEN = oldBLTEN ^ newBLTEN;
    bool toggleSPREN = oldSPREN ^ newSPREN;
    bool toggleDSKEN = oldDSKEN ^ newDSKEN;

    // Inform the delegates
    blitter.pokeDMACON(oldValue, newValue);

    // Bitplane DMA
    if (toggleBPLEN) {

        // Update the bpl event table in the next rasterline
        hsyncActions |= HSYNC_UPDATE_BPL_TABLE;

        if (newBPLEN) {

            // Bitplane DMA is switched on
            if (pos.h + 2 < ddfstrtReached || bpldma(dmaconAtDDFStrt)) {

                updateBplEvents(newValue, bplcon0, pos.h + 2);
                updateBplEvent();
            }

        } else {

            // Bitplane DMA is switched off
            updateBplEvents(newValue, bplcon0, pos.h + 2);
            updateBplEvent();
        }

        // Let Denise know about the change
        denise.pokeDMACON(oldValue, newValue);
    }

    // Check DAS DMA (Disk, Audio, Sprites)
    // u16 oldDAS = oldDMAEN ? (oldValue & 0x3F) : 0;
    // u16 newDAS = newDMAEN ? (newValue & 0x3F) : 0;
    // if (oldDAS != newDAS) {
    
    // Disk DMA and sprite DMA
    // We don't need to rebuild the table if audio DMA changes, because the
    // audio events are always executed.
    if (toggleDSKEN || toggleSPREN) {
        
        if (toggleSPREN)
            debug(DMA_DEBUG, "Sprite DMA %s\n", newSPREN ? "on" : "off");
        if (toggleDSKEN)
            debug(DMA_DEBUG, "Disk DMA %s\n", newDSKEN ? "on" : "off");
        
        u16 newDAS = newDMAEN ? (newValue & 0x3F) : 0;
        
        // Schedule the DAS DMA table to be rebuild
        hsyncActions |= HSYNC_UPDATE_DAS_TABLE;

        // Make the effect visible in the current rasterline as well
        for (int i = pos.h; i < HPOS_CNT; i++) {
            dasEvent[i] = dasDMA[newDAS][i];
        }
        updateDasJumpTable();

        // Rectify the currently scheduled DAS event
        scheduleDasEventForCycle(pos.h);
    }

    // Copper DMA
    if (toggleCOPEN) {
        debug(DMA_DEBUG, "Copper DMA %s\n", newCOPEN ? "on" : "off");
        if (newCOPEN) copper.activeInThisFrame = true;
    }

    // Blitter DMA
    if (toggleBLTEN) {
        debug(DMA_DEBUG, "Blitter DMA %s\n", newBLTEN ? "on" : "off");
    }

    // Audio DMA
    if (oldAUD0EN ^ newAUD0EN) {
        newAUD0EN ? audioUnit.channel0.enableDMA() : audioUnit.channel0.disableDMA();
    }
    if (oldAUD1EN ^ newAUD1EN) {
        newAUD1EN ? audioUnit.channel1.enableDMA() : audioUnit.channel1.disableDMA();
    }
    if (oldAUD2EN ^ newAUD2EN) {
        newAUD2EN ? audioUnit.channel2.enableDMA() : audioUnit.channel2.disableDMA();
    }
    if (oldAUD3EN ^ newAUD3EN) {
        newAUD3EN ? audioUnit.channel3.enableDMA() : audioUnit.channel3.disableDMA();
    }
}

void
Agnus::pokeDSKPTH(u16 value)
{
    debug(DSKREG_DEBUG, "pokeDSKPTH(%X)\n", value);
    dskpt = REPLACE_HI_WORD(dskpt, value);
}

void
Agnus::pokeDSKPTL(u16 value)
{
    debug(DSKREG_DEBUG, "pokeDSKPTL(%X)\n", value);
    dskpt = REPLACE_LO_WORD(dskpt, value & 0xFFFE);
}

template <int x> void
Agnus::pokeAUDxLCH(u16 value)
{
    debug(AUDREG_DEBUG, "pokeAUD%dLCH(%X)\n", x, value);

     audlc[x] = REPLACE_HI_WORD(audlc[x], value);
}

template <int x> void
Agnus::pokeAUDxLCL(u16 value)
{
    debug(AUDREG_DEBUG, "pokeAUD%dLCL(%X)\n", x, value);

    audlc[x] = REPLACE_LO_WORD(audlc[x], value & 0xFFFE);
}

bool
Agnus::skipBPLxPT(int x)
{
    /* If a new value is written into BPLxPTL or BPLxPTH, this usually happens
     * as described in the left scenario:
     *
     * 88888888888888889999999999999999      88888888888888889999999999999999
     * 0123456789ABCDEF0123456789ABCDEF      0123456789ABCDEF0123456789ABCDEF
     * .4.2.351.4.2.351.4.2.351.4.2.351      .4.2.351.4.2.351.4.2.351.4.2.351
     *     ^ ^                                     ^ ^
     *     | |                                     | |
     *     | Change takes effect here              | New value is lost
     *     Write to BPLxPT                         Write to BPL1PT
     *
     * The right scenario shows that the new value can get lost under certain
     * circumstances. The following must hold:
     *
     *     (1) There is a Lx or Hx event once cycle after the BPL1PT write.
     *     (2) There is no DMA going on when the write would happen.
     */

    if (isBplxEvent(bplEvent[pos.h + 1], x)) { // (1)

        if (bplEvent[pos.h + 2] == EVENT_NONE) { // (2)

            // debug("skipBPLxPT: Value gets lost\n");
            // dumpBplEventTable();
            return true;
        }
    }

    return false;
}

template <int x> void
Agnus::pokeBPLxPTH(u16 value)
{
    // debug(BPLREG_DEBUG, "pokeBPL%dPTH($%d) (%X)\n", x, value, value);

    // Check if the written value gets lost
    if (skipBPLxPT(x)) {
        // debug("BPLxPTH gets lost\n");
        return;
    }

    // Schedule the register updated
    switch (x) {
        case 1: recordRegisterChange(DMA_CYCLES(2), REG_BPL1PTH, value); break;
        case 2: recordRegisterChange(DMA_CYCLES(2), REG_BPL2PTH, value); break;
        case 3: recordRegisterChange(DMA_CYCLES(2), REG_BPL3PTH, value); break;
        case 4: recordRegisterChange(DMA_CYCLES(2), REG_BPL4PTH, value); break;
        case 5: recordRegisterChange(DMA_CYCLES(2), REG_BPL5PTH, value); break;
        case 6: recordRegisterChange(DMA_CYCLES(2), REG_BPL6PTH, value); break;
    }
}

template <int x> void
Agnus::pokeBPLxPTL(u16 value)
{
    // debug(BPLREG_DEBUG, "pokeBPL%dPTL(%d) ($%X)\n", x, value, value);

    // Check if the written value gets lost
    if (skipBPLxPT(x)) {
        debug(BPLREG_DEBUG, "BPLxPTL gets lost\n");
        return;
    }

    // Schedule the register updated
    switch (x) {
        case 1: recordRegisterChange(DMA_CYCLES(2), REG_BPL1PTL, value); break;
        case 2: recordRegisterChange(DMA_CYCLES(2), REG_BPL2PTL, value); break;
        case 3: recordRegisterChange(DMA_CYCLES(2), REG_BPL3PTL, value); break;
        case 4: recordRegisterChange(DMA_CYCLES(2), REG_BPL4PTL, value); break;
        case 5: recordRegisterChange(DMA_CYCLES(2), REG_BPL5PTL, value); break;
        case 6: recordRegisterChange(DMA_CYCLES(2), REG_BPL6PTL, value); break;
    }
}

template <int x> void
Agnus::setBPLxPTH(u16 value)
{
    debug(BPLREG_DEBUG, "setBPLxPTH(%d, %X)\n", x, value);
    bplpt[x - 1] = REPLACE_HI_WORD(bplpt[x - 1], value);
}

template <int x> void
Agnus::setBPLxPTL(u16 value)
{
    debug(BPLREG_DEBUG, "setBPLxPTL(%d, %X)\n", x, value);
    bplpt[x - 1] = REPLACE_LO_WORD(bplpt[x - 1], value & 0xFFFE);
}

void
Agnus::pokeBPL1MOD(u16 value)
{
    debug(BPLREG_DEBUG, "pokeBPL1MOD(%X)\n", value);
    recordRegisterChange(DMA_CYCLES(2), REG_BPL1MOD, value);
}

void
Agnus::setBPL1MOD(u16 value)
{
    debug(BPLREG_DEBUG, "setBPL1MOD(%X)\n", value);
    bpl1mod = (i16)(value & 0xFFFE);
}

void
Agnus::pokeBPL2MOD(u16 value)
{
    debug(BPLREG_DEBUG, "pokeBPL2MOD(%X)\n", value);
    recordRegisterChange(DMA_CYCLES(2), REG_BPL2MOD, value);
}

void
Agnus::setBPL2MOD(u16 value)
{
    debug(BPLREG_DEBUG, "setBPL2MOD(%X)\n", value);
    bpl2mod = (i16)(value & 0xFFFE);
}

template <int x> void
Agnus::pokeSPRxPTH(u16 value)
{
    debug(SPRREG_DEBUG, "pokeSPR%dPTH(%X)\n", x, value);
    sprpt[x] = REPLACE_HI_WORD(sprpt[x], value);
}

template <int x> void
Agnus::pokeSPRxPTL(u16 value)
{
    debug(SPRREG_DEBUG, "pokeSPR%dPTL(%X)\n", x, value);
    sprpt[x] = REPLACE_LO_WORD(sprpt[x], value & 0xFFFE);
}

template <int x> void
Agnus::pokeSPRxPOS(u16 value)
{
    debug(SPRREG_DEBUG, "pokeSPR%dPOS(%X)\n", x, value);

    // Compute the value of the vertical counter that is seen here
    i16 v = (pos.h < 0xDF) ? pos.v : (pos.v + 1);

    // Compute the new vertical start position
    sprVStrt[x] = ((value & 0xFF00) >> 8) | (sprVStrt[x] & 0x0100);

    // Update sprite DMA status
    if (sprVStrt[x] == v) sprDmaState[x] = SPR_DMA_ACTIVE;
    if (sprVStop[x] == v) sprDmaState[x] = SPR_DMA_IDLE;
}

template <int x> void
Agnus::pokeSPRxCTL(u16 value)
{
    debug(SPRREG_DEBUG, "pokeSPR%dCTL(%X)\n", x, value);

    // Compute the value of the vertical counter that is seen here
    i16 v = (pos.h < 0xDF) ? pos.v : (pos.v + 1);

    // Compute the new vertical start and stop position
    sprVStrt[x] = ((value & 0b100) << 6) | (sprVStrt[x] & 0x00FF);
    sprVStop[x] = ((value & 0b010) << 7) | (value >> 8);

    // Update sprite DMA status
    if (sprVStrt[x] == v) sprDmaState[x] = SPR_DMA_ACTIVE;
    if (sprVStop[x] == v) sprDmaState[x] = SPR_DMA_IDLE;
}

u16
Agnus::peek(u32 addr)
{
    addr &= ptrMask;
    
    if (addr >= 0x80000 && slowRamIsMirroredIn()) {
        return mem.peek16 <ACC_AGNUS, MEM_SLOW> (addr);
    }
    
    return mem.peek16 <ACC_AGNUS, MEM_CHIP> (addr);
}

void
Agnus::poke(u32 addr, u16 value)
{
    addr &= ptrMask;

    // Check if SlowRam is mirrored in
    if (addr >= 0x80000 && slowRamIsMirroredIn()) {
        mem.pokeSlow16(addr, value);
        return;
    }
    
    mem.pokeChip16(addr, value);
}

template <BusOwner owner> bool
Agnus::busIsFree()
{
    // Deny if the bus is already in use
    if (busOwner[pos.h] != BUS_NONE) return false;

    switch (owner) {

        case BUS_COPPER:
        {
            // Deny if Copper DMA is disabled
            if (!copdma()) return false;

            // Deny in cycle E0
            if (unlikely(pos.h == 0xE0)) return false;
            return true;
        }
        case BUS_BLITTER:
        {
            // Deny if Blitter DMA is disabled
            if (!bltdma()) return false;
            
            // Deny if the CPU has precedence
            if (bls && !bltpri()) return false;

            return true;
        }
    }

    assert(false);
    return false;
}

template <BusOwner owner> bool
Agnus::allocateBus()
{
    // Deny if the bus has been allocated already
    if (busOwner[pos.h] != BUS_NONE) return false;

    switch (owner) {

        case BUS_COPPER:
        {
            // Assign bus to the Copper
            busOwner[pos.h] = BUS_COPPER;
            return true;
        }
        case BUS_BLITTER:
        {
            // Deny if Blitter DMA is off
            if (!bltdma()) return false;

            // Deny if the CPU has precedence
            if (bls && !bltpri()) return false;

            // Assign the bus to the Blitter
            busOwner[pos.h] = BUS_BLITTER;
            return true;
        }
    }

    assert(false);
    return false;
}

u16
Agnus::doDiskDMA()
{
    u16 result = peek(dskpt);
    dskpt += 2;

    assert(pos.h < HPOS_CNT);
    busOwner[pos.h] = BUS_DISK;
    busValue[pos.h] = result;
    stats.bus.raw[BUS_DISK]++;

    return result;
}

template <int channel> u16
Agnus::doAudioDMA()
{
    u16 result = peek(audpt[channel]);
    audpt[channel] += 2;

    assert(pos.h < HPOS_CNT);
    busOwner[pos.h] = BUS_AUDIO;
    busValue[pos.h] = result;
    stats.bus.raw[BUS_AUDIO]++;

    return result;
}

template <int bitplane> u16
Agnus::doBitplaneDMA()
{
    u16 result = peek(bplpt[bitplane]);
    bplpt[bitplane] += 2;

    assert(pos.h < HPOS_CNT);
    busOwner[pos.h] = BUS_BITPLANE;
    busValue[pos.h] = result;
    stats.bus.raw[BUS_BITPLANE]++;

    return result;
}

template <int channel> u16
Agnus::doSpriteDMA()
{
    u16 result = peek(sprpt[channel]);
    sprpt[channel] += 2;

    assert(pos.h < HPOS_CNT);
    busOwner[pos.h] = BUS_SPRITE;
    busValue[pos.h] = result;
    stats.bus.raw[BUS_SPRITE]++;

    return result;
}

u16
Agnus::doCopperDMA(u32 addr)
{
    u16 result = peek(addr);

    assert(pos.h < HPOS_CNT);
    busOwner[pos.h] = BUS_COPPER;
    busValue[pos.h] = result;
    stats.bus.raw[BUS_COPPER]++;

    return result;
}

u16
Agnus::doBlitterDMA(u32 addr)
{
    // Assure that the Blitter owns the bus when this function is called
    assert(busOwner[pos.h] == BUS_BLITTER);

    u16 result = peek(addr);

    assert(pos.h < HPOS_CNT);
    busOwner[pos.h] = BUS_BLITTER;
    busValue[pos.h] = result;
    stats.bus.raw[BUS_BLITTER]++;

    return result;
}

void
Agnus::doDiskDMA(u16 value)
{
    poke(dskpt, value);
    dskpt += 2;

    assert(pos.h < HPOS_CNT);
    busOwner[pos.h] = BUS_DISK;
    busValue[pos.h] = value;
    stats.bus.raw[BUS_DISK]++;
}

void
Agnus::doCopperDMA(u32 addr, u16 value)
{
    mem.pokeCustom16<ACC_AGNUS>(addr, value);
    
    assert(pos.h < HPOS_CNT);
    busOwner[pos.h] = BUS_COPPER;
    busValue[pos.h] = value;
    stats.bus.raw[BUS_COPPER]++;
}

void
Agnus::doBlitterDMA(u32 addr, u16 value)
{
    poke(addr, value);
    
    assert(pos.h < HPOS_CNT);
    assert(busOwner[pos.h] == BUS_BLITTER); // Bus is already allocated
    busValue[pos.h] = value;
    stats.bus.raw[BUS_BLITTER]++;
}

void
Agnus::clearBplEvents()
{
    for (int i = 0; i < HPOS_MAX; i++) bplEvent[i] = EVENT_NONE;
    for (int i = 0; i < HPOS_MAX; i++) nextBplEvent[i] = HPOS_MAX;

    verifyBplEvents();
}

void
Agnus::updateBplEvents(u16 dmacon, u16 bplcon0, int first, int last)
{
    assert(first >= 0 && last < HPOS_CNT);

    int channels = bpu(bplcon0);
    bool hires = Denise::hires(bplcon0);

    // Set number of bitplanes to 0 if we are not in a bitplane DMA line
    if (!inBplDmaLine(dmacon, bplcon0)) channels = 0;
    assert(channels <= 6);

    // Allocate slots
    if (hires) {
        
        for (int i = first; i <= last; i++)
            bplEvent[i] =
            inHiresDmaAreaOdd(i) ? bplDMA[1][channels][i] :
            inHiresDmaAreaEven(i) ? bplDMA[1][channels][i] : EVENT_NONE;
        
        // Add extra shift register events if the even/odd DDF windows differ
        // These events are like BPL_H0 events without performing DMA.
        for (int i = ddfHires.strtEven; i < ddfHires.strtOdd; i++)
            if ((i & 3) == 3 && bplEvent[i] == EVENT_NONE) bplEvent[i] = BPL_SR;
        for (int i = ddfHires.stopOdd; i < ddfHires.stopEven; i++)
            if ((i & 3) == 3 && bplEvent[i] == EVENT_NONE) bplEvent[i] = BPL_SR;

    } else {
        
        for (int i = first; i <= last; i++)
            bplEvent[i] =
            inLoresDmaAreaOdd(i) ? bplDMA[0][channels][i] :
            inLoresDmaAreaEven(i) ? bplDMA[0][channels][i] : EVENT_NONE;
    
        // Add extra shift register events if the even/odd DDF windows differ
        // These events are like BPL_L0 events without performing DMA.
        for (int i = ddfLores.strtEven; i < ddfLores.strtOdd; i++)
             if ((i & 7) == 7 && bplEvent[i] == EVENT_NONE) bplEvent[i] = BPL_SR;
        for (int i = ddfLores.stopOdd; i < ddfLores.stopEven; i++)
             if ((i & 7) == 7 && bplEvent[i] == EVENT_NONE) bplEvent[i] = BPL_SR;
    }
        
    // Make sure the table ends with a BPL_EOL event
    bplEvent[HPOS_MAX] = BPL_EOL;

    // Update the drawing flags and update the jump table
    updateDrawingFlags(hires);
    
    verifyBplEvents();
}

void
Agnus::updateDrawingFlags(bool hires)
{
    assert(scrollHiresEven < 8);
    assert(scrollHiresOdd  < 8);
    assert(scrollLoresEven < 8);
    assert(scrollHiresOdd  < 8);
    
    // Superimpose the drawing flags (bits 0 and 1)
    // Bit 0 is used to for odd bitplanes and bit 1 for even bitplanes
    if (hires) {
        for (int i = scrollHiresOdd; i < HPOS_CNT; i += 4)
            bplEvent[i] = (EventID)(bplEvent[i] | 1);
        for (int i = scrollHiresEven; i < HPOS_CNT; i += 4)
            bplEvent[i] = (EventID)(bplEvent[i] | 2);
    } else {
        for (int i = scrollLoresOdd; i < HPOS_CNT; i += 8)
            bplEvent[i] = (EventID)(bplEvent[i] | 1);
        for (int i = scrollLoresEven; i < HPOS_CNT; i += 8)
            bplEvent[i] = (EventID)(bplEvent[i] | 2);
    }
    updateBplJumpTable();
}

void
Agnus::verifyBplEvents()
{
    assert((bplEvent[HPOS_MAX] & 0b11111100) == BPL_EOL);
    assert(nextBplEvent[HPOS_MAX] == 0);
}

void
Agnus::clearDasEvents()
{
    updateDasEvents(0);
}

void
Agnus::updateDasEvents(u16 dmacon)
{
    assert(dmacon < 64);

    // Allocate slots and renew the jump table
    for (int i = 0; i < 0x38; i++) dasEvent[i] = dasDMA[dmacon][i];
    updateDasJumpTable(0x38);

    verifyDasEvents();
}

void
Agnus::verifyDasEvents()
{
    assert(dasEvent[0x01] == DAS_REFRESH);
    assert(dasEvent[0xDF] == DAS_SDMA);

    for (int i = 0x34; i < 0xDF; i++) {
        assert(dasEvent[i] == EVENT_NONE);
        assert(nextDasEvent[i] == 0xDF);
    }
    for (int i = 0xE0; i < HPOS_CNT; i++) {
        assert(dasEvent[i] == EVENT_NONE);
        assert(nextDasEvent[i] == 0);
    }
}

void
Agnus::updateBplJumpTable(i16 end)
{
    assert(end <= HPOS_MAX);

    u8 next = nextBplEvent[end];
    for (int i = end; i >= 0; i--) {
        nextBplEvent[i] = next;
        if (bplEvent[i]) next = i;
    }
}

void
Agnus::updateDasJumpTable(i16 end)
{
    assert(end <= HPOS_MAX);

    u8 next = nextDasEvent[end];
    for (int i = end; i >= 0; i--) {
        nextDasEvent[i] = next;
        if (dasEvent[i]) next = i;
    }
}

void
Agnus::dumpEventTable(EventID *table, char str[256][3], int from, int to)
{
    char r1[256], r2[256], r3[256], r4[256], r5[256];
    int i;

    for (i = 0; i <= to - from; i++) {

        int digit1 = (from + i) / 16;
        int digit2 = (from + i) % 16;

        r1[i] = (digit1 < 10) ? digit1 + '0' : (digit1 - 10) + 'A';
        r2[i] = (digit2 < 10) ? digit2 + '0' : (digit2 - 10) + 'A';
        r3[i] = str[table[from + i]][0];
        r4[i] = str[table[from + i]][1];
        r5[i] = str[table[from + i]][2];
    }
    r1[i] = r2[i] = r3[i] = r4[i] = r5[i] = 0;

    msg("%s\n", r1);
    msg("%s\n", r2);
    msg("%s\n", r3);
    msg("%s\n", r4);
    msg("%s\n", r5);
}

void
Agnus::dumpBplEventTable(int from, int to)
{
    char str[256][3];

    memset(str, '?', sizeof(str));
    
    // Events
    for (int i = 0; i < 4; i++) {
        str[i][0] = '.';                str[i][1] = '.';
        str[(int)BPL_L1 + i][0]  = 'L'; str[(int)BPL_L1 + i][1]  = '1';
        str[(int)BPL_L2 + i][0]  = 'L'; str[(int)BPL_L2 + i][1]  = '2';
        str[(int)BPL_L3 + i][0]  = 'L'; str[(int)BPL_L3 + i][1]  = '3';
        str[(int)BPL_L4 + i][0]  = 'L'; str[(int)BPL_L4 + i][1]  = '4';
        str[(int)BPL_L5 + i][0]  = 'L'; str[(int)BPL_L5 + i][1]  = '5';
        str[(int)BPL_L6 + i][0]  = 'L'; str[(int)BPL_L6 + i][1]  = '6';
        str[(int)BPL_H1 + i][0]  = 'H'; str[(int)BPL_H1 + i][1]  = '1';
        str[(int)BPL_H2 + i][0]  = 'H'; str[(int)BPL_H2 + i][1]  = '2';
        str[(int)BPL_H3 + i][0]  = 'H'; str[(int)BPL_H3 + i][1]  = '3';
        str[(int)BPL_H4 + i][0]  = 'H'; str[(int)BPL_H4 + i][1]  = '4';
        str[(int)BPL_EOL + i][0] = 'E'; str[(int)BPL_EOL + i][1] = 'O';
    }

    // Drawing flags
    for (int i = 1; i < 256; i += 4) str[i][2] = 'o';
    for (int i = 2; i < 256; i += 4) str[i][2] = 'e';
    for (int i = 3; i < 256; i += 4) str[i][2] = 'b';

    dumpEventTable(bplEvent, str, from, to);
}

void
Agnus::dumpBplEventTable()
{
    // Dump the event table
    msg("Event table:\n\n");
    msg("ddfstrt = %X dffstop = %X\n", ddfstrt, ddfstop);
    msg("ddfLoresOdd:  (%X - %X)\n", ddfLores.strtOdd, ddfLores.stopOdd);
    msg("ddfLoresEven: (%X - %X)\n", ddfLores.strtEven, ddfLores.stopEven);
    msg("ddfHiresOdd:  (%X - %X)\n", ddfHires.strtOdd, ddfHires.stopOdd);
    msg("ddfHiresEven: (%X - %X)\n", ddfHires.strtEven, ddfHires.stopEven);

    dumpBplEventTable(0x00, 0x4F);
    dumpBplEventTable(0x50, 0x9F);
    dumpBplEventTable(0xA0, 0xE2);

    // Dump the jump table
    msg("\nJump table:\n\n");
    int i = nextBplEvent[0];
    msg("0 -> %X", i);
    while (i) {
        assert(i < HPOS_CNT);
        assert(nextBplEvent[i] == 0 || nextBplEvent[i] > i);
        i = nextBplEvent[i];
        msg(" -> %X", i);
    }
    msg("\n");
}

void
Agnus::dumpDasEventTable(int from, int to)
{
    char str[256][3];

    memset(str, '?', sizeof(str));
    str[(int)EVENT_NONE][0]  = '.'; str[(int)EVENT_NONE][1]  = '.';
    str[(int)DAS_REFRESH][0] = 'R'; str[(int)DAS_REFRESH][1] = 'E';
    str[(int)DAS_D0][0]      = 'D'; str[(int)DAS_D0][1]      = '0';
    str[(int)DAS_D1][0]      = 'D'; str[(int)DAS_D1][1]      = '1';
    str[(int)DAS_D2][0]      = 'D'; str[(int)DAS_D2][1]      = '2';
    str[(int)DAS_A0][0]      = 'A'; str[(int)DAS_A0][1]      = '0';
    str[(int)DAS_A1][0]      = 'A'; str[(int)DAS_A1][1]      = '1';
    str[(int)DAS_A2][0]      = 'A'; str[(int)DAS_A2][1]      = '2';
    str[(int)DAS_A3][0]      = 'A'; str[(int)DAS_A3][1]      = '3';
    str[(int)DAS_S0_1][0]    = '0'; str[(int)DAS_S0_1][1]    = '1';
    str[(int)DAS_S0_2][0]    = '0'; str[(int)DAS_S0_2][1]    = '2';
    str[(int)DAS_S1_1][0]    = '1'; str[(int)DAS_S1_1][1]    = '1';
    str[(int)DAS_S1_2][0]    = '1'; str[(int)DAS_S1_2][1]    = '2';
    str[(int)DAS_S2_1][0]    = '2'; str[(int)DAS_S2_1][1]    = '1';
    str[(int)DAS_S2_2][0]    = '2'; str[(int)DAS_S2_2][1]    = '2';
    str[(int)DAS_S3_1][0]    = '3'; str[(int)DAS_S3_1][1]    = '1';
    str[(int)DAS_S3_2][0]    = '3'; str[(int)DAS_S3_2][1]    = '2';
    str[(int)DAS_S4_1][0]    = '4'; str[(int)DAS_S4_1][1]    = '1';
    str[(int)DAS_S4_2][0]    = '4'; str[(int)DAS_S4_2][1]    = '2';
    str[(int)DAS_S5_1][0]    = '5'; str[(int)DAS_S5_1][1]    = '1';
    str[(int)DAS_S5_2][0]    = '5'; str[(int)DAS_S5_2][1]    = '2';
    str[(int)DAS_S6_1][0]    = '6'; str[(int)DAS_S6_1][1]    = '1';
    str[(int)DAS_S6_2][0]    = '6'; str[(int)DAS_S6_2][1]    = '2';
    str[(int)DAS_S7_1][0]    = '7'; str[(int)DAS_S7_1][1]    = '1';
    str[(int)DAS_S7_2][0]    = '7'; str[(int)DAS_S7_2][1]    = '2';
    str[(int)DAS_SDMA][0]    = 'S'; str[(int)DAS_SDMA][1]    = 'D';

    for (int i = 1; i < 256; i++) str[i][2] = ' ';
    
    dumpEventTable(dasEvent, str, from, to);
}

void
Agnus::dumpDasEventTable()
{
    // Dump the event table
    dumpDasEventTable(0x00, 0x4F);
    dumpDasEventTable(0x50, 0x9F);
    dumpDasEventTable(0xA0, 0xE2);
}

template void Agnus::pokeAUDxLCH<0>(u16 value);
template void Agnus::pokeAUDxLCH<1>(u16 value);
template void Agnus::pokeAUDxLCH<2>(u16 value);
template void Agnus::pokeAUDxLCH<3>(u16 value);

template void Agnus::pokeAUDxLCL<0>(u16 value);
template void Agnus::pokeAUDxLCL<1>(u16 value);
template void Agnus::pokeAUDxLCL<2>(u16 value);
template void Agnus::pokeAUDxLCL<3>(u16 value);

template void Agnus::pokeBPLxPTH<1>(u16 value);
template void Agnus::pokeBPLxPTH<2>(u16 value);
template void Agnus::pokeBPLxPTH<3>(u16 value);
template void Agnus::pokeBPLxPTH<4>(u16 value);
template void Agnus::pokeBPLxPTH<5>(u16 value);
template void Agnus::pokeBPLxPTH<6>(u16 value);
template void Agnus::setBPLxPTH<1>(u16 value);
template void Agnus::setBPLxPTH<2>(u16 value);
template void Agnus::setBPLxPTH<3>(u16 value);
template void Agnus::setBPLxPTH<4>(u16 value);
template void Agnus::setBPLxPTH<5>(u16 value);
template void Agnus::setBPLxPTH<6>(u16 value);

template void Agnus::pokeBPLxPTL<1>(u16 value);
template void Agnus::pokeBPLxPTL<2>(u16 value);
template void Agnus::pokeBPLxPTL<3>(u16 value);
template void Agnus::pokeBPLxPTL<4>(u16 value);
template void Agnus::pokeBPLxPTL<5>(u16 value);
template void Agnus::pokeBPLxPTL<6>(u16 value);
template void Agnus::setBPLxPTL<1>(u16 value);
template void Agnus::setBPLxPTL<2>(u16 value);
template void Agnus::setBPLxPTL<3>(u16 value);
template void Agnus::setBPLxPTL<4>(u16 value);
template void Agnus::setBPLxPTL<5>(u16 value);
template void Agnus::setBPLxPTL<6>(u16 value);

template void Agnus::pokeSPRxPTH<0>(u16 value);
template void Agnus::pokeSPRxPTH<1>(u16 value);
template void Agnus::pokeSPRxPTH<2>(u16 value);
template void Agnus::pokeSPRxPTH<3>(u16 value);
template void Agnus::pokeSPRxPTH<4>(u16 value);
template void Agnus::pokeSPRxPTH<5>(u16 value);
template void Agnus::pokeSPRxPTH<6>(u16 value);
template void Agnus::pokeSPRxPTH<7>(u16 value);

template void Agnus::pokeSPRxPTL<0>(u16 value);
template void Agnus::pokeSPRxPTL<1>(u16 value);
template void Agnus::pokeSPRxPTL<2>(u16 value);
template void Agnus::pokeSPRxPTL<3>(u16 value);
template void Agnus::pokeSPRxPTL<4>(u16 value);
template void Agnus::pokeSPRxPTL<5>(u16 value);
template void Agnus::pokeSPRxPTL<6>(u16 value);
template void Agnus::pokeSPRxPTL<7>(u16 value);

template void Agnus::pokeSPRxPOS<0>(u16 value);
template void Agnus::pokeSPRxPOS<1>(u16 value);
template void Agnus::pokeSPRxPOS<2>(u16 value);
template void Agnus::pokeSPRxPOS<3>(u16 value);
template void Agnus::pokeSPRxPOS<4>(u16 value);
template void Agnus::pokeSPRxPOS<5>(u16 value);
template void Agnus::pokeSPRxPOS<6>(u16 value);
template void Agnus::pokeSPRxPOS<7>(u16 value);

template void Agnus::pokeSPRxCTL<0>(u16 value);
template void Agnus::pokeSPRxCTL<1>(u16 value);
template void Agnus::pokeSPRxCTL<2>(u16 value);
template void Agnus::pokeSPRxCTL<3>(u16 value);
template void Agnus::pokeSPRxCTL<4>(u16 value);
template void Agnus::pokeSPRxCTL<5>(u16 value);
template void Agnus::pokeSPRxCTL<6>(u16 value);
template void Agnus::pokeSPRxCTL<7>(u16 value);

template u16 Agnus::doAudioDMA<0>();
template u16 Agnus::doAudioDMA<1>();
template u16 Agnus::doAudioDMA<2>();
template u16 Agnus::doAudioDMA<3>();

template u16 Agnus::doBitplaneDMA<0>();
template u16 Agnus::doBitplaneDMA<1>();
template u16 Agnus::doBitplaneDMA<2>();
template u16 Agnus::doBitplaneDMA<3>();
template u16 Agnus::doBitplaneDMA<4>();
template u16 Agnus::doBitplaneDMA<5>();

template u16 Agnus::doSpriteDMA<0>();
template u16 Agnus::doSpriteDMA<1>();
template u16 Agnus::doSpriteDMA<2>();
template u16 Agnus::doSpriteDMA<3>();
template u16 Agnus::doSpriteDMA<4>();
template u16 Agnus::doSpriteDMA<5>();
template u16 Agnus::doSpriteDMA<6>();
template u16 Agnus::doSpriteDMA<7>();

template bool Agnus::allocateBus<BUS_COPPER>();
template bool Agnus::allocateBus<BUS_BLITTER>();

template bool Agnus::busIsFree<BUS_COPPER>();
template bool Agnus::busIsFree<BUS_BLITTER>();
