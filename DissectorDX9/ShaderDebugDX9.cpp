#include "ShaderDebugDX9.h"
#include <assert.h>
#include "..\Dissector\DissectorHelpers.h"

namespace ShaderDebugDX9
{
    // ------------------------------------------------------------------------
    // Internal functions
    // ------------------------------------------------------------------------
    static bool ReadVersionToken( char*& ioIter, ShaderInfo* oToken )
    {
        DWORD code = GetBufferData<DWORD>( ioIter );
        if( oToken )
        {
            oToken->mShaderType = (code & 0xFFFF0000) >> 16;
            if( oToken->mShaderType != VertexShader && oToken->mShaderType != PixelShader )
            {
                return false;
            }
            oToken->mShaderVersion = code & 0xFFFF;
        }

        return true;
    }

    static void WriteFloatConstantInstruction( char*& oIter, int iRegister, float i0, float i1, float i2, float i3 )
    {
        StoreBufferData<DWORD>( D3DSIO_DEF | (5 << D3DSI_INSTLENGTH_SHIFT), oIter );
        StoreBufferData<DWORD>( 0x80000000 | D3DSP_WRITEMASK_ALL | (D3DSPR_CONST << D3DSP_REGTYPE_SHIFT) | iRegister, oIter );
        StoreBufferData<float>( i0, oIter );
        StoreBufferData<float>( i1, oIter );
        StoreBufferData<float>( i2, oIter );
        StoreBufferData<float>( i3, oIter );
    }

    static void WriteIntConstantInstruction( char*& oIter, int iRegister, int i0, int i1, int i2, int i3 )
    {
        StoreBufferData<DWORD>( D3DSIO_DEFI | (5 << D3DSI_INSTLENGTH_SHIFT), oIter );
        StoreBufferData<DWORD>( 0x80000000 | D3DSP_WRITEMASK_ALL | (D3DSPR_CONSTINT << D3DSP_REGTYPE_SHIFT) | iRegister, oIter );
        StoreBufferData<int>( i0, oIter );
        StoreBufferData<int>( i1, oIter );
        StoreBufferData<int>( i2, oIter );
        StoreBufferData<int>( i3, oIter );
    }

    static DWORD GetDstRegType( DWORD iToken )
    {
        return ((iToken&D3DSP_REGTYPE_MASK) >> (D3DSP_REGTYPE_SHIFT)) |
               ((iToken&D3DSP_REGTYPE_MASK2) >> (D3DSP_REGTYPE_SHIFT2));
    }

    static DWORD GetSrcRegType( DWORD iToken )
    {
        return GetDstRegType( iToken );
    }

    static DWORD MakeDstRegType( DWORD iType )
    {
        return  ((iType << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK) |
                ((iType << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2);
    }

    static DWORD MakeDstReg( DWORD iType, DWORD iNum, DWORD iWriteMask )
    {
        return DWORD( 0x80000000 | (iNum & D3DSP_REGNUM_MASK) | MakeDstRegType( iType ) | iWriteMask );
    }

    static DWORD MakeSrcReg( DWORD iType, DWORD iNum, DWORD iSwizzleMask, DWORD iSrcMod )
    {
        return DWORD( 0x80000000 | (iNum & D3DSP_REGNUM_MASK) | MakeDstRegType( iType ) | iSwizzleMask | iSrcMod );
    }

    static DWORD GetDstRegNum( DWORD iToken )
    {
        return (iToken & D3DSP_REGNUM_MASK);
    }

    static DWORD GetSrcRegNum( DWORD iToken )
    {
        return (iToken & D3DSP_REGNUM_MASK);
    }

    static void WriteDCLOutputInstruction( char*& oIter, unsigned int iUsage, unsigned int iIndex, unsigned int iRegister )
    {
        // DCL instruction
        StoreBufferData<DWORD>( DWORD( D3DSIO_DCL | (2 << D3DSI_INSTLENGTH_SHIFT) ), oIter );

        // Usage data
        StoreBufferData<DWORD>( 0x80000000 | (iUsage << D3DSP_DCL_USAGE_SHIFT) & D3DSP_DCL_USAGE_MASK | 
                                (iIndex << D3DSP_DCL_USAGEINDEX_SHIFT) & D3DSP_DCL_USAGEINDEX_MASK , oIter );
        // Destination Register
        StoreBufferData<DWORD>( MakeDstReg( D3DSPR_OUTPUT, iRegister, D3DSP_WRITEMASK_ALL ), oIter );
    }

    static unsigned int DstRegOffset( DWORD opCode )
    {
        switch( opCode )
        {
        case( D3DSIO_BREAK ):
        case( D3DSIO_BREAKC ):
        case( D3DSIO_BREAKP ):
        case( D3DSIO_CALL ):
        case( D3DSIO_CALLNZ ):
        case( D3DSIO_ELSE ):
        case( D3DSIO_ENDIF ):
        case( D3DSIO_ENDLOOP ):
        case( D3DSIO_ENDREP ):
        case( D3DSIO_IF ):
        case( D3DSIO_IFC ):
        case( D3DSIO_LABEL ):
        case( D3DSIO_LOOP ):
        case( D3DSIO_NOP ):
        case( D3DSIO_PHASE ):
        case( D3DSIO_REP ):
        case( D3DSIO_RET ):
            return 0;

        case( D3DSIO_DCL ):
            return 2;

        default:
            return 1;
        }
    }

    void MovRegToReg( char*& oIter, DWORD iDstReg, DWORD iSrcReg )
    {
        StoreBufferData<DWORD>( DWORD( D3DSIO_MOV | (2 << D3DSI_INSTLENGTH_SHIFT) ), oIter );
        StoreBufferData<DWORD>( iDstReg, oIter );
        StoreBufferData<DWORD>( iSrcReg, oIter );
    }

    // Returns output mask
    unsigned char MoveFromDstReg( char*& oIter, DWORD iDstReg, DWORD regType, DWORD regNum, DWORD writeMask )
    {
        DWORD dstRegNum  = iDstReg & D3DSP_REGNUM_MASK;
        DWORD dstRegType = GetDstRegType( iDstReg );
        DWORD dstWriteMask = iDstReg & D3DSP_WRITEMASK_ALL;

        unsigned char outputMask = (unsigned char)(dstWriteMask >> 16);


        if( dstRegNum == regNum && dstRegType == regType ) // It's already in the output. No Need to ouput instruction.
            return outputMask;


        int defaultSwizzle = 0;
        {
            DWORD dstWriteMaskCheck = dstWriteMask >> 16;
            assert( dstWriteMaskCheck );
            while( (dstWriteMaskCheck & 1) == 0 )
            {
                defaultSwizzle++;
                dstWriteMaskCheck >>= 1;
            }
        }

        // Convert the dst register token to a source register token for the mov op
        DWORD srcReg = iDstReg & 0xF0001FFF; // zero out swizzle and source modifier.
        srcReg |= (dstWriteMask & D3DSP_WRITEMASK_0) ? D3DVS_X_X : (defaultSwizzle << (D3DVS_SWIZZLE_SHIFT));
        srcReg |= (dstWriteMask & D3DSP_WRITEMASK_1) ? D3DVS_Y_Y : (defaultSwizzle << (D3DVS_SWIZZLE_SHIFT+2));
        srcReg |= (dstWriteMask & D3DSP_WRITEMASK_2) ? D3DVS_Z_Z : (defaultSwizzle << (D3DVS_SWIZZLE_SHIFT+4));
        srcReg |= (dstWriteMask & D3DSP_WRITEMASK_3) ? D3DVS_W_W : (defaultSwizzle << (D3DVS_SWIZZLE_SHIFT+6));

        DWORD dstReg = MakeDstReg( regType, regNum, writeMask );

        MovRegToReg( oIter, dstReg, srcReg );

        return outputMask;
    }

    unsigned int ShaderInfo::ConvertInstructionNumToOpNum( unsigned int iInstructionNum, bool* oDebuggable,
        size_t* oLoopOffsets, unsigned int* oLoopRegisters, int* oLoopCounters, int* oNumLoops )
    {
        char* dataIter = (char*)mShaderData;
        char* endIter = dataIter + mShaderSize;
        if( !ReadVersionToken( dataIter, NULL ) )
        {
            return -1;
        }

        size_t       loopOffsets[sNumIntRegisters];
        unsigned int loopRegisters[sNumIntRegisters];
        int          loopCounter[sNumIntRegisters];
        unsigned int loopOpNum[sNumIntRegisters];
        int          numLoops = 0;

        unsigned int instructionCount = 0;
        unsigned int opCount = 0;
        bool checkDebuggable = true;
        while( (dataIter < endIter) && instructionCount < iInstructionNum )
        {
            DWORD token = GetBufferData<DWORD>( dataIter );

            if( token == D3DPS_END() || token == D3DVS_END() )
            {
                // Shader finished.
                checkDebuggable = false;
                break;
            }

            DWORD opCode = token & D3DSI_OPCODE_MASK;
            if( opCode == D3DSIO_COMMENT )
            {
                DWORD commentSize = (token & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                dataIter += commentSize * sizeof(DWORD);
            }
            else
            {
                int opLength = (token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
                ++instructionCount;
                ++opCount;
                assert( opCode <= D3DSIO_BREAKP );

                int dstRegOffset = DstRegOffset( opCode );

                switch( opCode )
                {
                // Loop handling
                case( D3DSIO_LOOP ):
                case( D3DSIO_REP ):
                {
                    DWORD srcReg = *(DWORD*)dataIter;
                    DWORD num = GetSrcRegNum( srcReg );
                    DWORD type = GetSrcRegType( srcReg );
                    assert( type == D3DSPR_CONSTINT );
                    loopOffsets[numLoops] = size_t(dataIter - (char*)mShaderData) + opLength * sizeof(DWORD);
                    loopRegisters[numLoops] = num;
                    loopCounter[numLoops] = 0;
                    loopOpNum[numLoops] = opCount;
                    numLoops++;
                }break;

                case( D3DSIO_ENDLOOP ):
                case( D3DSIO_ENDREP ):
                {
                    int loopIdx = numLoops - 1;
                    loopCounter[loopIdx]++;
                    if( loopCounter[loopIdx] < mIntRegisterData[ loopRegisters[loopIdx] << 2 ] )
                    {
                        if( instructionCount < iInstructionNum )
                        {
                            opLength = 0; // To prevent it changing the dataiter which we are about to change ourselves.
                            dataIter = ((char*)mShaderData) + loopOffsets[loopIdx];
                            opCount = loopOpNum[loopIdx];
                        }
                    }
                    else
                    {
                        numLoops--;
                    }
                }break;
                } // switch( opCode )

                dataIter += opLength * sizeof(DWORD);
            }
        }

        bool debuggableOp = false;
        if( checkDebuggable && (dataIter < endIter) )
        {
            DWORD token = GetBufferData<DWORD>( dataIter );
            DWORD opCode = token & D3DSI_OPCODE_MASK;
            if( opCode != D3DSIO_COMMENT )
            {
                int opLength = (token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
                int dstRegOffset = DstRegOffset( opCode );
                debuggableOp = (dstRegOffset != 0 && opLength != 0 );
            }
        }

        if( oLoopOffsets )
        {
            for( int ii = 0; ii < numLoops; ++ii )
                oLoopOffsets[ii] = loopOffsets[ii];
        }
        if( oLoopRegisters )
        {
            for( int ii = 0; ii < numLoops; ++ii )
                oLoopRegisters[ii] = loopRegisters[ii];
        }
        if( oLoopCounters )
        {
            for( int ii = 0; ii < numLoops; ++ii )
                oLoopCounters[ii] = loopCounter[ii];
        }
        if( oNumLoops )
        {
            *oNumLoops = numLoops;
        }
        if( oDebuggable )
        {
            *oDebuggable = debuggableOp;
        }

        return opCount;
    }

    // ------------------------------------------------------------------------
    // External interface functions
    // ------------------------------------------------------------------------

    ShaderInfo GetShaderInfo( void* iData, unsigned int iDataSize, IDirect3DDevice9* iD3DDevice )
    {
        ShaderInfo rvalue;
        memset( &rvalue, 0, sizeof(ShaderInfo) );

        rvalue.mShaderData = iData;
        rvalue.mShaderSize = iDataSize;

        char* dataIter = (char*)iData;
        char* endIter = dataIter + iDataSize;

        if( !ReadVersionToken( dataIter, &rvalue ) )
        {
            return rvalue;
        }

        int loopRegister[sNumIntRegisters];
        int lastLoopType[sNumIntRegisters];
        int curLoopDepth = 0;

        if( iD3DDevice->GetPixelShaderConstantI( 0, rvalue.mIntRegisterData, sNumIntRegisters ) != D3D_OK )
        {
            return rvalue;
        }

        // Find a free constant register
        unsigned int mask[8]; // 256 bits
        memset( mask, 0, sizeof(mask) );

        unsigned int maxVal = 256;
        if( rvalue.mShaderType == PixelShader )
        {
            maxVal = 224;
        }

        int instructionCount = 0;
        int instructionStep = 1;
        int opCount = 0;
        while( dataIter < endIter )
        {
            DWORD token = GetBufferData<DWORD>( dataIter );

            if( token == D3DPS_END() || token == D3DVS_END() )
            {
                // Shader finished.
                break;
            }

            DWORD opCode = token & D3DSI_OPCODE_MASK;
            if( opCode == D3DSIO_COMMENT )
            {
                DWORD commentSize = (token & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                if( commentSize >= 1 )
                {
                    commentSize--;
                    DWORD fourcc = GetBufferData<DWORD>( dataIter );
                    if( fourcc == MAKEFOURCC( 'D', 'B', 'U', 'G' ) )
                    {
                        rvalue.mDebugHeader = (DBUGHeader*)dataIter;
                    }
                    else if( fourcc == MAKEFOURCC( 'C', 'T', 'A', 'B' ) )
                    {
                        rvalue.mConstantTable = (_D3DXSHADER_CONSTANTTABLE*)dataIter;

                        DWORD numConstants = rvalue.mConstantTable->Constants;
                        _D3DXSHADER_CONSTANTINFO* ci = (_D3DXSHADER_CONSTANTINFO*)(((char*)rvalue.mConstantTable) + 
                            rvalue.mConstantTable->ConstantInfo);
                        for( unsigned int ii = 0; ii < numConstants; ++ii )
                        {
                            WORD begin = ci[ii].RegisterIndex;
                            WORD end = begin + ci[ii].RegisterCount;
                            for( WORD iter = begin; iter < end; ++iter )
                            {
                                WORD bitOffset = iter & 0x1F;
                                WORD dwordNum =  iter & 0xFFFFFFE0;
                                mask[ dwordNum ] |= 1 << bitOffset;
                            }
                        }
                    }
                }
                dataIter += commentSize * sizeof(DWORD);
            }
            else
            {
                int dstRegOffset = DstRegOffset( opCode );
                if( dstRegOffset > 0 )
                {
                    DWORD dstReg = ((DWORD*)dataIter)[dstRegOffset-1];

                    DWORD regType = GetDstRegType( dstReg );
                    if( regType == D3DSPR_OUTPUT || regType == D3DSPR_COLOROUT )
                    {
                        int regNum = GetDstRegNum( dstReg );
                        rvalue.mOutputRegisterMask |= 1 << regNum;
                        if( opCode == D3DSIO_MOV )
                        {
                            rvalue.mOutputInstruction[regNum] = instructionCount;
                        }
                    }
                }

                instructionCount += instructionStep;
                ++opCount;
                assert( opCode <= D3DSIO_BREAKP );
                int opLength = (token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;

                switch( opCode )
                {
                case( D3DSIO_DEF ):
                {
                    DWORD dstReg = *(DWORD*)dataIter;
                    DWORD num = GetDstRegNum( dstReg );
                    DWORD bitOffset = num & 0x1F;
                    DWORD dwordNum =  num & 0xFFFFFFE0;
                    mask[ dwordNum ] |= 1 << bitOffset;
                }break;

                case( D3DSIO_DEFI ):
                {
                    DWORD dstReg = *(DWORD*)dataIter;
                    DWORD num = GetDstRegNum( dstReg );
                    rvalue.mIntRegisterData[(num<<2) + 0] = ((DWORD*)dataIter)[1];
                    rvalue.mIntRegisterData[(num<<2) + 1] = ((DWORD*)dataIter)[2];
                    rvalue.mIntRegisterData[(num<<2) + 2] = ((DWORD*)dataIter)[3];
                    rvalue.mIntRegisterData[(num<<2) + 3] = ((DWORD*)dataIter)[4];
                }break;

                // Loop handling
                case( D3DSIO_LOOP ):
                {
                    DWORD srcReg = *(DWORD*)dataIter;
                    DWORD num = GetSrcRegNum( srcReg );
                    DWORD type = GetSrcRegType( srcReg );
                    assert( type == D3DSPR_CONSTINT );
                    instructionStep *= rvalue.mIntRegisterData[ num << 2 ];
                    loopRegister[curLoopDepth]   = num;
                    lastLoopType[curLoopDepth++] = D3DSIO_LOOP;
                }break;

                case( D3DSIO_REP ):
                {
                    DWORD srcReg = *(DWORD*)dataIter;
                    DWORD num = GetSrcRegNum( srcReg );
                    DWORD type = GetSrcRegType( srcReg );
                    assert( type == D3DSPR_CONSTINT );
                    instructionStep *= rvalue.mIntRegisterData[ num << 2 ];
                    loopRegister[curLoopDepth]   = num;
                    lastLoopType[curLoopDepth++] = D3DSIO_REP;
                }break;

                case( D3DSIO_ENDLOOP ):
                {
                    curLoopDepth--;
                    instructionStep /= rvalue.mIntRegisterData[ loopRegister[curLoopDepth] << 2 ];
                    assert( lastLoopType[curLoopDepth] == D3DSIO_LOOP );
                }break;

                case( D3DSIO_ENDREP ):
                {
                    curLoopDepth--;
                    instructionStep /= rvalue.mIntRegisterData[ loopRegister[curLoopDepth] << 2 ];
                    assert( lastLoopType[curLoopDepth] == D3DSIO_REP );
                }break;
                } // switch( opCode )

                dataIter += opLength * sizeof(DWORD);
            }
        }

        // Go through mask and find first free register
        rvalue.mFreeConstantRegister = -1;
        for( int ii = 0; ii < 8; ++ii )
        {
            if( mask[ii] != 0xFFFFFFFF )
            {
                unsigned int bits = mask[ii];
                unsigned int num = 0;
                while( bits & 0x1 )
                {
                    ++num;
                    bits >>= 1;
                }
                rvalue.mFreeConstantRegister = ii * 32 + num;
                break;
            }
        }

        rvalue.mInstructionCount = instructionCount;
        rvalue.mOpCount = opCount;
        return rvalue;
    }

    void CopyVersionToken( char*& ioInputIter, char*& ioOutputIter )
    {
        StoreBufferData<DWORD>( GetBufferData<DWORD>( ioInputIter ), ioOutputIter );
    }

    bool PatchShaderForDebug( unsigned int iInstructionNum, ShaderInfo* iShaderInfo, unsigned int iRemoveTypeMask,
                              void* iData, unsigned int iDataSize,
                              void* oData, unsigned int& oDataSize, unsigned char& oOutputMask, unsigned int* oOpNum )
    {
        if( !iShaderInfo->mDebugHeader )
            return false;

        bool pixel = iShaderInfo->mShaderType == PixelShader;

        char* iIter = (char*)iData;
        char* oIter = (char*)oData;

        char* iInstrEnd;
        char* iIterEnd = ((char*)iData) + iDataSize - sizeof(DWORD);
        assert( *(DWORD*)iIterEnd == D3DSIO_END );

        size_t       loopOffsets[sNumIntRegisters];
        unsigned int loopRegisters[sNumIntRegisters];
        int          loopCounter[sNumIntRegisters];
        int          numLoops = 0;

        if( iInstructionNum == LastInstruction )
        {
            iInstrEnd = iIterEnd;
        }
        else
        {
            bool debuggable = false;
            unsigned int num = iShaderInfo->ConvertInstructionNumToOpNum( iInstructionNum, &debuggable, loopOffsets, 
                loopRegisters, loopCounter, &numLoops );
            if( !debuggable )
                return false;
            iInstrEnd = (char*)iShaderInfo->GetInstructionPointer( num );
            if( oOpNum )
                *oOpNum = num;
        }

        bool intRegisterRedefined[sNumIntRegisters];
        memset( intRegisterRedefined, 0, sizeof(intRegisterRedefined) );

        for( int ii = 0; ii < numLoops; ++ii )
        {
            intRegisterRedefined[ loopRegisters[ii] ] = true;
        }

        // Copy the version
        CopyVersionToken( iIter, oIter );

        // Output the neccessary constants
        if( iShaderInfo->mFreeConstantRegister < 0 ) // TODO: Check for room at the top based on shader type.
            return false;

        // Instruction number constant
        int instrConst = iShaderInfo->mFreeConstantRegister;
        WriteFloatConstantInstruction( oIter, iShaderInfo->mFreeConstantRegister,
            (iInstructionNum == LastInstruction) ? -1.f : float(iInstructionNum), 2.f, 1.f, 0.f );

        int posConst;
        if( !pixel )
        {
            // Vertex position output constant
            posConst = iShaderInfo->mFreeConstantRegister+1;
            WriteFloatConstantInstruction( oIter, iShaderInfo->mFreeConstantRegister+1, 0.f, 0.f, 0.f, 1.f );
            // TODO: the position constant can just use swizzled values from the instruction constant

            // Position output
            WriteDCLOutputInstruction( oIter, D3DDECLUSAGE_POSITION, 0, 0 );

            // Data output in texcoord0
            WriteDCLOutputInstruction( oIter, D3DDECLUSAGE_TEXCOORD, 0, 1 );

            // Instruction output in texcoord1
            WriteDCLOutputInstruction( oIter, D3DDECLUSAGE_TEXCOORD, 1, 2 );
        }

        // If we're inside any loops, set the loop constant registers to be the proper number for the iteration we want.
        for( int ii = 0; ii < numLoops; ++ii )
        {
            unsigned int reg = loopRegisters[ii];
            int* baseData = &iShaderInfo->mIntRegisterData[ reg << 2 ];

            WriteIntConstantInstruction( oIter, reg, loopCounter[ii]+1, baseData[1], baseData[2], baseData[3] );
        }

        DWORD closeOps[32];
        int closeOpsCount = 0;

        // Copy the shader data over and omit any instructions we don't want.
        while( iIter < iInstrEnd )
        {
            DWORD token = GetBufferData<DWORD>( iIter );

            DWORD opCode = token & D3DSI_OPCODE_MASK;
            if( opCode == D3DSIO_COMMENT )
            {
                // Don't copy comments to the patched shader.
                DWORD commentSize = (token & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                iIter += commentSize * sizeof(DWORD);
            }
            else
            {
                int opLength = (token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
                bool copy = true;

                switch( opCode )
                {
                case( D3DSIO_DCL ):
                {
                    if( !pixel )
                    {
                        int dstRegOffset = DstRegOffset( opCode );
                        DWORD dstReg = ((DWORD*)iIter)[dstRegOffset-1];

                        DWORD regType = GetDstRegType( dstReg );
                        if( regType == D3DSPR_OUTPUT )
                        {
                            // For vertex shader we supply our own output register definitions.
                            copy = false;
                        }
                    }
                }break;

                case( D3DSIO_DEFI ):
                {
                    int dstRegOffset = DstRegOffset( opCode );
                    DWORD dstReg = ((DWORD*)iIter)[dstRegOffset-1];
                    DWORD regType = GetDstRegType( dstReg );
                    DWORD num = GetDstRegNum( dstReg );
                    if( regType == D3DSPR_CONSTINT && intRegisterRedefined[num] )
                    {
                        // We've redefined this register for looping purposes so don't define it again.
                        copy = false;
                    }
                }break;

                // Flow control handling
                case( D3DSIO_IF ):
                case( D3DSIO_IFC ):
                    closeOps[closeOpsCount++] = D3DSIO_ENDIF;
                    break;
                case( D3DSIO_LOOP ):
                    closeOps[closeOpsCount++] = D3DSIO_ENDLOOP;
                    break;
                case( D3DSIO_REP ):
                    closeOps[closeOpsCount++] = D3DSIO_ENDREP;
                    break;
                case( D3DSIO_ENDIF ):
                case( D3DSIO_ENDLOOP ):
                case( D3DSIO_ENDREP ):
                    if( closeOpsCount <= 0 || token != closeOps[closeOpsCount-1] )
                    {
                        return false; // Badly formed shader.
                    }

                    --closeOpsCount;
                    break;

                case( D3DSIO_TEXKILL ): if( iRemoveTypeMask & InstructionTypes::Clip ) copy = false; break;

                default: break;
                }

                if( copy )
                {
                    StoreBufferData<DWORD>( token, oIter );
                    for( int ii = 0; ii < opLength; ++ii )
                    {
                        StoreBufferData<DWORD>( GetBufferData<DWORD>( iIter ), oIter );
                    }
                }
                else
                {
                    iIter += sizeof(DWORD) * opLength;
                }
            }
        }

        DWORD dstReg;
        bool doMove;

        if( iInstructionNum == LastInstruction )
        {
            dstReg = 0;
            doMove = false;
        }
        else
        {
            DWORD token = GetBufferData<DWORD>( iIter );
            int opLength = (token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;

            DWORD opCode = token & D3DSI_OPCODE_MASK;

            int dstRegOffset = DstRegOffset( opCode );
            if( dstRegOffset == 0 || opLength == 0 )
                return false;

            dstReg = ((DWORD*)iIter)[dstRegOffset-1];
            DWORD regType = GetDstRegType( dstReg );

            bool copy = true;
            if( !pixel && opCode == D3DSIO_DCL && GetDstRegType( dstReg ) == D3DSPR_OUTPUT )
            {
                // Skip output definitions (nothing to output).
                return false;
            }
            else if( opCode == D3DSIO_MOV && (regType == D3DSPR_OUTPUT || regType == D3DSPR_COLOROUT) )
            {
                // The instruction is trying to mov somethign to an output register.
                // Change the output register to be the one we want instead.
                copy = false;
                StoreBufferData<DWORD>( token, oIter );

                DWORD dstWriteMask = dstReg & D3DSP_WRITEMASK_ALL;
                oOutputMask = (unsigned char)(dstWriteMask >> 16);
                dstReg &= ~D3DSP_REGNUM_MASK;
                dstReg |= (pixel ? 1 : 2);
                StoreBufferData<DWORD>( dstReg, oIter );
                StoreBufferData<DWORD>( ((DWORD*)iIter)[1], oIter ); // src reg
                doMove = false; // This move replaces the other move
            }

            if( copy )
            {
                // Copy the debug op
                StoreBufferData<DWORD>( token, oIter );
                for( int ii = 0; ii < opLength; ++ii )
                    StoreBufferData<DWORD>( GetBufferData<DWORD>( iIter ), oIter );
                doMove = true;
            }
            else
            {
                iIter += sizeof(DWORD) * opLength;
            }
        }


        if( pixel )
        {
            // Mov last instruction output data to o1
            if( doMove )
            {
                oOutputMask = MoveFromDstReg( oIter, dstReg, D3DSPR_COLOROUT, 1, D3DSP_WRITEMASK_ALL );
            }

            // Output instruction marker to o0
            MovRegToReg( oIter, MakeDstReg( D3DSPR_COLOROUT, 0, D3DSP_WRITEMASK_ALL ),
                         MakeSrcReg( D3DSPR_CONST, instrConst, D3DVS_NOSWIZZLE, D3DSPSM_NONE ) );
        }
        else
        {
            // Mov last instruction output data to o2
            if( doMove )
            {
                oOutputMask = MoveFromDstReg( oIter, dstReg, D3DSPR_OUTPUT, 2, D3DSP_WRITEMASK_ALL );
            }

            // Output position to o0
            MovRegToReg( oIter, MakeDstReg( D3DSPR_OUTPUT, 0, D3DSP_WRITEMASK_ALL ),
                         MakeSrcReg( D3DSPR_CONST, posConst, D3DVS_NOSWIZZLE, D3DSPSM_NONE ) );

            // Add instruction marker data to o1
            MovRegToReg( oIter, MakeDstReg( D3DSPR_OUTPUT, 1, D3DSP_WRITEMASK_ALL ),
                         MakeSrcReg( D3DSPR_CONST, instrConst, D3DVS_NOSWIZZLE, D3DSPSM_NONE ) );
        }

        // If we're inside a loop we need to finish copying all the instruction in the loop to ensure that if we go
        // through the loops multiple times the proper instructions are there. We also need to make sure that nothing
        // overwrites our output registers in the loop.
        while( (iIter < iIterEnd) && closeOpsCount )
        {
            DWORD token = GetBufferData<DWORD>( iIter );

            DWORD opCode = token & D3DSI_OPCODE_MASK;
            if( opCode == D3DSIO_COMMENT )
            {
                // Don't copy comments to the patched shader.
                DWORD commentSize = (token & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                iIter += commentSize * sizeof(DWORD);
            }
            else
            {
                int opLength = (token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
                bool copy = true;

                switch( opCode )
                {
                case( D3DSIO_MOV ):
                {
                    int dstRegOffset = DstRegOffset( opCode );
                    dstReg = ((DWORD*)iIter)[dstRegOffset-1];
                    DWORD regType = GetDstRegType( dstReg );
                    DWORD num = GetDstRegNum( dstReg );
                    if( (regType == D3DSPR_OUTPUT || regType == D3DSPR_COLOROUT) )
                    {
                        // We've overridden output registers so we don't want loop instructions to jack with them.
                        copy = false;
                    }
                }break;
                // Flow control handling
                case( D3DSIO_IF ):
                case( D3DSIO_IFC ):
                    closeOps[closeOpsCount++] = D3DSIO_ENDIF;
                    break;
                case( D3DSIO_LOOP ):
                    closeOps[closeOpsCount++] = D3DSIO_ENDLOOP;
                    break;
                case( D3DSIO_REP ):
                    closeOps[closeOpsCount++] = D3DSIO_ENDREP;
                    break;
                case( D3DSIO_ENDIF ):
                case( D3DSIO_ENDLOOP ):
                case( D3DSIO_ENDREP ):
                    if( closeOpsCount <= 0 || token != closeOps[closeOpsCount-1] )
                    {
                        return false; // Badly formed shader.
                    }

                    --closeOpsCount;
                    break;

                case( D3DSIO_TEXKILL ): if( iRemoveTypeMask & InstructionTypes::Clip ) copy = false; break;

                default: break;
                }

                if( copy )
                {
                    StoreBufferData<DWORD>( token, oIter );
                    for( int ii = 0; ii < opLength; ++ii )
                    {
                        StoreBufferData<DWORD>( GetBufferData<DWORD>( iIter ), oIter );
                    }
                }
                else
                {
                    iIter += sizeof(DWORD) * opLength;
                }
            }
        }

        //Close any if, for, or loop instructions
        for( int ii = closeOpsCount-1; ii >=0 ; --ii )
        {
            StoreBufferData<DWORD>( closeOps[ii], oIter );
        }

        // Write the shader end instruction.
        StoreBufferData<DWORD>( D3DSIO_END, oIter );

        oDataSize = UINT(oIter - (char*)oData);

        return true;
    }

    unsigned int FindOutputInstruction( D3DDECLUSAGE iUsage, unsigned int iIndex, void* iSrc, unsigned int iSrcSize )
    {
        char* iIter = (char*)iSrc;
        char* iIterEnd = ((char*)iSrc) + iSrcSize - sizeof(DWORD);
        assert( *(DWORD*)iIterEnd == D3DSIO_END );
        while( iIter < iIterEnd )
        {
            DWORD token = GetBufferData<DWORD>( iIter );

            DWORD opCode = token & D3DSI_OPCODE_MASK;
            if( opCode == D3DSIO_COMMENT )
            {
                DWORD commentSize = (token & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                iIter += commentSize * sizeof(DWORD);
            }
            else
            {
                if( opCode == D3DSIO_DCL )
                {
                    DWORD usage = *(DWORD*)(iIter + sizeof(DWORD));
                    DWORD type = (usage & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT;
                    DWORD index = (usage & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;
                }

                int opLength = (token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
                iIter += sizeof(DWORD) * opLength;
            }
        }

        return -1;
    }
};