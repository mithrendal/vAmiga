// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

enum IrqSource : long
{
    INT_TBE,
    INT_DSKBLK,
    INT_SOFT,
    INT_PORTS,
    INT_COPER,
    INT_VERTB,
    INT_BLIT,
    INT_AUD0,
    INT_AUD1,
    INT_AUD2,
    INT_AUD3,
    INT_RBF,
    INT_DSKSYN,
    INT_EXTER,
    INT_COUNT
};

static inline bool isIrqSource(long value)
{
    return value >= 0 && value < INT_COUNT;
}
