// pevent --- portable event objects
// Copyright (C) 2015-2024 Katayama Hirofumi MZ.
// This file is released under the terms of the Modified BSD License.

#ifdef _WIN32

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif

#include "pevent.h"

/*--------------------------------------------------------------------------*/
/* These definitions are optimized for Win32 */

#include <cassert>

extern "C"
{
    pe_event_t pe_create_event(bool manual_reset, bool initial_state)
    {
        pe_event_t ret;
        ret = ::CreateEvent(NULL, manual_reset, initial_state, NULL);
        assert(ret);
        return ret;
    }
    bool pe_wait_for_event(pe_event_t handle, uint32_t milliseconds)
    {
        assert(handle);
        return (::WaitForSingleObject(handle, milliseconds) == WAIT_TIMEOUT);
    }
    bool pe_close_event(pe_event_t handle)
    {
        assert(handle);
        return ::CloseHandle(handle);
    }
    bool pe_set_event(pe_event_t handle)
    {
        assert(handle);
        return ::SetEvent(handle);
    }
    bool pe_reset_event(pe_event_t handle)
    {
        assert(handle);
        return ::ResetEvent(handle);
    }
    bool pe_pulse_event(pe_event_t handle)
    {
        assert(handle);
        return ::PulseEvent(handle);
    }
} // extern "C"

/*--------------------------------------------------------------------------*/

#endif  // def _WIN32
