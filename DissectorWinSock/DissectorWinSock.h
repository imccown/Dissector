#pragma once

namespace DissectorWinsock
{
    // Setup
    void Initialize( unsigned int iListenPort = 2534 ); // This will register itself with the dissector library.
    void Shutdown();

    void FrameUpdate();
};
