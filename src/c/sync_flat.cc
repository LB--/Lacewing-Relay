
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

lw_sync * lw_sync_new ()
    { return (lw_sync *) new Lacewing::Sync();
    }
void lw_sync_delete (lw_sync * sync)
    { delete (Lacewing::Sync *) sync;
    }
lw_sync_lock * lw_sync_lock_new (lw_sync * sync)
    { return (lw_sync_lock *) new Lacewing::Sync::Lock(*(Lacewing::Sync *) sync);
    }
void lw_sync_lock_delete (lw_sync_lock * lock)
    { delete (Lacewing::Sync::Lock *) lock;
    }
void lw_sync_lock_release (lw_sync_lock * lock)
    { ((Lacewing::Sync::Lock *) lock)->Release();
    }





