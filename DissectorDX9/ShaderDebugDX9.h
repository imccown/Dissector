#pragma once
#include <d3dx9.h>
#include "DX9Internal.h"

namespace ShaderDebugDX9
{
    // ------------------------------------------------------------------------
    // Structures 
    // ------------------------------------------------------------------------
    enum ShaderType
    {
        PixelShader = -1,
        VertexShader = -2,
    };

    // Reverse engineered debugging data headers
    struct InstructionDebugInfo
    {
        WORD lineNumber;
        WORD fileNumber;
        DWORD offsetToInstructionByteCode; // This is an offset from the beginning of the file, not the header start.
    };

    struct VariableInstructionEntry
    {
        DWORD opNum;
        WORD  offsetIds[4]; // how the xyzw components of the output register map to the variable's structure (element offset)
    };

    struct VariableEntry
    {
        DWORD offsetToFunctionName; // Only applies to variables that are function inputs
        DWORD offsetToName;
        DWORD offsetToTypeInfo; // points to type _D3DXSHADER_TYPEINFO for variable
        DWORD numInstructionEntries;
        DWORD offsetToInstructionEntries;
    };

    struct DBUGHeader
    {
        DWORD headerSize; // in bytes
        DWORD offsetToCreatorName;
        DWORD zero;
        DWORD countOfFilenames;
        DWORD offsetToFilenameOffsets;
        DWORD instructionCount;
        DWORD offsetToInstructionDebugInfo;
        DWORD numVariableEntries;
        DWORD offsetToVariableEntries;
        DWORD offsetToEntryPointName;
    };

    struct ShaderInfo
    {
        void* mShaderData;
        unsigned int mShaderSize;

        short mShaderType;
        short mShaderVersion;

        // Op number refers to the offset in to the shader program, instruction number means the nth instruction executed
        // These can differ due to loops and reps (but not ifs).
        unsigned int mInstructionCount; // Contains total number of instructions including loop iterations
        unsigned int mOpCount; // Total number of instructions (i.e. loops only count once)
        unsigned int mVariableCount;

        unsigned int mOutputRegisterMask; // bit 1 is c0 (o0 for vertex), bit 2 is c1, etc
        unsigned int mOutputInstruction[32];
        int          mIntRegisterData[4*sNumIntRegisters];

        DBUGHeader*                mDebugHeader;
        _D3DXSHADER_CONSTANTTABLE* mConstantTable;
        int mFreeConstantRegister; // Need one of these for shader debugging.

        char* GetCreatorName()
        {
            return ((char*)mDebugHeader) + mDebugHeader->offsetToCreatorName;
        }

        char* GetFilename( unsigned int iNum )
        {
            DWORD* filenameOffsets = (DWORD*)( ((char*)mDebugHeader) + mDebugHeader->offsetToFilenameOffsets );

            return ((char*)mDebugHeader) + filenameOffsets[ iNum ];
        }

        VariableEntry* GetVariableEntries()
        {
            return (VariableEntry*)( ((char*)mDebugHeader) + mDebugHeader->offsetToVariableEntries );
        }

        // These works on the OP number not the instruction number
        InstructionDebugInfo* GetInstructionDebugInfo()
        {
            return (InstructionDebugInfo*)( ((char*)mDebugHeader) + mDebugHeader->offsetToInstructionDebugInfo );
        }

        DWORD* GetInstructionPointer( unsigned int iOpNum )
        {
            InstructionDebugInfo* dbgInfo = &GetInstructionDebugInfo()[iOpNum];
            return (DWORD*)( ((char*)mShaderData) + dbgInfo->offsetToInstructionByteCode );
        }

        unsigned int ConvertInstructionNumToOpNum( unsigned int iInstructionNum, bool* oDebuggable = NULL,
            size_t* oLoopOffsets = NULL, unsigned int* oLoopRegisters = NULL, int* oLoopCounters = NULL, int* oNumLoops = NULL );

        template <typename tType>
        tType DebugOffsetToPointer( DWORD iOffset )
        {
            return (tType)(((char*)mDebugHeader) + iOffset);
        }
    };  

    // ------------------------------------------------------------------------
    // Functions
    // ------------------------------------------------------------------------
    ShaderInfo GetShaderInfo( void* iData, unsigned int iDataSize, IDirect3DDevice9* iD3DDevice );

    enum 
    {
        ShaderPatchSize = 48*4,
        LastInstruction = unsigned int(-1),
    };

    namespace InstructionTypes
    {
        enum Type // bitmask
        {
            Clip        = 0x00000001,
            Texture     = 0x00000002,
            //Next      = 0x00000004,
        };
    }

    // oDst should be at least iSrcSize + ShaderPatchSize to ensure we have enough room to add the instructions.
    // The final size of the new shader will be written to ioDstSize.
    bool PatchShaderForDebug( unsigned int iInstructionNum, ShaderInfo* iShaderInfo, unsigned int iRemoveTypeMask,
                              void* iSrc, unsigned int iSrcSize,
                              void* oDst, unsigned int& ioDstSize, unsigned char& oOutputMask,
                              unsigned int* oOpNum = NULL );

    unsigned int FindOutputInstruction( D3DDECLUSAGE iUsage, unsigned int iIndex, void* iSrc, unsigned int iSrcSize );
};