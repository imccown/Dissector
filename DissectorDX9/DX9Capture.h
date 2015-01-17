#pragma once
#include "DX9Internal.h"

namespace DissectorDX9
{
    void SetRenderStates( IDirect3DDevice9* iD3DDevice, const Dissector::DrawCallData* iDC );
    void GetRenderStates( void* iDevice, int iEventType, const void* iEventData, unsigned int iDataSize );
};