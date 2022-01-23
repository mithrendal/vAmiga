// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "CIA.h"
#include "Agnus.h"

void
CIA::serviceEvent(EventID id)
{
    switch(id) {

        case CIA_EXECUTE:
            
            executeOneCycle();
            break;

        case CIA_WAKEUP:
            
            wakeUp();
            break;

        default:
            fatalError;
    }
}

void
CIA::scheduleNextExecution()
{
    if (isCIAA()) {
        scheduler.scheduleAbs<SLOT_CIAA>(clock + CIA_CYCLES(1), CIA_EXECUTE);
    } else {
        scheduler.scheduleAbs<SLOT_CIAB>(clock + CIA_CYCLES(1), CIA_EXECUTE);
    }
}

void
CIA::scheduleWakeUp()
{
    if (isCIAA()) {
        scheduler.scheduleAbs<SLOT_CIAA>(wakeUpCycle, CIA_WAKEUP);
    } else {
        scheduler.scheduleAbs<SLOT_CIAB>(wakeUpCycle, CIA_WAKEUP);
    }
}
