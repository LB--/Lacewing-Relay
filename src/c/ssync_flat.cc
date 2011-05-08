
/*
    Copyright (C) 2011 James McLaughlin

    This file is part of Lacewing.

    Lacewing is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lacewing is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Lacewing.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "../Common.h"

lw_ssync * lw_ssync_new ()
    { return (lw_ssync *) new Lacewing::SpinSync();
    }
void lw_ssync_delete (lw_ssync * sync)
    { delete (Lacewing::SpinSync *) sync;
    }
lw_ssync_wlock * lw_ssync_wlock_new (lw_ssync * sync)
    { return (lw_ssync_wlock *) new Lacewing::SpinSync::WriteLock(*(Lacewing::SpinSync *) sync);
    }
void lw_ssync_wlock_delete (lw_ssync_wlock * lock)
    { delete (Lacewing::SpinSync::WriteLock *) lock;
    }
void lw_ssync_wlock_release (lw_ssync_wlock * lock)
    { ((Lacewing::SpinSync::WriteLock *) lock)->Release();
    }
lw_ssync_rlock * lw_ssync_rlock_new (lw_ssync * sync)
    { return (lw_ssync_rlock *) new Lacewing::SpinSync::ReadLock(*(Lacewing::SpinSync *) sync);
    }
void lw_ssync_rlock_delete (lw_ssync_rlock * lock)
    { delete (Lacewing::SpinSync::ReadLock *) lock;
    }
void lw_ssync_rlock_release (lw_ssync_rlock * lock)
    { ((Lacewing::SpinSync::ReadLock *) lock)->Release();
    }




