#pragma once

template< typename iType >
void StoreBufferData( const iType& iData, char*& oData )
{
    *(iType*)oData = iData;
    oData += sizeof(iType);
}

template< typename iType >
iType GetBufferData( char*& ioData )
{
    iType* rvalue = (iType*)ioData;
    ioData += sizeof(iType);

    return *rvalue;
}

