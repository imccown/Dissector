// Need initguid to get com ids for query interface.
#define INITGUID
#include "DissectorDX9.h"
#include "..\Dissector\Dissector.h"
#include <assert.h>
#include <d3dx9.h>
#include "ShaderDebugDX9.h"
#include "..\Dissector\DissectorHelpers.h"
#include "DX9Internal.h"
#include "DX9Helpers.h"
#include "DX9Tests.h"
#include "DX9Capture.h"
#include <stdio.h>
#pragma warning ( disable : 4996 )
DX9Data sDX9Data;

static unsigned char sSingleColorShaderCode[156] = {
	0x00, 0x03, 0xFF, 0xFF, 0xFE, 0xFF, 0x21, 0x00, 0x43, 0x54, 0x41, 0x42,
	0x1C, 0x00, 0x00, 0x00, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF,
	0x01, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x48, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6B, 0x43, 0x6F, 0x6C, 0x6F, 0x72, 0x00, 0xAB, 0x01, 0x00, 0x03, 0x00,
	0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x70, 0x73, 0x5F, 0x33, 0x5F, 0x30, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F,
	0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53,
	0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D,
	0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x39, 0x2E, 0x32, 0x39, 0x2E, 0x39,
	0x35, 0x32, 0x2E, 0x33, 0x31, 0x31, 0x31, 0x00, 0x01, 0x00, 0x00, 0x02,
	0x00, 0x08, 0x0F, 0x80, 0x00, 0x00, 0xE4, 0xA0, 0xFF, 0xFF, 0x00, 0x00
};

static unsigned char sPosTexVSRaw[636] = {
	0x00, 0x03, 0xFE, 0xFF, 0xFE, 0xFF, 0x6B, 0x00, 0x44, 0x42, 0x55, 0x47,
	0x28, 0x00, 0x00, 0x00, 0x74, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x5C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4C, 0x01, 0x00, 0x00,
	0x94, 0x00, 0x00, 0x00, 0x44, 0x3A, 0x5C, 0x63, 0x6F, 0x64, 0x65, 0x5C,
	0x44, 0x69, 0x73, 0x73, 0x65, 0x63, 0x74, 0x6F, 0x72, 0x5C, 0x44, 0x69,
	0x73, 0x73, 0x65, 0x63, 0x74, 0x6F, 0x72, 0x44, 0x58, 0x39, 0x5C, 0x50,
	0x6F, 0x73, 0x56, 0x65, 0x72, 0x74, 0x65, 0x78, 0x2E, 0x68, 0x6C, 0x73,
	0x6C, 0x00, 0xAB, 0xAB, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
	0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x28, 0x02, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0xFF, 0x34, 0x02, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
	0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x4C, 0x02, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
	0x6C, 0x02, 0x00, 0x00, 0x50, 0x6F, 0x73, 0x56, 0x65, 0x72, 0x74, 0x65,
	0x78, 0x56, 0x53, 0x00, 0x69, 0x50, 0x6F, 0x73, 0x69, 0x74, 0x69, 0x6F,
	0x6E, 0x00, 0xAB, 0xAB, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 0x54, 0x65, 0x78,
	0x30, 0x00, 0xAB, 0xAB, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00,
	0xAC, 0x00, 0x00, 0x00, 0xBC, 0x00, 0x00, 0x00, 0xC4, 0x00, 0x00, 0x00,
	0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x01, 0x00, 0x02, 0x00,
	0xD4, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x02, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x05, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0x69, 0x56, 0x65, 0x72, 0x74, 0x00, 0xAB, 0xAB,
	0xA0, 0x00, 0x00, 0x00, 0xC4, 0x00, 0x00, 0x00, 0xBC, 0x00, 0x00, 0x00,
	0xC4, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00,
	0x01, 0x00, 0x02, 0x00, 0x14, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x03, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0x94, 0x00, 0x00, 0x00, 0xE4, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0xF4, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00,
	0x24, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x34, 0x01, 0x00, 0x00,
	0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52,
	0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65,
	0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x39,
	0x2E, 0x32, 0x39, 0x2E, 0x39, 0x35, 0x32, 0x2E, 0x33, 0x31, 0x31, 0x31,
	0x00, 0xAB, 0xAB, 0xAB, 0xFE, 0xFF, 0x16, 0x00, 0x43, 0x54, 0x41, 0x42,
	0x1C, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFE, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
	0x1C, 0x00, 0x00, 0x00, 0x76, 0x73, 0x5F, 0x33, 0x5F, 0x30, 0x00, 0x4D,
	0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29,
	0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72,
	0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x39, 0x2E,
	0x32, 0x39, 0x2E, 0x39, 0x35, 0x32, 0x2E, 0x33, 0x31, 0x31, 0x31, 0x00,
	0x51, 0x00, 0x00, 0x05, 0x00, 0x00, 0x0F, 0xA0, 0x00, 0x00, 0x80, 0x3F,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x1F, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0F, 0x90,
	0x1F, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00, 0x80, 0x01, 0x00, 0x0F, 0x90,
	0x1F, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0F, 0xE0,
	0x1F, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00, 0x80, 0x01, 0x00, 0x03, 0xE0,
	0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x0F, 0xE0, 0x00, 0x00, 0x04, 0x90,
	0x00, 0x00, 0x50, 0xA0, 0x00, 0x00, 0x15, 0xA0, 0x01, 0x00, 0x00, 0x02,
	0x01, 0x00, 0x03, 0xE0, 0x01, 0x00, 0xE4, 0x90, 0xFF, 0xFF, 0x00, 0x00
};

static unsigned char sSingleTexPSRaw[556] = {
	0x00, 0x03, 0xFF, 0xFF, 0xFE, 0xFF, 0x4C, 0x00, 0x44, 0x42, 0x55, 0x47,
	0x28, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00,
	0x80, 0x00, 0x00, 0x00, 0x44, 0x3A, 0x5C, 0x63, 0x6F, 0x64, 0x65, 0x5C,
	0x44, 0x69, 0x73, 0x73, 0x65, 0x63, 0x74, 0x6F, 0x72, 0x5C, 0x44, 0x69,
	0x73, 0x73, 0x65, 0x63, 0x74, 0x6F, 0x72, 0x44, 0x58, 0x39, 0x5C, 0x53,
	0x69, 0x6E, 0x67, 0x6C, 0x65, 0x54, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65,
	0x2E, 0x68, 0x6C, 0x73, 0x6C, 0x00, 0xAB, 0xAB, 0x28, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0xFF, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
	0xFC, 0x01, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x18, 0x02, 0x00, 0x00, 0x53, 0x69, 0x6E, 0x67,
	0x6C, 0x65, 0x54, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x50, 0x53, 0x00,
	0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x02, 0x00, 0x03, 0x00, 0x69, 0x54, 0x65, 0x78, 0x30, 0x00, 0xAB, 0xAB,
	0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
	0x90, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00,
	0x80, 0x00, 0x00, 0x00, 0xAC, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0xC4, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72,
	0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C,
	0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F,
	0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x39, 0x2E, 0x32, 0x39, 0x2E,
	0x39, 0x35, 0x32, 0x2E, 0x33, 0x31, 0x31, 0x31, 0x00, 0xAB, 0xAB, 0xAB,
	0xFE, 0xFF, 0x2D, 0x00, 0x43, 0x54, 0x41, 0x42, 0x1C, 0x00, 0x00, 0x00,
	0x7F, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0x02, 0x00, 0x00, 0x00,
	0x1C, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00,
	0x44, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x67, 0x54, 0x65, 0x78, 0x00, 0xAB, 0xAB, 0xAB,
	0x04, 0x00, 0x0C, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x75, 0x76, 0x4F, 0x66, 0x66, 0x73, 0x65, 0x74,
	0x00, 0xAB, 0xAB, 0xAB, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x73, 0x5F, 0x33,
	0x5F, 0x30, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74,
	0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68,
	0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65,
	0x72, 0x20, 0x39, 0x2E, 0x32, 0x39, 0x2E, 0x39, 0x35, 0x32, 0x2E, 0x33,
	0x31, 0x31, 0x31, 0x00, 0x1F, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00, 0x80,
	0x00, 0x00, 0x03, 0x90, 0x1F, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x90,
	0x00, 0x08, 0x0F, 0xA0, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x80,
	0x00, 0x00, 0xE4, 0xA0, 0x00, 0x00, 0xE4, 0x90, 0x42, 0x00, 0x00, 0x03,
	0x00, 0x08, 0x0F, 0x80, 0x00, 0x00, 0xE4, 0x80, 0x00, 0x08, 0xE4, 0xA0,
	0xFF, 0xFF, 0x00, 0x00
};


static unsigned char sVSDebugPSRaw[148] = {
	0x00, 0x03, 0xFF, 0xFF, 0xFE, 0xFF, 0x16, 0x00, 0x43, 0x54, 0x41, 0x42,
	0x1C, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x1C, 0x00, 0x00, 0x00, 0x70, 0x73, 0x5F, 0x33, 0x5F, 0x30, 0x00, 0x4D,
	0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29,
	0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72,
	0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x39, 0x2E,
	0x32, 0x39, 0x2E, 0x39, 0x35, 0x32, 0x2E, 0x33, 0x31, 0x31, 0x31, 0x00,
	0x1F, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0F, 0x90,
	0x1F, 0x00, 0x00, 0x02, 0x05, 0x00, 0x01, 0x80, 0x01, 0x00, 0x0F, 0x90,
	0x01, 0x00, 0x00, 0x02, 0x00, 0x08, 0x0F, 0x80, 0x00, 0x00, 0xE4, 0x90,
	0x01, 0x00, 0x00, 0x02, 0x01, 0x08, 0x0F, 0x80, 0x01, 0x00, 0xE4, 0x90,
	0xFF, 0xFF, 0x00, 0x00
};

const D3DRENDERSTATETYPE sCaptureRenderStates[sNumCaptureRenderStates] = {
        D3DRS_ZENABLE,
        D3DRS_FILLMODE,
        D3DRS_ZWRITEENABLE,
        D3DRS_ALPHATESTENABLE,
        D3DRS_LASTPIXEL,
        D3DRS_SRCBLEND,
        D3DRS_DESTBLEND,
        D3DRS_CULLMODE,
        D3DRS_ZFUNC,
        D3DRS_ALPHAREF,
        D3DRS_ALPHAFUNC,
        D3DRS_ALPHABLENDENABLE,
        D3DRS_STENCILENABLE,
        D3DRS_STENCILFAIL,
        D3DRS_STENCILZFAIL,
        D3DRS_STENCILPASS,
        D3DRS_STENCILFUNC,
        D3DRS_STENCILREF,
        D3DRS_STENCILMASK,
        D3DRS_STENCILWRITEMASK,
        D3DRS_TEXTUREFACTOR,
        D3DRS_CLIPPING,
        D3DRS_CLIPPLANEENABLE,
        D3DRS_POINTSIZE,
        D3DRS_POINTSIZE_MIN,
        D3DRS_POINTSPRITEENABLE,
        D3DRS_POINTSCALEENABLE,
        D3DRS_POINTSCALE_A,
        D3DRS_POINTSCALE_B,
        D3DRS_POINTSCALE_C,
        D3DRS_MULTISAMPLEANTIALIAS,
        D3DRS_MULTISAMPLEMASK,
        D3DRS_POINTSIZE_MAX,
        D3DRS_COLORWRITEENABLE,
        D3DRS_BLENDOP,
        D3DRS_SCISSORTESTENABLE,
        D3DRS_SLOPESCALEDEPTHBIAS,
        D3DRS_ANTIALIASEDLINEENABLE,
        D3DRS_TWOSIDEDSTENCILMODE,
        D3DRS_CCW_STENCILFAIL,
        D3DRS_CCW_STENCILZFAIL,
        D3DRS_CCW_STENCILPASS,
        D3DRS_CCW_STENCILFUNC,
        D3DRS_COLORWRITEENABLE1,
        D3DRS_COLORWRITEENABLE2,
        D3DRS_COLORWRITEENABLE3,
        D3DRS_BLENDFACTOR,
        D3DRS_SRGBWRITEENABLE,
        D3DRS_DEPTHBIAS,
        D3DRS_SEPARATEALPHABLENDENABLE,
        D3DRS_SRCBLENDALPHA,
        D3DRS_DESTBLENDALPHA,
        D3DRS_BLENDOPALPHA
};


const D3DSAMPLERSTATETYPE sCaptureSamplerStates[sNumCaptureSamplerStates] = {
        D3DSAMP_ADDRESSU,
        D3DSAMP_ADDRESSV,
        D3DSAMP_ADDRESSW,
        D3DSAMP_BORDERCOLOR,
        D3DSAMP_MAGFILTER,
        D3DSAMP_MINFILTER,
        D3DSAMP_MIPFILTER,
        D3DSAMP_MIPMAPLODBIAS,
        D3DSAMP_MAXMIPLEVEL,
        D3DSAMP_MAXANISOTROPY,
        D3DSAMP_SRGBTEXTURE,
        D3DSAMP_ELEMENTINDEX,
};

namespace DissectorDX9
{
    Dissector::RenderStateType rts[] =
    {
        { RT_PIXELSHADER,        0, Dissector::RSTF_VISUALIZE_RESOURCE_PIXEL_SHADER, "Pixel Shader" },
        { RT_VERTEXSHADER,       0, Dissector::RSTF_VISUALIZE_RESOURCE_VERTEX_SHADER, "Vertex Shader" },

        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_START, "Textures" },
        { RT_TEXTURE_BEGIN +  0, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 0" },
        { RT_TEXTURE_BEGIN +  1, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 1" },
        { RT_TEXTURE_BEGIN +  2, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 2" },
        { RT_TEXTURE_BEGIN +  3, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 3" },
        { RT_TEXTURE_BEGIN +  4, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 4" },
        { RT_TEXTURE_BEGIN +  5, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 5" },
        { RT_TEXTURE_BEGIN +  6, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 6" },
        { RT_TEXTURE_BEGIN +  7, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 7" },
        //{ RT_TEXTURE_BEGIN +  8, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 8" },
        //{ RT_TEXTURE_BEGIN +  9, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 9" },
        //{ RT_TEXTURE_BEGIN + 10, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 10" },
        //{ RT_TEXTURE_BEGIN + 11, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 11" },
        //{ RT_TEXTURE_BEGIN + 12, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 12" },
        //{ RT_TEXTURE_BEGIN + 13, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 13" },
        //{ RT_TEXTURE_BEGIN + 14, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 14" },
        //{ RT_TEXTURE_BEGIN + 15, 0, Dissector::RSTF_VISUALIZE_RESOURCE_TEXTURE, "Texture 15" },
        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_END, "Textures" },

        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_START, "Vertex Streams" },
        { RT_INDEXBUFFER,              0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Index Buffer" },
        { RT_VERTEXDECLARATION,        0, Dissector::RSTF_VISUALIZE_RESOURCE_VERTEXTYPE, "Vertex Declaration" },
        { RT_STREAMSOURCE_BEGIN + ( 0 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 0" },
        { RT_STREAMSOURCE_BEGIN + ( 0 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 0 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 0 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 0 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 0 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 0 Divider" },
        { RT_STREAMSOURCE_BEGIN + ( 1 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 1" },
        { RT_STREAMSOURCE_BEGIN + ( 1 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 1 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 1 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 1 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 1 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 1 Divider" },
        { RT_STREAMSOURCE_BEGIN + ( 2 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 2" },
        { RT_STREAMSOURCE_BEGIN + ( 2 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 2 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 2 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 2 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 2 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 2 Divider" },
        { RT_STREAMSOURCE_BEGIN + ( 3 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 3" },
        { RT_STREAMSOURCE_BEGIN + ( 3 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 3 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 3 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 3 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 3 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 3 Divider" },
        { RT_STREAMSOURCE_BEGIN + ( 4 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 4" },
        { RT_STREAMSOURCE_BEGIN + ( 4 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 4 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 4 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 4 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 4 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 4 Divider" },
        { RT_STREAMSOURCE_BEGIN + ( 5 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 5" },
        { RT_STREAMSOURCE_BEGIN + ( 5 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 5 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 5 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 5 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 5 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 5 Divider" },
        { RT_STREAMSOURCE_BEGIN + ( 6 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 6" },
        { RT_STREAMSOURCE_BEGIN + ( 6 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 6 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 6 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 6 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 6 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 6 Divider" },
        { RT_STREAMSOURCE_BEGIN + ( 7 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 7" },
        { RT_STREAMSOURCE_BEGIN + ( 7 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 7 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 7 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 7 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 7 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 7 Divider" },
        { RT_STREAMSOURCE_BEGIN + ( 8 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 8" },
        { RT_STREAMSOURCE_BEGIN + ( 8 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 8 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 8 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 8 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 8 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 8 Divider" },
        { RT_STREAMSOURCE_BEGIN + ( 9 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 9" },
        { RT_STREAMSOURCE_BEGIN + ( 9 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 9 Offset" },
        { RT_STREAMSOURCE_BEGIN + ( 9 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 9 Stride" },
        { RT_STREAMSOURCE_BEGIN + ( 9 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 9 Divider" },
        { RT_STREAMSOURCE_BEGIN + (10 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 10" },
        { RT_STREAMSOURCE_BEGIN + (10 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 10 Offset" },
        { RT_STREAMSOURCE_BEGIN + (10 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 10 Stride" },
        { RT_STREAMSOURCE_BEGIN + (10 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 10 Divider" },
        { RT_STREAMSOURCE_BEGIN + (11 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 11" },
        { RT_STREAMSOURCE_BEGIN + (11 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 11 Offset" },
        { RT_STREAMSOURCE_BEGIN + (11 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 11 Stride" },
        { RT_STREAMSOURCE_BEGIN + (11 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 11 Divider" },
        { RT_STREAMSOURCE_BEGIN + (12 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 12" },
        { RT_STREAMSOURCE_BEGIN + (12 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 12 Offset" },
        { RT_STREAMSOURCE_BEGIN + (12 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 12 Stride" },
        { RT_STREAMSOURCE_BEGIN + (12 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 12 Divider" },
        { RT_STREAMSOURCE_BEGIN + (13 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 13" },
        { RT_STREAMSOURCE_BEGIN + (13 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 13 Offset" },
        { RT_STREAMSOURCE_BEGIN + (13 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 13 Stride" },
        { RT_STREAMSOURCE_BEGIN + (13 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 13 Divider" },
        { RT_STREAMSOURCE_BEGIN + (14 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 14" },
        { RT_STREAMSOURCE_BEGIN + (14 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 14 Offset" },
        { RT_STREAMSOURCE_BEGIN + (14 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 14 Stride" },
        { RT_STREAMSOURCE_BEGIN + (14 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 14 Divider" },
        { RT_STREAMSOURCE_BEGIN + (15 * 4) + 0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_BUFFER, "Buffer 15" },
        { RT_STREAMSOURCE_BEGIN + (15 * 4) + 1,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 15 Offset" },
        { RT_STREAMSOURCE_BEGIN + (15 * 4) + 2,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 15 Stride" },
        { RT_STREAMSOURCE_BEGIN + (15 * 4) + 3,  0, Dissector::RSTF_VISUALIZE_INT, "Buffer 15 Divider" },
        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_END, "Vertex Streams" },

        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_START, "Render Targets + Write Masks" },
        { RT_RENDERTARGET_BEGIN +  0,  0, Dissector::RSTF_VISUALIZE_RESOURCE_RENDERTARGET, "Render Target 0" },
        { RT_RENDERTARGET_BEGIN +  1,  0, Dissector::RSTF_VISUALIZE_RESOURCE_RENDERTARGET, "Render Target 1" },
        { RT_RENDERTARGET_BEGIN +  2,  0, Dissector::RSTF_VISUALIZE_RESOURCE_RENDERTARGET, "Render Target 2" },
        { RT_RENDERTARGET_BEGIN +  3,  0, Dissector::RSTF_VISUALIZE_RESOURCE_RENDERTARGET, "Render Target 3" },
        { RT_DEPTHSTENCILSURFACE,      0, Dissector::RSTF_VISUALIZE_RESOURCE_DEPTHTARGET, "Depth Stencil" },
        { RT_RS_COLORWRITEENABLE,      0, Dissector::RSTF_VISUALIZE_HEX, "Color Write Mask" },   // D3DRS_COLORWRITEENABLE,
        { RT_RS_COLORWRITEENABLE1,     0, Dissector::RSTF_VISUALIZE_HEX, "Color Write Mask 1" }, // D3DRS_COLORWRITEENABLE1,
        { RT_RS_COLORWRITEENABLE2,     0, Dissector::RSTF_VISUALIZE_HEX, "Color Write Mask 2" }, // D3DRS_COLORWRITEENABLE2,
        { RT_RS_COLORWRITEENABLE3,     0, Dissector::RSTF_VISUALIZE_HEX, "Color Write Mask 3" }, // D3DRS_COLORWRITEENABLE3,

        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_END,   "Render Targets" },

        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_START, "Culling & Clipping" },
        { RT_RS_CULLMODE,           0,                     ENUM_CULL, "Cull Mode" }, // D3DRS_CULLMODE,
        { RT_CLIPSTATUS,            0, Dissector::RSTF_VISUALIZE_HEX, "Clip Status" },
        { RT_RS_CLIPPING,           0, Dissector::RSTF_VISUALIZE_HEX, "Primitive Clipping" }, // D3DRS_CLIPPING,
        { RT_RS_CLIPPLANEENABLE,    0, Dissector::RSTF_VISUALIZE_HEX, "Enable Clip Planes" }, // D3DRS_CLIPPLANEENABLE,
        { RT_CLIPPLANE_BEGIN +  0,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 0 X" },
        { RT_CLIPPLANE_BEGIN +  1,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 0 Y" },
        { RT_CLIPPLANE_BEGIN +  2,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 0 Z" },
        { RT_CLIPPLANE_BEGIN +  3,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 0 W" },
        { RT_CLIPPLANE_BEGIN +  4,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 1 X" },
        { RT_CLIPPLANE_BEGIN +  5,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 1 Y" },
        { RT_CLIPPLANE_BEGIN +  6,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 1 Z" },
        { RT_CLIPPLANE_BEGIN +  7,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 1 W" },
        { RT_CLIPPLANE_BEGIN +  8,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 2 X" },
        { RT_CLIPPLANE_BEGIN +  9,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 2 Y" },
        { RT_CLIPPLANE_BEGIN + 10,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 2 Z" },
        { RT_CLIPPLANE_BEGIN + 11,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 2 W" },
        { RT_CLIPPLANE_BEGIN + 12,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 3 X" },
        { RT_CLIPPLANE_BEGIN + 13,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 3 Y" },
        { RT_CLIPPLANE_BEGIN + 14,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 3 Z" },
        { RT_CLIPPLANE_BEGIN + 15,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Clip plane 3 W" },
        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_END,   "Clipping" },

        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_START, "Viewport & Scissor" },
        { RT_VIEWPORT_BEGIN    +  0,  0, Dissector::RSTF_VISUALIZE_INT, "Viewport X" },
        { RT_VIEWPORT_BEGIN    +  1,  0, Dissector::RSTF_VISUALIZE_INT, "Viewport Y" },
        { RT_VIEWPORT_BEGIN    +  2,  0, Dissector::RSTF_VISUALIZE_INT, "Viewport Width" },
        { RT_VIEWPORT_BEGIN    +  3,  0, Dissector::RSTF_VISUALIZE_INT, "Viewport Height" },
        { RT_VIEWPORT_BEGIN    +  4,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Viewport Min Z" },
        { RT_VIEWPORT_BEGIN    +  5,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Viewport Max Z" },
        { RT_RS_SCISSORTESTENABLE,    0, Dissector::RSTF_VISUALIZE_BOOL, "Scissor Test" }, // D3DRS_SCISSORTESTENABLE,
        { RT_SCISSORRECT_BEGIN +  0,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Scissor Rect Left" },
        { RT_SCISSORRECT_BEGIN +  1,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Scissor Rect Top" },
        { RT_SCISSORRECT_BEGIN +  2,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Scissor Rect Right" },
        { RT_SCISSORRECT_BEGIN +  3,  0, Dissector::RSTF_VISUALIZE_FLOAT, "Scissor Rect Bottom" },
        { -1,                    0, Dissector::RSTF_VISUALIZE_GROUP_END,   "Viewport & Scissor" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Sampler 0" },
        { RT_SAMPLER0_BEGIN +  0, 0, ENUM_SAMPLER, "Address Mode U" },
        { RT_SAMPLER0_BEGIN +  1, 0, ENUM_SAMPLER, "Address Mode V" },
        { RT_SAMPLER0_BEGIN +  2, 0, ENUM_SAMPLER, "Address Mode W" },
        { RT_SAMPLER0_BEGIN +  3, 0, Dissector::RSTF_VISUALIZE_HEX, "Border Color" },
        { RT_SAMPLER0_BEGIN +  4, 0, ENUM_FILTER, "Magnification Filter" },
        { RT_SAMPLER0_BEGIN +  5, 0, ENUM_FILTER, "Minification Filter" },
        { RT_SAMPLER0_BEGIN +  6, 0, ENUM_FILTER, "Mip Filter" },
        { RT_SAMPLER0_BEGIN +  7, 0, Dissector::RSTF_VISUALIZE_FLOAT, "MipMap LOD Bias" },
        { RT_SAMPLER0_BEGIN +  8, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Mip Level" },
        { RT_SAMPLER0_BEGIN +  9, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Anisotropy" },
        { RT_SAMPLER0_BEGIN + 10, 0, Dissector::RSTF_VISUALIZE_BOOL, "SRGB Enabled" },
        { RT_SAMPLER0_BEGIN + 11, 0, Dissector::RSTF_VISUALIZE_UINT, "Element Index" },
        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Sampler 0" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Sampler 1" },
        { RT_SAMPLER1_BEGIN +  0, 0, ENUM_SAMPLER, "Address Mode U" },
        { RT_SAMPLER1_BEGIN +  1, 0, ENUM_SAMPLER, "Address Mode V" },
        { RT_SAMPLER1_BEGIN +  2, 0, ENUM_SAMPLER, "Address Mode W" },
        { RT_SAMPLER1_BEGIN +  3, 0, Dissector::RSTF_VISUALIZE_HEX, "Border Color" },
        { RT_SAMPLER1_BEGIN +  4, 0, ENUM_FILTER, "Magnification Filter" },
        { RT_SAMPLER1_BEGIN +  5, 0, ENUM_FILTER, "Minification Filter" },
        { RT_SAMPLER1_BEGIN +  6, 0, ENUM_FILTER, "Mip Filter" },
        { RT_SAMPLER1_BEGIN +  7, 0, Dissector::RSTF_VISUALIZE_FLOAT, "MipMap LOD Bias" },
        { RT_SAMPLER1_BEGIN +  8, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Mip Level" },
        { RT_SAMPLER1_BEGIN +  9, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Anisotropy" },
        { RT_SAMPLER1_BEGIN + 10, 0, Dissector::RSTF_VISUALIZE_BOOL, "SRGB Enabled" },
        { RT_SAMPLER1_BEGIN + 11, 0, Dissector::RSTF_VISUALIZE_UINT, "Element Index" },
        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Sampler 1" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Sampler 2" },
        { RT_SAMPLER2_BEGIN +  0, 0, ENUM_SAMPLER, "Address Mode U" },
        { RT_SAMPLER2_BEGIN +  1, 0, ENUM_SAMPLER, "Address Mode V" },
        { RT_SAMPLER2_BEGIN +  2, 0, ENUM_SAMPLER, "Address Mode W" },
        { RT_SAMPLER2_BEGIN +  3, 0, Dissector::RSTF_VISUALIZE_HEX, "Border Color" },
        { RT_SAMPLER2_BEGIN +  4, 0, ENUM_FILTER, "Magnification Filter" },
        { RT_SAMPLER2_BEGIN +  5, 0, ENUM_FILTER, "Minification Filter" },
        { RT_SAMPLER2_BEGIN +  6, 0, ENUM_FILTER, "Mip Filter" },
        { RT_SAMPLER2_BEGIN +  7, 0, Dissector::RSTF_VISUALIZE_FLOAT, "MipMap LOD Bias" },
        { RT_SAMPLER2_BEGIN +  8, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Mip Level" },
        { RT_SAMPLER2_BEGIN +  9, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Anisotropy" },
        { RT_SAMPLER2_BEGIN + 10, 0, Dissector::RSTF_VISUALIZE_BOOL, "SRGB Enabled" },
        { RT_SAMPLER2_BEGIN + 11, 0, Dissector::RSTF_VISUALIZE_UINT, "Element Index" },
        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Sampler 2" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Sampler 3" },
        { RT_SAMPLER3_BEGIN +  0, 0, ENUM_SAMPLER, "Address Mode U" },
        { RT_SAMPLER3_BEGIN +  1, 0, ENUM_SAMPLER, "Address Mode V" },
        { RT_SAMPLER3_BEGIN +  2, 0, ENUM_SAMPLER, "Address Mode W" },
        { RT_SAMPLER3_BEGIN +  3, 0, Dissector::RSTF_VISUALIZE_HEX, "Border Color" },
        { RT_SAMPLER3_BEGIN +  4, 0, ENUM_FILTER, "Magnification Filter" },
        { RT_SAMPLER3_BEGIN +  5, 0, ENUM_FILTER, "Minification Filter" },
        { RT_SAMPLER3_BEGIN +  6, 0, ENUM_FILTER, "Mip Filter" },
        { RT_SAMPLER3_BEGIN +  7, 0, Dissector::RSTF_VISUALIZE_FLOAT, "MipMap LOD Bias" },
        { RT_SAMPLER3_BEGIN +  8, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Mip Level" },
        { RT_SAMPLER3_BEGIN +  9, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Anisotropy" },
        { RT_SAMPLER3_BEGIN + 10, 0, Dissector::RSTF_VISUALIZE_BOOL, "SRGB Enabled" },
        { RT_SAMPLER3_BEGIN + 11, 0, Dissector::RSTF_VISUALIZE_UINT, "Element Index" },
        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Sampler 3" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Sampler 4" },
        { RT_SAMPLER4_BEGIN +  0, 0, ENUM_SAMPLER, "Address Mode U" },
        { RT_SAMPLER4_BEGIN +  1, 0, ENUM_SAMPLER, "Address Mode V" },
        { RT_SAMPLER4_BEGIN +  2, 0, ENUM_SAMPLER, "Address Mode W" },
        { RT_SAMPLER4_BEGIN +  3, 0, Dissector::RSTF_VISUALIZE_HEX, "Border Color" },
        { RT_SAMPLER4_BEGIN +  4, 0, ENUM_FILTER, "Magnification Filter" },
        { RT_SAMPLER4_BEGIN +  5, 0, ENUM_FILTER, "Minification Filter" },
        { RT_SAMPLER4_BEGIN +  6, 0, ENUM_FILTER, "Mip Filter" },
        { RT_SAMPLER4_BEGIN +  7, 0, Dissector::RSTF_VISUALIZE_FLOAT, "MipMap LOD Bias" },
        { RT_SAMPLER4_BEGIN +  8, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Mip Level" },
        { RT_SAMPLER4_BEGIN +  9, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Anisotropy" },
        { RT_SAMPLER4_BEGIN + 10, 0, Dissector::RSTF_VISUALIZE_BOOL, "SRGB Enabled" },
        { RT_SAMPLER4_BEGIN + 11, 0, Dissector::RSTF_VISUALIZE_UINT, "Element Index" },
        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Sampler 4" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Sampler 5" },
        { RT_SAMPLER5_BEGIN +  0, 0, ENUM_SAMPLER, "Address Mode U" },
        { RT_SAMPLER5_BEGIN +  1, 0, ENUM_SAMPLER, "Address Mode V" },
        { RT_SAMPLER5_BEGIN +  2, 0, ENUM_SAMPLER, "Address Mode W" },
        { RT_SAMPLER5_BEGIN +  3, 0, Dissector::RSTF_VISUALIZE_HEX, "Border Color" },
        { RT_SAMPLER5_BEGIN +  4, 0, ENUM_FILTER, "Magnification Filter" },
        { RT_SAMPLER5_BEGIN +  5, 0, ENUM_FILTER, "Minification Filter" },
        { RT_SAMPLER5_BEGIN +  6, 0, ENUM_FILTER, "Mip Filter" },
        { RT_SAMPLER5_BEGIN +  7, 0, Dissector::RSTF_VISUALIZE_FLOAT, "MipMap LOD Bias" },
        { RT_SAMPLER5_BEGIN +  8, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Mip Level" },
        { RT_SAMPLER5_BEGIN +  9, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Anisotropy" },
        { RT_SAMPLER5_BEGIN + 10, 0, Dissector::RSTF_VISUALIZE_BOOL, "SRGB Enabled" },
        { RT_SAMPLER5_BEGIN + 11, 0, Dissector::RSTF_VISUALIZE_UINT, "Element Index" },
        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Sampler 5" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Sampler 6" },
        { RT_SAMPLER6_BEGIN +  0, 0, ENUM_SAMPLER, "Address Mode U" },
        { RT_SAMPLER6_BEGIN +  1, 0, ENUM_SAMPLER, "Address Mode V" },
        { RT_SAMPLER6_BEGIN +  2, 0, ENUM_SAMPLER, "Address Mode W" },
        { RT_SAMPLER6_BEGIN +  3, 0, Dissector::RSTF_VISUALIZE_HEX, "Border Color" },
        { RT_SAMPLER6_BEGIN +  4, 0, ENUM_FILTER, "Magnification Filter" },
        { RT_SAMPLER6_BEGIN +  5, 0, ENUM_FILTER, "Minification Filter" },
        { RT_SAMPLER6_BEGIN +  6, 0, ENUM_FILTER, "Mip Filter" },
        { RT_SAMPLER6_BEGIN +  7, 0, Dissector::RSTF_VISUALIZE_FLOAT, "MipMap LOD Bias" },
        { RT_SAMPLER6_BEGIN +  8, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Mip Level" },
        { RT_SAMPLER6_BEGIN +  9, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Anisotropy" },
        { RT_SAMPLER6_BEGIN + 10, 0, Dissector::RSTF_VISUALIZE_BOOL, "SRGB Enabled" },
        { RT_SAMPLER6_BEGIN + 11, 0, Dissector::RSTF_VISUALIZE_UINT, "Element Index" },
        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Sampler 6" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Sampler 7" },
        { RT_SAMPLER7_BEGIN +  0, 0, ENUM_SAMPLER, "Address Mode U" },
        { RT_SAMPLER7_BEGIN +  1, 0, ENUM_SAMPLER, "Address Mode V" },
        { RT_SAMPLER7_BEGIN +  2, 0, ENUM_SAMPLER, "Address Mode W" },
        { RT_SAMPLER7_BEGIN +  3, 0, Dissector::RSTF_VISUALIZE_HEX, "Border Color" },
        { RT_SAMPLER7_BEGIN +  4, 0, ENUM_FILTER, "Magnification Filter" },
        { RT_SAMPLER7_BEGIN +  5, 0, ENUM_FILTER, "Minification Filter" },
        { RT_SAMPLER7_BEGIN +  6, 0, ENUM_FILTER, "Mip Filter" },
        { RT_SAMPLER7_BEGIN +  7, 0, Dissector::RSTF_VISUALIZE_FLOAT, "MipMap LOD Bias" },
        { RT_SAMPLER7_BEGIN +  8, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Mip Level" },
        { RT_SAMPLER7_BEGIN +  9, 0, Dissector::RSTF_VISUALIZE_UINT, "Max Anisotropy" },
        { RT_SAMPLER7_BEGIN + 10, 0, Dissector::RSTF_VISUALIZE_BOOL, "SRGB Enabled" },
        { RT_SAMPLER7_BEGIN + 11, 0, Dissector::RSTF_VISUALIZE_UINT, "Element Index" },
        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Sampler 7" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Vertex Shader Constants" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   0, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "0" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   1, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "1" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   2, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "2" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   3, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "3" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   4, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "4" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   5, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "5" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   6, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "6" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   7, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "7" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   8, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "8" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +   9, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "9" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  10, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "10" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  11, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "11" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  12, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "12" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  13, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "13" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  14, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "14" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  15, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "15" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  16, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "16" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  17, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "17" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  18, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "18" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  19, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "19" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  20, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "20" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  21, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "21" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  22, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "22" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  23, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "23" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  24, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "24" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  25, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "25" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  26, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "26" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  27, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "27" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  28, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "28" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  29, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "29" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  30, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "30" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  31, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "31" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  32, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "32" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  33, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "33" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  34, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "34" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  35, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "35" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  36, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "36" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  37, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "37" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  38, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "38" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  39, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "39" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  40, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "40" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  41, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "41" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  42, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "42" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  43, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "43" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  44, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "44" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  45, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "45" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  46, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "46" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  47, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "47" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  48, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "48" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  49, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "49" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  50, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "50" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  51, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "51" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  52, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "52" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  53, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "53" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  54, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "54" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  55, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "55" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  56, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "56" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  57, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "57" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  58, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "58" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  59, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "59" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  60, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "60" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  61, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "61" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  62, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "62" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  63, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "63" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  64, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "64" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  65, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "65" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  66, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "66" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  67, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "67" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  68, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "68" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  69, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "69" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  70, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "70" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  71, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "71" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  72, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "72" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  73, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "73" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  74, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "74" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  75, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "75" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  76, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "76" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  77, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "77" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  78, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "78" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  79, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "79" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  80, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "80" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  81, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "81" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  82, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "82" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  83, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "83" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  84, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "84" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  85, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "85" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  86, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "86" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  87, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "87" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  88, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "88" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  89, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "89" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  90, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "90" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  91, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "91" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  92, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "92" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  93, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "93" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  94, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "94" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  95, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "95" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  96, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "96" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  97, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "97" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  98, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "98" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN +  99, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "99" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 100, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "100" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 101, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "101" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 102, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "102" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 103, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "103" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 104, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "104" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 105, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "105" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 106, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "106" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 107, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "107" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 108, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "108" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 109, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "109" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 110, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "110" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 111, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "111" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 112, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "112" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 113, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "113" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 114, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "114" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 115, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "115" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 116, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "116" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 117, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "117" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 118, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "118" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 119, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "119" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 120, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "120" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 121, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "121" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 122, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "122" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 123, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "123" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 124, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "124" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 125, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "125" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 126, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "126" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 127, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "127" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 128, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "128" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 129, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "129" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 130, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "130" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 131, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "131" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 132, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "132" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 133, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "133" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 134, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "134" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 135, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "135" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 136, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "136" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 137, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "137" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 138, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "138" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 139, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "139" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 140, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "140" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 141, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "141" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 142, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "142" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 143, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "143" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 144, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "144" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 145, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "145" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 146, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "146" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 147, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "147" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 148, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "148" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 149, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "149" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 150, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "150" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 151, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "151" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 152, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "152" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 153, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "153" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 154, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "154" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 155, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "155" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 156, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "156" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 157, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "157" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 158, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "158" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 159, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "159" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 160, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "160" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 161, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "161" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 162, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "162" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 163, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "163" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 164, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "164" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 165, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "165" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 166, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "166" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 167, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "167" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 168, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "168" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 169, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "169" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 170, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "170" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 171, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "171" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 172, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "172" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 173, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "173" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 174, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "174" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 175, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "175" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 176, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "176" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 177, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "177" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 178, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "178" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 179, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "179" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 180, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "180" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 181, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "181" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 182, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "182" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 183, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "183" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 184, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "184" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 185, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "185" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 186, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "186" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 187, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "187" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 188, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "188" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 189, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "189" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 190, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "190" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 191, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "191" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 192, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "192" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 193, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "193" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 194, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "194" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 195, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "195" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 196, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "196" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 197, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "197" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 198, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "198" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 199, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "199" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 200, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "200" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 201, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "201" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 202, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "202" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 203, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "203" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 204, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "204" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 205, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "205" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 206, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "206" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 207, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "207" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 208, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "208" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 209, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "209" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 210, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "210" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 211, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "211" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 212, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "212" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 213, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "213" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 214, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "214" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 215, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "215" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 216, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "216" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 217, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "217" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 218, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "218" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 219, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "219" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 220, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "220" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 221, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "221" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 222, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "222" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 223, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "223" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 224, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "224" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 225, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "225" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 226, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "226" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 227, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "227" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 228, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "228" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 229, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "229" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 230, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "230" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 231, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "231" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 232, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "232" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 233, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "233" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 234, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "234" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 235, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "235" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 236, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "236" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 237, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "237" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 238, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "238" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 239, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "239" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 240, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "240" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 241, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "241" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 242, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "242" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 243, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "243" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 244, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "244" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 245, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "245" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 246, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "246" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 247, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "247" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 248, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "248" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 249, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "249" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 250, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "250" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 251, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "251" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 252, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "252" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 253, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "253" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 254, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "254" },
        { RT_VERTEXSHADERCONSTANTF_BEGIN + 255, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "255" },

        { RT_VERTEXSHADERCONSTANTI_BEGIN +   0, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 0" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +   1, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 1" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +   2, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 2" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +   3, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 3" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +   4, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 4" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +   5, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 5" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +   6, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 6" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +   7, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 7" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +   8, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 8" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +   9, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 9" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +  10, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 10" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +  11, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 11" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +  12, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 12" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +  13, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 13" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +  14, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 14" },
        { RT_VERTEXSHADERCONSTANTI_BEGIN +  15, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 15" },

        { RT_VERTEXSHADERCONSTANTB_BEGIN +   0, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 0" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +   1, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 1" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +   2, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 2" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +   3, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 3" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +   4, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 4" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +   5, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 5" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +   6, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 6" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +   7, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 7" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +   8, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 8" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +   9, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 9" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +  10, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 10" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +  11, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 11" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +  12, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 12" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +  13, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 13" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +  14, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 14" },
        { RT_VERTEXSHADERCONSTANTB_BEGIN +  15, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 15" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Vertex Shader Constants" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_START, "Pixel Shader Constants" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   0, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "0" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   1, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "1" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   2, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "2" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   3, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "3" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   4, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "4" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   5, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "5" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   6, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "6" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   7, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "7" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   8, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "8" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +   9, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "9" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  10, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "10" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  11, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "11" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  12, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "12" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  13, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "13" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  14, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "14" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  15, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "15" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  16, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "16" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  17, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "17" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  18, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "18" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  19, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "19" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  20, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "20" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  21, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "21" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  22, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "22" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  23, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "23" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  24, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "24" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  25, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "25" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  26, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "26" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  27, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "27" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  28, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "28" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  29, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "29" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  30, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "30" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  31, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "31" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  32, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "32" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  33, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "33" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  34, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "34" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  35, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "35" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  36, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "36" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  37, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "37" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  38, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "38" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  39, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "39" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  40, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "40" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  41, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "41" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  42, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "42" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  43, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "43" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  44, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "44" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  45, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "45" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  46, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "46" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  47, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "47" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  48, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "48" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  49, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "49" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  50, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "50" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  51, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "51" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  52, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "52" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  53, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "53" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  54, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "54" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  55, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "55" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  56, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "56" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  57, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "57" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  58, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "58" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  59, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "59" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  60, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "60" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  61, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "61" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  62, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "62" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  63, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "63" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  64, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "64" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  65, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "65" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  66, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "66" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  67, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "67" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  68, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "68" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  69, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "69" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  70, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "70" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  71, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "71" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  72, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "72" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  73, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "73" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  74, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "74" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  75, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "75" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  76, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "76" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  77, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "77" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  78, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "78" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  79, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "79" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  80, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "80" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  81, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "81" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  82, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "82" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  83, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "83" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  84, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "84" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  85, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "85" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  86, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "86" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  87, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "87" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  88, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "88" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  89, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "89" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  90, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "90" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  91, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "91" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  92, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "92" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  93, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "93" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  94, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "94" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  95, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "95" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  96, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "96" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  97, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "97" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  98, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "98" },
        { RT_PIXELSHADERCONSTANTF_BEGIN +  99, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "99" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 100, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "100" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 101, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "101" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 102, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "102" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 103, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "103" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 104, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "104" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 105, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "105" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 106, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "106" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 107, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "107" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 108, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "108" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 109, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "109" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 110, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "110" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 111, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "111" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 112, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "112" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 113, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "113" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 114, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "114" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 115, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "115" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 116, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "116" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 117, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "117" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 118, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "118" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 119, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "119" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 120, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "120" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 121, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "121" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 122, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "122" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 123, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "123" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 124, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "124" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 125, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "125" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 126, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "126" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 127, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "127" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 128, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "128" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 129, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "129" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 130, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "130" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 131, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "131" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 132, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "132" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 133, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "133" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 134, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "134" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 135, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "135" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 136, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "136" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 137, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "137" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 138, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "138" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 139, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "139" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 140, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "140" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 141, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "141" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 142, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "142" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 143, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "143" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 144, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "144" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 145, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "145" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 146, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "146" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 147, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "147" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 148, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "148" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 149, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "149" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 150, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "150" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 151, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "151" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 152, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "152" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 153, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "153" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 154, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "154" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 155, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "155" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 156, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "156" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 157, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "157" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 158, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "158" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 159, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "159" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 160, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "160" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 161, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "161" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 162, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "162" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 163, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "163" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 164, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "164" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 165, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "165" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 166, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "166" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 167, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "167" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 168, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "168" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 169, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "169" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 170, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "170" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 171, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "171" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 172, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "172" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 173, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "173" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 174, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "174" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 175, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "175" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 176, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "176" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 177, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "177" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 178, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "178" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 179, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "179" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 180, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "180" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 181, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "181" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 182, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "182" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 183, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "183" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 184, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "184" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 185, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "185" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 186, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "186" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 187, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "187" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 188, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "188" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 189, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "189" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 190, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "190" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 191, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "191" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 192, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "192" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 193, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "193" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 194, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "194" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 195, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "195" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 196, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "196" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 197, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "197" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 198, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "198" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 199, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "199" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 200, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "200" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 201, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "201" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 202, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "202" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 203, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "203" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 204, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "204" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 205, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "205" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 206, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "206" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 207, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "207" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 208, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "208" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 209, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "209" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 210, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "210" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 211, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "211" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 212, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "212" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 213, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "213" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 214, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "214" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 215, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "215" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 216, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "216" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 217, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "217" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 218, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "218" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 219, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "219" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 220, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "220" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 221, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "221" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 222, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "222" },
        { RT_PIXELSHADERCONSTANTF_BEGIN + 223, 0, Dissector::RSTF_VISUALIZE_FLOAT4, "223" },

        { RT_PIXELSHADERCONSTANTI_BEGIN +   0, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 0" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +   1, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 1" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +   2, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 2" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +   3, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 3" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +   4, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 4" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +   5, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 5" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +   6, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 6" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +   7, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 7" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +   8, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 8" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +   9, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 9" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +  10, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 10" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +  11, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 11" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +  12, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 12" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +  13, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 13" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +  14, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 14" },
        { RT_PIXELSHADERCONSTANTI_BEGIN +  15, 0, Dissector::RSTF_VISUALIZE_UINT4, "Int 15" },

        { RT_PIXELSHADERCONSTANTB_BEGIN +   0, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 0" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +   1, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 1" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +   2, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 2" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +   3, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 3" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +   4, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 4" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +   5, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 5" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +   6, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 6" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +   7, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 7" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +   8, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 8" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +   9, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 9" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +  10, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 10" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +  11, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 11" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +  12, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 12" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +  13, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 13" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +  14, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 14" },
        { RT_PIXELSHADERCONSTANTB_BEGIN +  15, 0, Dissector::RSTF_VISUALIZE_BOOL4, "Bool 15" },

        { -1,                     0, Dissector::RSTF_VISUALIZE_GROUP_END, "Pixel Shader Constants" },

        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_START, "Stencil" },
        { RT_RS_STENCILENABLE,              0, Dissector::RSTF_VISUALIZE_BOOL, "Enable Stencil" },        // D3DRS_STENCILENABLE,
        { RT_RS_STENCILFAIL,                0, ENUM_STENCILOP,                 "Stencil Fail Op" },       // D3DRS_STENCILFAIL,
        { RT_RS_STENCILZFAIL,               0, ENUM_STENCILOP,                 "Stencil Z-Fail Op" },     // D3DRS_STENCILZFAIL,
        { RT_RS_STENCILPASS,                0, ENUM_STENCILOP,                 "Stencil Pass Op" },       // D3DRS_STENCILPASS,
        { RT_RS_STENCILFUNC,                0, ENUM_CMP,                       "Stencil Function" },      // D3DRS_STENCILFUNC,
        { RT_RS_STENCILREF,                 0, Dissector::RSTF_VISUALIZE_HEX,  "Stencil Ref Value" },     // D3DRS_STENCILREF,
        { RT_RS_STENCILMASK,                0, Dissector::RSTF_VISUALIZE_HEX,  "Stencil Mask" },          // D3DRS_STENCILMASK,
        { RT_RS_STENCILWRITEMASK,           0, Dissector::RSTF_VISUALIZE_HEX,  "Stencil Write Mask" },    // D3DRS_STENCILWRITEMASK,
        { RT_RS_TWOSIDEDSTENCILMODE,        0, Dissector::RSTF_VISUALIZE_BOOL, "Two Sided Stencil" },     // D3DRS_TWOSIDEDSTENCILMODE,
        { RT_RS_CCW_STENCILFAIL,            0, ENUM_STENCILOP,                 "CCW Stencil Fail Op" },   // D3DRS_CCW_STENCILFAIL,
        { RT_RS_CCW_STENCILZFAIL,           0, ENUM_STENCILOP,                 "CCW Stencil Z-Fail Op" }, // D3DRS_CCW_STENCILZFAIL,
        { RT_RS_CCW_STENCILPASS,            0, ENUM_STENCILOP,                 "CCW Stencil Pass Op" },   // D3DRS_CCW_STENCILPASS,
        { RT_RS_CCW_STENCILFUNC,            0, ENUM_CMP,                       "CCW Stencil Func" },      // D3DRS_CCW_STENCILFUNC,
        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_END,   "Stencil" },

        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_START, "Blending" },
        { RT_RS_ALPHABLENDENABLE,           0, Dissector::RSTF_VISUALIZE_BOOL,  "Alpha Blend Enable" },  // D3DRS_ALPHABLENDENABLE,
        { RT_RS_SRCBLEND,                   0, ENUM_BLEND,                      "Source Blend" },        // D3DRS_SRCBLEND,
        { RT_RS_DESTBLEND,                  0, ENUM_BLEND,                      "Destination Blend" },   // D3DRS_DESTBLEND,
        { RT_RS_BLENDOP,                    0, ENUM_BLENDOP,                    "Blend Op" },            // D3DRS_BLENDOP,
        { RT_RS_SEPARATEALPHABLENDENABLE,   0, Dissector::RSTF_VISUALIZE_BOOL,  "Seperate Alpha Blend" },// D3DRS_SEPARATEALPHABLENDENABLE,
        { RT_RS_SRCBLENDALPHA,              0, ENUM_BLEND,                      "Alpha Source Blend" },  // D3DRS_SRCBLENDALPHA,
        { RT_RS_DESTBLENDALPHA,             0, ENUM_BLEND,                      "Alpha Dest Blend" },    // D3DRS_DESTBLENDALPHA,
        { RT_RS_BLENDOPALPHA,               0, ENUM_BLENDOP,                    "Alpha Blend Op" },      // D3DRS_BLENDOPALPHA
        { RT_RS_BLENDFACTOR,                0, Dissector::RSTF_VISUALIZE_HEX, "Blend Factor" },        // D3DRS_BLENDFACTOR,
        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_END,   "Blending" },

        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_START, "Point Sprites" },
        { RT_RS_POINTSPRITEENABLE,          0, Dissector::RSTF_VISUALIZE_BOOL, "Point Sprite Enable" }, // D3DRS_POINTSPRITEENABLE,
        { RT_RS_POINTSIZE,                  0, Dissector::RSTF_VISUALIZE_FLOAT, "Point Sprite Size" }, // D3DRS_POINTSIZE,
        { RT_RS_POINTSIZE_MIN,              0, Dissector::RSTF_VISUALIZE_FLOAT, "Point Sprite Size Min" }, // D3DRS_POINTSIZE_MIN,
        { RT_RS_POINTSIZE_MAX,              0, Dissector::RSTF_VISUALIZE_FLOAT, "Point Sprite Size Max" }, // D3DRS_POINTSIZE_MAX,
        { RT_RS_POINTSCALEENABLE,           0, Dissector::RSTF_VISUALIZE_BOOL, "Point Sprite Scale Enable" }, // D3DRS_POINTSCALEENABLE,
        { RT_RS_POINTSCALE_A,               0, Dissector::RSTF_VISUALIZE_FLOAT, "Point Sprite Scale A" }, // D3DRS_POINTSCALE_A,
        { RT_RS_POINTSCALE_B,               0, Dissector::RSTF_VISUALIZE_FLOAT, "Point Sprite Scale B" }, // D3DRS_POINTSCALE_B,
        { RT_RS_POINTSCALE_C,               0, Dissector::RSTF_VISUALIZE_FLOAT, "Point Sprite Scale C" }, // D3DRS_POINTSCALE_C,
        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_END,   "Point Sprites" },

        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_START, "Alpha Test" },
        { RT_RS_ALPHATESTENABLE,            0, Dissector::RSTF_VISUALIZE_BOOL, "Alpha Test" },           // D3DRS_ALPHATESTENABLE,
        { RT_RS_ALPHAREF,                   0, Dissector::RSTF_VISUALIZE_HEX,  "Alpha Test Ref Value" }, // D3DRS_ALPHAREF,
        { RT_RS_ALPHAFUNC,                  0, ENUM_CMP,                       "Alpha Test Op" },        // D3DRS_ALPHAFUNC,
        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_END,   "Alpha Test" },

        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_START, "Depth Testing & Bias" },
        { RT_RS_ZENABLE,                    0, ENUM_ZBUFFER,                   "Z Buffer"  },    // D3DRS_ZENABLE,
        { RT_RS_ZWRITEENABLE,               0, Dissector::RSTF_VISUALIZE_BOOL, "Z Write"  },     // D3DRS_ZWRITEENABLE,
        { RT_RS_ZFUNC,                      0, ENUM_CMP,                       "Z Function"  },  // D3DRS_ZFUNC,
        { RT_RS_SLOPESCALEDEPTHBIAS,        0, Dissector::RSTF_VISUALIZE_FLOAT, "Depth Slope Scale bias" }, // D3DRS_SLOPESCALEDEPTHBIAS,
        { RT_RS_DEPTHBIAS,                  0, Dissector::RSTF_VISUALIZE_FLOAT, "Depth Bias" }, // D3DRS_DEPTHBIAS,
        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_END,   "Depth Testing" },

        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_START, "Render States" },
        { RT_RS_FILLMODE,                   0, ENUM_FILLMODE,                  "Fill Mode" }, // D3DRS_FILLMODE,
        { RT_RS_LASTPIXEL,                  0, Dissector::RSTF_VISUALIZE_BOOL, "Last Pixel" }, // D3DRS_LASTPIXEL,
        { RT_RS_TEXTUREFACTOR,              0, Dissector::RSTF_VISUALIZE_HEX, "Texture Factor" }, // D3DRS_TEXTUREFACTOR,
        { RT_RS_MULTISAMPLEANTIALIAS,       0, Dissector::RSTF_VISUALIZE_BOOL, "Enable MSAA" }, // D3DRS_MULTISAMPLEANTIALIAS,
        { RT_RS_MULTISAMPLEMASK,            0, Dissector::RSTF_VISUALIZE_HEX, "MSAA Mask" }, // D3DRS_MULTISAMPLEMASK,
        { RT_RS_ANTIALIASEDLINEENABLE,      0, Dissector::RSTF_VISUALIZE_BOOL, "Antialiased Lines" }, // D3DRS_ANTIALIASEDLINEENABLE,
        { RT_RS_SRGBWRITEENABLE,            0, Dissector::RSTF_VISUALIZE_BOOL, "SRGB Enable" }, // D3DRS_SRGBWRITEENABLE,
        { -1,                               0, Dissector::RSTF_VISUALIZE_GROUP_END, "Render States" },
    };

    // ------------------------------------------------------------------------
    // D3D resource initialization
    // ------------------------------------------------------------------------
    void InitializeD3DResources( IDirect3DDevice9* iD3DDevice )
    {
        HRESULT res = iD3DDevice->CreatePixelShader( (DWORD*)sSingleColorShaderCode, &sDX9Data.mSingleColorShader );
        assert( res == D3D_OK );


        // Create vertex buffer/format for fullscreen quad
        D3DVERTEXELEMENT9 dwDecl[] = 
        {
            {0, 0, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
            {0, 8, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END()
        };

        res = iD3DDevice->CreateVertexDeclaration( dwDecl, &sDX9Data.mPosVert );
        assert( res == S_OK );

        res = iD3DDevice->CreateVertexBuffer( sizeof(float)*4*4, D3DUSAGE_WRITEONLY, 0,
            D3DPOOL_DEFAULT, &sDX9Data.mFullscreenQuadVerts, NULL );
        assert( res == S_OK );

        float* vertData = NULL;
        res = sDX9Data.mFullscreenQuadVerts->Lock( 0, 0, (void**)&vertData, 0 );
        assert( res == S_OK && vertData );

        *(vertData++) = -1.f;
        *(vertData++) = -1.f;
        *(vertData++) = 0.f;
        *(vertData++) = 1.f;

        *(vertData++) = -1.f;
        *(vertData++) = 1.f;
        *(vertData++) = 0.f;
        *(vertData++) = 0.f;

        *(vertData++) = 1.f;
        *(vertData++) = -1.f;
        *(vertData++) = 1.f;
        *(vertData++) = 1.f;

        *(vertData++) = 1.f;
        *(vertData++) = 1.f;
        *(vertData++) = 1.f;
        *(vertData++) = 0.f;

        res = sDX9Data.mFullscreenQuadVerts->Unlock();
        assert( res == S_OK );

        res = iD3DDevice->CreateVertexShader( (const DWORD*)sPosTexVSRaw, &sDX9Data.mPosTexVS );
        assert( res == S_OK );

        res = iD3DDevice->CreatePixelShader( (const DWORD*)sSingleTexPSRaw, &sDX9Data.mTexPS );
        assert( res == S_OK );

        res = iD3DDevice->CreatePixelShader( (const DWORD*)sVSDebugPSRaw, &sDX9Data.mVSDebugPS );
        assert( res == S_OK );

        iD3DDevice->CreateQuery(D3DQUERYTYPE_EVENT, &sDX9Data.mTimingQuery);
        iD3DDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &sDX9Data.mOcclusionQuery);
        iD3DDevice->CreateQuery( D3DQUERYTYPE_BANDWIDTHTIMINGS, &sDX9Data.mBandwidthTimingsQuery );
        iD3DDevice->CreateQuery( D3DQUERYTYPE_CACHEUTILIZATION, &sDX9Data.mCacheUtilizationQuery );
        iD3DDevice->CreateQuery( D3DQUERYTYPE_PIPELINETIMINGS , &sDX9Data.mPipelineTimingsQuery );
        iD3DDevice->CreateQuery( D3DQUERYTYPE_VCACHE          , &sDX9Data.mVcacheQuery );
        iD3DDevice->CreateQuery( D3DQUERYTYPE_VERTEXSTATS     , &sDX9Data.mVertexStatsQuery );
        iD3DDevice->CreateQuery( D3DQUERYTYPE_VERTEXTIMINGS   , &sDX9Data.mVertexTimingsQuery );
        iD3DDevice->CreateQuery( D3DQUERYTYPE_PIXELTIMINGS    , &sDX9Data.mPixelTimingsQuery );
    }

    void CleanDepthBufferReplacements()
    {
        for( DepthBufferReplacement* iter = sDX9Data.mDepthBuffers; iter; )
        {
            DepthBufferReplacement* delPtr = iter;
            iter = iter->mNext;
            ULONG ref = delPtr->mReplacement->Release();
            if( delPtr->mTexture )
                ref = delPtr->mTexture->Release();
            Dissector::FreeCallback( delPtr );
        }

        sDX9Data.mDepthBuffers = NULL;
    }

    void CleanRTCopies()
    {
        for( RenderTargetData* rt = sDX9Data.mRTCopies; rt != NULL; )
        {
            rt->mSurfaceCopy->Release();
            RenderTargetData* oldrt = rt;
            rt = rt->mNext;
            Dissector::FreeCallback( oldrt );
        }

        sDX9Data.mRTCopies = NULL;
    }

    void ReleaseAssetHandles()
    {
        if( sDX9Data.mAssetHandles ) 
        {
            for( unsigned int ii = 0; ii < sDX9Data.mAssetHandlesCount; ++ii )
            {
                sDX9Data.mAssetHandles[ii]->Release();
            }
            Dissector::FreeCallback( sDX9Data.mAssetHandles );
            sDX9Data.mAssetHandles = NULL;
            sDX9Data.mAssetHandlesCount = 0;
            sDX9Data.mAssetHandlesSize = 0;
        }
    }

    void DestroyD3DResources()
    {
        ReleaseAssetHandles();

        SAFE_RELEASE( sDX9Data.mTimingQuery );
        SAFE_RELEASE( sDX9Data.mOcclusionQuery );
        SAFE_RELEASE( sDX9Data.mBandwidthTimingsQuery );
        SAFE_RELEASE( sDX9Data.mCacheUtilizationQuery );
        SAFE_RELEASE( sDX9Data.mPipelineTimingsQuery );
        SAFE_RELEASE( sDX9Data.mVcacheQuery );
        SAFE_RELEASE( sDX9Data.mVertexStatsQuery );
        SAFE_RELEASE( sDX9Data.mVertexTimingsQuery );
        SAFE_RELEASE( sDX9Data.mPixelTimingsQuery );
        SAFE_RELEASE( sDX9Data.mSingleColorShader );
        SAFE_RELEASE( sDX9Data.mFullscreenQuadVerts );
        SAFE_RELEASE( sDX9Data.mPosTexVS );
        SAFE_RELEASE( sDX9Data.mTexPS );
        SAFE_RELEASE( sDX9Data.mVSDebugPS );
        SAFE_RELEASE( sDX9Data.mPosVert );

        CleanDepthBufferReplacements();
        CleanRTCopies();
    }

    // ------------------------------------------------------------------------
    // Callback functions
    // ------------------------------------------------------------------------
    void OutputDebugStep( int iFile, int iLine, unsigned int iStart, unsigned int iEnd, 
                          ShaderDebugDX9::ShaderInfo& info, RegisterData* regPtr, unsigned int regSize,
                          char*& iter )
    {
        bool debuggable;
        unsigned int startOp = info.ConvertInstructionNumToOpNum( iStart, &debuggable );
        unsigned int endOp = info.ConvertInstructionNumToOpNum( iEnd, &debuggable );

        // output a debug step
        Dissector::ShaderDebugStep step;
        step.mFilename = iFile;
        step.mLine = iLine;
        step.mNumUpdates = 0;
        StoreBufferData( step, iter );
        DWORD* numUpdatesPtr = (DWORD*)( iter - sizeof(DWORD) );

        DWORD numUpdates = 0;
        for( unsigned int jj = 0; jj < info.mDebugHeader->numVariableEntries; ++jj )
        {
            ShaderDebugDX9::VariableEntry& entry = info.GetVariableEntries()[jj];
            unsigned int blockOffset = entry.offsetToFunctionName;
            ShaderDebugDX9::VariableInstructionEntry* instr =
                info.DebugOffsetToPointer<ShaderDebugDX9::VariableInstructionEntry*>(
                entry.offsetToInstructionEntries );

            for( unsigned int kk = 0; kk < entry.numInstructionEntries; ++kk )
            {
                if( instr[kk].opNum >= startOp && 
                    instr[kk].opNum < endOp )   
                {
                    Dissector::ShaderDebugVariableUpdate update;
                    update.mDataBlockOffset = blockOffset;
                    for( int mm = 0; mm < 4; ++mm )
                        update.mUpdateOffsets[mm] = instr[kk].offsetIds[mm];

                    // Find the register data that corresponds to the instruction id
                    for( unsigned int mm = 0; mm < regSize; ++mm )
                    {
                        if( regPtr[mm].mOpNum == instr[kk].opNum &&
                            regPtr[mm].mInstructionNum >= iStart && regPtr[mm].mInstructionNum < iEnd )
                        {
                            for( int nn = 0; nn < 4; ++nn )
                            {
                                update.mDataInt[nn] = regPtr[mm].mInt[nn];
                            }
                            StoreBufferData( update, iter );
                            ++numUpdates;
                            break;
                        }
                    }
                }
            }
        }

        *numUpdatesPtr = numUpdates;
    }

    bool DebugShader(void* iDevice, int iEventId, const Dissector::DrawCallData& iData, unsigned int iShaderType,
                          char* iDebugData, unsigned int iDataSize, bool iPixelShader )
    {
        bool validResults = false;
        unsigned int pixelX = 0;
        unsigned int pixelY = 0;
        unsigned int vertNum = 0;
        if( iPixelShader )
        {
            pixelX = *(unsigned int*)iDebugData;
            pixelY = *(unsigned int*)(iDebugData + sizeof(unsigned int));
        }
        else
        {
            vertNum = *(unsigned int*)iDebugData;
            pixelX = vertNum;
        }

        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
        SetRenderStates( D3DDevice, &iData );

        HRESULT res;

        bool rvalue = true;
        UINT shaderSize = 0;
        UINT shaderDataPatchedSize = 0;

        ScopedFree<RegisterData> regPtr;
        ScopedFree<void> shaderData;
        ScopedFree<void> shaderDataPatched;

        ScopedRelease<IDirect3DSurface9> curTarget, newTarget, instrTarget;

        if( iPixelShader )
            shaderData = GetPixelShaderByteCode( D3DDevice, shaderSize );
        else
            shaderData = GetVertexShaderByteCode( D3DDevice, shaderSize );

        if( !shaderData )
        {
            rvalue = false;
            goto DebugShaderCleanup;
        }

        ShaderDebugDX9::ShaderInfo info;
        D3DSURFACE_DESC curDesc;

        if( iPixelShader )
        {
            if( (res = D3DDevice->GetRenderTarget( 0, &curTarget.mPtr ) ) != D3D_OK )
            {
                rvalue = false;
                goto DebugShaderCleanup;
            }
            curTarget->GetDesc( &curDesc );

            if( ( res = D3DDevice->CreateRenderTarget( curDesc.Width, curDesc.Height, D3DFMT_A32B32G32R32F,
                                           D3DMULTISAMPLE_NONE, 0, true, &newTarget.mPtr, NULL ) ) != D3D_OK )
            {
                rvalue = false;
                goto DebugShaderCleanup;
            }
            if( ( res = D3DDevice->CreateRenderTarget( curDesc.Width, curDesc.Height, D3DFMT_A32B32G32R32F,
                                           D3DMULTISAMPLE_NONE, 0, true, &instrTarget.mPtr, NULL ) ) != D3D_OK )
            {
                rvalue = false;
                goto DebugShaderCleanup;
            }
        }
        else
        {
            if( ( res = D3DDevice->CreateRenderTarget( 1, 1, D3DFMT_A32B32G32R32F,
                                           D3DMULTISAMPLE_NONE, 0, true, &newTarget.mPtr, NULL ) ) != D3D_OK )
            {
                rvalue = false;
                goto DebugShaderCleanup;
            }
            if( ( res = D3DDevice->CreateRenderTarget( 1, 1, D3DFMT_A32B32G32R32F,
                                           D3DMULTISAMPLE_NONE, 0, true, &instrTarget.mPtr, NULL ) ) != D3D_OK )
            {
                rvalue = false;
                goto DebugShaderCleanup;
            }
        }

        // Patch the shader to debug the instruction
        shaderDataPatched = Dissector::MallocCallback( shaderSize + ShaderDebugDX9::ShaderPatchSize );
        if( !shaderDataPatched )
        {
            rvalue = false;
            goto DebugShaderCleanup;
        }

        info = ShaderDebugDX9::GetShaderInfo( shaderData, shaderSize, D3DDevice );
        if (!info.mDebugHeader)
        {
            rvalue = false;
            goto DebugShaderCleanup;
        }

        regPtr = (RegisterData*)Dissector::MallocCallback( sizeof(RegisterData) * info.mInstructionCount );
        unsigned int regSize = 0;

        // Setup device states
        D3DDevice->SetRenderTarget( 0, instrTarget );
        D3DDevice->SetRenderTarget( 1, newTarget );
        ResetDevice( D3DDevice ); // Reset debug states to make sure the pixel is written

        // Debug area to lock
        RECT area;
        if( iPixelShader )
        {
            area.left = pixelX;
            area.top = pixelY;
            area.right = pixelX + 1;
            area.bottom = pixelY + 1;
        }
        else
        {
            area.left = 0;
            area.top = 0;
            area.right = 1;
            area.bottom = 1;
        }

        int lastFile = -1;
        int lastLine = -1;
        Dissector::ShaderDebugHeader blobHeader;
        blobHeader.mNumSteps = 1;

        for( unsigned int ii = 0; ii < info.mInstructionCount; ++ii )
        {
            ScopedRelease<IDirect3DPixelShader9> debugPixelShader;
            ScopedRelease<IDirect3DVertexShader9> debugVertShader;

            unsigned char outputMask = 0;
            unsigned int opNum;
            shaderDataPatchedSize = shaderSize + ShaderDebugDX9::ShaderPatchSize;
            if( !ShaderDebugDX9::PatchShaderForDebug( ii, &info, 0, shaderData, shaderSize,
                shaderDataPatched, shaderDataPatchedSize, outputMask, &opNum ) )
            {
                continue;
            }

            if( iPixelShader )
            {
                if( (res = D3DDevice->CreatePixelShader( (const DWORD*)shaderDataPatched.mPtr, &debugPixelShader.mPtr )) !=
                    D3D_OK || !debugPixelShader )
                {
                    continue;
                }
                D3DDevice->SetPixelShader( debugPixelShader );
                ExecuteEvent( D3DDevice, iData );
            }
            else
            {
                if( (res = D3DDevice->CreateVertexShader( (const DWORD*)shaderDataPatched.mPtr, &debugVertShader.mPtr )) != 
                    D3D_OK || !debugVertShader )
                {
                    continue;
                }

                D3DDevice->SetVertexShader( debugVertShader );
                D3DDevice->SetPixelShader( sDX9Data.mVSDebugPS );
                D3DDevice->DrawPrimitive( D3DPT_POINTLIST, vertNum, 1 );
            }

            D3DLOCKED_RECT lrect;
            res = newTarget->LockRect( &lrect, &area, NULL );
            if( res == S_OK && lrect.pBits )
            {
                regPtr[regSize].mOpNum = opNum;
                regPtr[regSize].mInstructionNum = ii;
                regPtr[regSize].mFlags = RegisterData::FLAG_ISFLOAT;
                regPtr[regSize].mRegisterMask = outputMask;
                regPtr[regSize].mFloat[0] = ((float*)lrect.pBits)[0];
                regPtr[regSize].mFloat[1] = ((float*)lrect.pBits)[1];
                regPtr[regSize].mFloat[2] = ((float*)lrect.pBits)[2];
                regPtr[regSize].mFloat[3] = ((float*)lrect.pBits)[3];

                newTarget->UnlockRect();
            }

            res = instrTarget->LockRect( &lrect, &area, NULL );
            if( res == S_OK && lrect.pBits &&
                ((float*)lrect.pBits)[0] == float(ii) &&
                ((float*)lrect.pBits)[1] == 2.f &&
                ((float*)lrect.pBits)[2] == 1.f &&
                ((float*)lrect.pBits)[3] == 0.f
                )
            {
                regPtr[regSize].mFlags |= RegisterData::FLAG_ISVALID;
                validResults = true;
            }

            instrTarget->UnlockRect();
            regSize++;
        }


        blobHeader.mNumFilenames = info.mDebugHeader->countOfFilenames;
        // Allocate the space for the debug data blob

        unsigned int sizeofFilenames = 0;
        for( int ii = 0; ii < blobHeader.mNumFilenames; ++ii )
        {
            char* str = info.GetFilename( ii );
            sizeofFilenames += (unsigned int)strlen( str );
            sizeofFilenames += (unsigned int)sizeof(int); // for the leading size of the string
        }

        blobHeader.mNumVariables = 0;
        unsigned int sizeofVariableNames = 0;
        blobHeader.mVariableBlockSize = 0;

        for( unsigned int ii = 0; ii < info.mDebugHeader->numVariableEntries; ++ii )
        {
            ShaderDebugDX9::VariableEntry& entry = info.GetVariableEntries()[ii];
            char* name = info.DebugOffsetToPointer< char* >( entry.offsetToName );
            _D3DXSHADER_TYPEINFO* typeInfo = 
                info.DebugOffsetToPointer< _D3DXSHADER_TYPEINFO* >( entry.offsetToTypeInfo );
            
            unsigned int nameLength = (unsigned int)strlen(name);
            if( typeInfo->StructMembers )
            {
                blobHeader.mNumVariables += typeInfo->StructMembers;
                _D3DXSHADER_STRUCTMEMBERINFO* memInfo = 
                    info.DebugOffsetToPointer< _D3DXSHADER_STRUCTMEMBERINFO* >( typeInfo->StructMemberInfo );
                for( int jj = 0; jj < typeInfo->StructMembers; ++jj )
                {
                    _D3DXSHADER_TYPEINFO* memtypeInfo = 
                        info.DebugOffsetToPointer< _D3DXSHADER_TYPEINFO* >( memInfo[jj].TypeInfo );

                    char* memName = info.DebugOffsetToPointer< char* >( memInfo[jj].Name );
                    sizeofVariableNames += nameLength;
                    sizeofVariableNames += (unsigned int)strlen(memName) + 1; // +1 for '.'
                    sizeofVariableNames += sizeof( int ); // for the leading size of the string
                    blobHeader.mVariableBlockSize += memtypeInfo->Columns * memtypeInfo->Rows * sizeof(float);
                }
            }
            else
            {
                blobHeader.mNumVariables++;
                sizeofVariableNames += nameLength;
                sizeofVariableNames += sizeof( int ); // for the leading size of the string
                blobHeader.mVariableBlockSize += typeInfo->Columns * typeInfo->Rows * sizeof(float);
            }
        }


        // Count up how much space we'll need for the structure.
        lastFile = -1;
        lastLine = -1;
        blobHeader.mNumSteps = 0;
        for( unsigned int ii = 0; ii < info.mInstructionCount; ++ii )
        {
            bool debuggable = false;
            unsigned int num = info.ConvertInstructionNumToOpNum( ii, &debuggable );
            if( debuggable || (ii == info.mInstructionCount-1) )
            {
                ShaderDebugDX9::InstructionDebugInfo& instrInfo = info.GetInstructionDebugInfo()[num];
                int file = int(short(instrInfo.fileNumber));
                int line = int(short(instrInfo.lineNumber));
                if( file != lastFile ||
                    (file != -1 && line != lastLine) )
                {
                    blobHeader.mNumSteps++;
                    lastFile = file;
                    lastLine = line;
                }
            }
        }
        blobHeader.mNumSteps++;

        unsigned int debugBlobSize = sizeof(blobHeader) + sizeofFilenames + 
            blobHeader.mNumVariables * sizeof(Dissector::ShaderDebugVariable) + sizeofVariableNames +
            blobHeader.mNumSteps * sizeof(Dissector::ShaderDebugStep) +
            info.mInstructionCount * sizeof(Dissector::ShaderDebugVariableUpdate);

        char* debugBlob = (char*)Dissector::MallocCallback( debugBlobSize );

        if( !debugBlob )
        {
            goto DebugShaderCleanup;
        }

        char* iter = debugBlob;
        StoreBufferData( blobHeader, iter );

        for( int ii = 0; ii < blobHeader.mNumFilenames; ++ii )
        {
            char* str = info.GetFilename( ii );
            int size = (int)strlen( str );
            StoreBufferData( size, iter );
            memcpy( iter, str, size );
            iter += size;
        }

        // Write out all the variable entries. Dissassemble structs to multiple entries.
        unsigned int blockOffset = 0;
        for( unsigned int ii = 0; ii < info.mDebugHeader->numVariableEntries; ++ii )
        {
            ShaderDebugDX9::VariableEntry& entry = info.GetVariableEntries()[ii];
            char* name = info.DebugOffsetToPointer< char* >( entry.offsetToName );
            int nameLen = (int)strlen( name );
            _D3DXSHADER_TYPEINFO* typeInfo = 
                info.DebugOffsetToPointer< _D3DXSHADER_TYPEINFO* >( entry.offsetToTypeInfo );

            entry.offsetToFunctionName = blockOffset; // overwrite this with the block offset so later on we don't have
                                                      // to recompute it when outputting debug steps.

            if( typeInfo->StructMembers )
            {
                _D3DXSHADER_STRUCTMEMBERINFO* memInfo = 
                    info.DebugOffsetToPointer< _D3DXSHADER_STRUCTMEMBERINFO* >( typeInfo->StructMemberInfo );
                for( int jj = 0; jj < typeInfo->StructMembers; ++jj )
                {
                    _D3DXSHADER_TYPEINFO* memtypeInfo = 
                        info.DebugOffsetToPointer< _D3DXSHADER_TYPEINFO* >( memInfo[jj].TypeInfo );
                    char* memName = info.DebugOffsetToPointer< char* >( memInfo[jj].Name );

                    Dissector::ShaderDebugVariable var;
                    var.mColumns = memtypeInfo->Columns;
                    var.mRows = memtypeInfo->Rows;
                    var.mType = memtypeInfo->Type;
                    var.mVariableBlockOffset = blockOffset;

                    blockOffset += (memtypeInfo->Columns * memtypeInfo->Rows) * sizeof(float);
                    StoreBufferData( var, iter );
                    int memNameLen = (int)strlen( memName );
                    int fixedNameLen = nameLen + memNameLen + 1;
                    StoreBufferData( fixedNameLen, iter );

                    memcpy( iter, name, nameLen );
                    iter += nameLen;

                    *iter = '.';
                    iter++;

                    memcpy( iter, memName, memNameLen );
                    iter += memNameLen;
                }
            }
            else
            {
                Dissector::ShaderDebugVariable var;
                var.mColumns = typeInfo->Columns;
                var.mRows = typeInfo->Rows;
                var.mType = typeInfo->Type;
                var.mVariableBlockOffset = blockOffset;

                StoreBufferData( var, iter );

                StoreBufferData( nameLen, iter );
                memcpy( iter, name, nameLen );
                iter += nameLen;

                blockOffset += (typeInfo->Columns * typeInfo->Rows) * sizeof(float);
            }
        }

        // Fill out each step information block
        lastFile = -1;
        lastLine = -1;
        bool debuggable = false;
        unsigned int instrStart = info.ConvertInstructionNumToOpNum( 0, &debuggable );
        for( unsigned int ii = 0; ii < info.mInstructionCount; ++ii )
        {
            unsigned int num = info.ConvertInstructionNumToOpNum( ii, &debuggable );
            if( debuggable || (ii == info.mInstructionCount-1) )
            {
                ShaderDebugDX9::InstructionDebugInfo& instrInfo = info.GetInstructionDebugInfo()[num];
                int file = int(short(instrInfo.fileNumber));
                int line = int(short(instrInfo.lineNumber));
                if( file != lastFile ||
                    (file != -1 && line != lastLine) )
                {
                    OutputDebugStep( lastFile, lastLine, instrStart, ii, info, regPtr, regSize, iter );
                    lastFile = file;
                    lastLine = line;
                    instrStart = ii;
                }
            }
        }
        OutputDebugStep( lastFile, lastLine, instrStart, info.mInstructionCount, info, regPtr, regSize, iter );

        if( rvalue && validResults )
        {
            Dissector::ShaderDebugDataCallback(debugBlob, (unsigned int)(iter - debugBlob), iEventId, pixelX, pixelY);
            if( debugBlob )
            {
                Dissector::FreeCallback( debugBlob );
            }
        }

DebugShaderCleanup:
        D3DDevice->SetRenderTarget( 1, NULL );

        if( !validResults || !rvalue )
        {
            Dissector::ShaderDebugFailedCallback( iEventId, pixelX, pixelY );
        }

        return rvalue;
    }

    namespace VertexTests
    {
        enum Type
        {
            // In order of appearance in pipeline
            DegenerateIndices,
            VertexInputDegenerate,
            //VertexOutputDegenerate,
            //VerticesOffScreen,
        };
    }

    bool TestPixelWrittenInternal( DWORD iTestMask, unsigned int iRTNum, void* iDevice, unsigned int iEventId,
        ShaderDebugDX9::ShaderInfo& info, Dissector::PixelLocation iPixel, float* oOutput = NULL )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        assert( 0 != (info.mOutputRegisterMask & (1 << iRTNum)) );

        BeginFrame( iDevice );

        Dissector::DrawCallData* drawIter = NULL;

        drawIter = ExecuteToEvent( iDevice, iEventId );
        if( !drawIter )
        {
            return false;
        }

        SetRenderStates( D3DDevice, drawIter );

        unsigned int removeMask = 0;

        if( iTestMask & Dissector::PixelTests::Stencil )
        {
            D3DDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
        }
        if( iTestMask & Dissector::PixelTests::DepthTest )
        {
            D3DDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
        }
        if( iTestMask & Dissector::PixelTests::Scissor )
        {
            D3DDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
        }
        if( iTestMask & Dissector::PixelTests::ClipPlanes )
        {
            D3DDevice->SetRenderState( D3DRS_CLIPPLANEENABLE, FALSE );
        }
        if( iTestMask & Dissector::PixelTests::AlphaTest )
        {
            D3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
        }
        if( iTestMask & Dissector::PixelTests::ShaderClips )
        {
            removeMask = ShaderDebugDX9::InstructionTypes::Clip;
        }
        if( iTestMask & Dissector::PixelTests::TriangleCulled )
        {
            D3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
        }

        float outputValue[4];
        bool passed = GetPixelShaderInstructionOutput( iDevice, iPixel, drawIter,
            info.mOutputInstruction[iRTNum], true, removeMask, oOutput ? oOutput : outputValue );

        EndFrame( iDevice );

        return passed;
    }

    // ------------------------------------------------------------------------
    // Drawing commands
    // ------------------------------------------------------------------------
    void InternalCheckRTUsage( IDirect3DDevice9* iD3DDevice )
    {
        for( int ii = 0; ii < 4; ++ii )
        {
            ScopedRelease<IDirect3DSurface9> surface;
            iD3DDevice->GetRenderTarget( ii, &surface.mPtr );
            if( surface )
            {
                RenderTargetData* rt = sDX9Data.mRTCopies;
                bool found = false;
                while( rt )
                {
                    if( rt->mSurface == surface.mPtr )
                    {
                        found = true;
                        break;
                    }
                    rt = rt->mNext;
                }

                if( !found )
                {
                    rt = (RenderTargetData*)Dissector::MallocCallback( sizeof(RenderTargetData) );
                    rt->mSurface = surface.mPtr;
                    rt->mSurfaceCopy = NULL;
                    D3DSURFACE_DESC desc;
                    surface->GetDesc( &desc );

                    iD3DDevice->CreateRenderTarget( desc.Width, desc.Height, desc.Format, desc.MultiSampleType, desc.MultiSampleQuality,
                                                    true, &rt->mSurfaceCopy, NULL );
                    if( rt->mSurfaceCopy && 
                        S_OK == D3DXLoadSurfaceFromSurface( rt->mSurfaceCopy, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0 ) )
                    {
                        //iD3DDevice->StretchRect( rt->mSurfaceCopy, NULL, rt->mSurface, NULL, D3DTEXF_POINT );
                        rt->mNext = sDX9Data.mRTCopies;
                        sDX9Data.mRTCopies = rt;
                    }
                    else
                    {
                        if( rt->mSurfaceCopy )
                            rt->mSurfaceCopy->Release();
                        Dissector::FreeCallback( rt );
                    }

                }
            }
        }
    }

    int GetNumVertices( D3DPRIMITIVETYPE iPrimitiveType, UINT iPrimitiveCount )
    {
        switch( iPrimitiveType )
        {
        case( D3DPT_POINTLIST ):        return iPrimitiveCount;
        case( D3DPT_LINELIST ):         return iPrimitiveCount / 2;
        case( D3DPT_LINESTRIP ):        return iPrimitiveCount - 1;
        case( D3DPT_TRIANGLELIST ):     return iPrimitiveCount / 3;
        case( D3DPT_TRIANGLESTRIP ):    return iPrimitiveCount - 2;
        case( D3DPT_TRIANGLEFAN ):      return iPrimitiveCount - 2;
        }

        assert( false && "Unknown primitive type" );

        return 0;
    }

    HRESULT DrawPrimitiveUP( IDirect3DDevice9* iD3DDevice, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,
                             const void *pVertexStreamZeroData, UINT VertexStreamZeroStride )
    {
        if( Dissector::IsCapturing() )
        {
            int numVertices = GetNumVertices( PrimitiveType, PrimitiveCount );
            int dataSize = sizeof(DrawUpData) + VertexStreamZeroStride * numVertices;
            assert( dataSize < 2048 );

            char* drawcalldata = (char*)sDX9Data.mUPDataBuffer;
            DrawUpData* dc = (DrawUpData*)drawcalldata;
            dc->mPrimitiveType = PrimitiveType;
            dc->mPrimitiveCount = PrimitiveCount;
            dc->mStartElement = 0;
            dc->mStride = VertexStreamZeroStride;
            memcpy( drawcalldata + sizeof(DrawUpData), pVertexStreamZeroData, VertexStreamZeroStride * numVertices );
            InternalCheckRTUsage( iD3DDevice );

            Dissector::RegisterEvent( iD3DDevice, ET_DRAWUP, drawcalldata, dataSize );
        }
        return iD3DDevice->DrawPrimitiveUP( PrimitiveType, PrimitiveCount, 
                                     pVertexStreamZeroData, VertexStreamZeroStride );
    }

    HRESULT DrawIndexedPrimitiveUP( IDirect3DDevice9* iD3DDevice, D3DPRIMITIVETYPE PrimitiveType,
                                    UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount,
                                    const void *pIndexData, D3DFORMAT IndexDataFormat, 
                                    const void *pVertexStreamZeroData, UINT VertexStreamZeroStride )
    {
        if( Dissector::IsCapturing() )
        {
            unsigned int indexSize = IndexDataFormat == D3DFMT_INDEX16 ? 2 : 4;
            int numIndices = GetNumVertices( PrimitiveType, PrimitiveCount );
            int dataSize = (unsigned int)(sizeof(DrawIndexedUpData)) + VertexStreamZeroStride * NumVertices + numIndices * indexSize;
            assert( dataSize < 2048 );

            char* drawcalldata = (char*)sDX9Data.mUPDataBuffer;
            DrawIndexedUpData* dc = (DrawIndexedUpData*)drawcalldata;
            dc->mNumIndices = numIndices;
            dc->mNumVertices = NumVertices;
            dc->mIndexDataFormat = IndexDataFormat;
            dc->mStride = VertexStreamZeroStride;
            dc->mPrimitiveType = PrimitiveType;
            dc->mPrimitiveCount = PrimitiveCount;
            dc->mStartElement = MinVertexIndex;

            char* iter = drawcalldata + sizeof(DrawIndexedUpData);
            memcpy( iter, pIndexData, numIndices * indexSize );
            iter += numIndices * indexSize;
            memcpy( iter, pVertexStreamZeroData, VertexStreamZeroStride * NumVertices );
            InternalCheckRTUsage( iD3DDevice );

            Dissector::RegisterEvent( iD3DDevice, ET_DRAWUP, drawcalldata, dataSize );
        }

        return iD3DDevice->DrawIndexedPrimitiveUP( PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount,
                                            pIndexData, IndexDataFormat, pVertexStreamZeroData,
                                            VertexStreamZeroStride );
    }

    bool ConvertRenderTargetToTexture( void* iDevice, IDirect3DSurface9* iSurface, IDirect3DTexture9** oTex )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        D3DSURFACE_DESC desc;
        iSurface->GetDesc( &desc );

        if( desc.Format == MAKEFOURCC( 'N', 'U', 'L', 'L' ) )
            return false;

        ScopedRelease<IDirect3DTexture9> tex;
        HRESULT res = D3DDevice->CreateTexture(desc.Width, desc.Height, 1, 0, desc.Format, D3DPOOL_SYSTEMMEM, &tex.mPtr, NULL);
        if (res != S_OK || !tex.mPtr)
            return false;

        ScopedRelease<IDirect3DSurface9> mip;
        tex->GetSurfaceLevel( 0, &mip.mPtr );
        if( S_OK != D3DDevice->GetRenderTargetData( iSurface, mip.mPtr ) )
            return false;

        res = D3DDevice->CreateTexture(desc.Width, desc.Height, 1, 0, desc.Format, D3DPOOL_DEFAULT, oTex, NULL);
        if (res != S_OK || !*oTex)
            return false;

        if( D3DDevice->UpdateTexture( tex.mPtr, *oTex ) != S_OK )
        {
            (*oTex)->Release();
            return false;
        }

        return true;
    }

    bool GetDepthForTextureImage( void* iDevice, void* iTexture, unsigned int iRSType, unsigned int iEventId )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        Dissector::DrawCallData* drawIter = NULL;
        drawIter = ExecuteToEvent( iDevice, iEventId+1 );
        if( !drawIter )
            return false;

        ScopedEndFrame endScene( D3DDevice );

        ScopedRelease<IDirect3DSurface9> ds;
        D3DDevice->GetDepthStencilSurface( &ds.mPtr );

        IDirect3DTexture9* surface = GetDepthBufferTexture( ds.mPtr );
        if( !surface )
            return false;

        D3DSURFACE_DESC desc;
        surface->GetLevelDesc( 0, &desc );

        if( desc.Format == MAKEFOURCC( 'I', 'N', 'T', 'Z' ) )
        {
            ScopedRelease<IDirect3DSurface9> dest;
            D3DDevice->CreateRenderTarget( desc.Width, desc.Height, D3DFMT_R32F, D3DMULTISAMPLE_NONE, 0, true, &dest.mPtr, NULL );
            if( dest.mPtr )
            {
                RenderTextureToRT( D3DDevice, surface, dest, NULL, true );

                D3DLOCKED_RECT rect;
                HRESULT res = dest->LockRect( &rect, NULL, D3DLOCK_READONLY );

                Dissector::ImageRetrievalCallback( iTexture, iRSType, rect.pBits, desc.Width, desc.Height, rect.Pitch, Dissector::PIXEL_R32F );
                dest->UnlockRect();

            }
        }

        return true;
    }

    bool GetRTForTextureImage( void* iDevice, void* iTexture, unsigned int iRSType, unsigned int iNum )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        ScopedRelease<IDirect3DTexture9> gputex;

        ScopedRelease<IDirect3DSurface9> rt, newRt;
        D3DDevice->GetRenderTarget( iNum, &rt.mPtr );
        if( !rt )
            return false;

        D3DSURFACE_DESC desc;
        rt->GetDesc( &desc );
        if( !ConvertRenderTargetToTexture( iDevice, rt, &gputex.mPtr ) )
            return false;

        D3DDevice->CreateRenderTarget( desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, true, &newRt.mPtr, NULL );
        if( !newRt )
            return false;

        RenderTextureToRT( D3DDevice, gputex.mPtr, newRt.mPtr, NULL, false );

        D3DLOCKED_RECT rect;
        HRESULT res = newRt->LockRect( &rect, NULL, D3DLOCK_READONLY );
        Dissector::ImageRetrievalCallback( iTexture, iRSType, rect.pBits, desc.Width, desc.Height, rect.Pitch, Dissector::PIXEL_A8R8G8B8 );

        return true;
    }

    void GetTextureThumbnailInternal( void* iDevice, void* iTexture, unsigned int iVisualizerType, int iEventNum, void* iIDOveride = NULL )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
        IDirect3DTexture9* src = (IDirect3DTexture9*)iTexture;

        D3DSURFACE_DESC desc;
        src->GetLevelDesc( 0, &desc );

        // Attempt to preserve aspect ratio or the thumbnails end up looking weird.
        int width, height;
        if( desc.Width > desc.Height )
        {
            width = 128;
            height = int( 128 * float( desc.Height ) / float( desc.Width ) );
        }
        else
        {
            height = 128;
            width = int( 128 * float( desc.Width ) / float( desc.Height ) );
        }

        ScopedRelease<IDirect3DSurface9> dest;
        D3DDevice->CreateRenderTarget( width, height, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, true, &dest.mPtr, NULL );
        if( dest.mPtr && src )
        {
            RenderTextureToRT( D3DDevice, src, dest );

            D3DLOCKED_RECT rect;
            HRESULT res = dest->LockRect( &rect, NULL, D3DLOCK_READONLY );
            Dissector::ThumbnailRetrievalCallback( iIDOveride ? iIDOveride : iTexture, iVisualizerType, rect.pBits, width, height, rect.Pitch, iEventNum );
            dest->UnlockRect();
        }
    }

    bool GetRTThumbnail( void* iDevice, void* iTexture, unsigned int iVisualizerType, int iEventNum )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        ScopedRelease<IDirect3DSurface9> rt, newRt;
        D3DDevice->GetRenderTarget( 0, &rt.mPtr );
        if( !rt )
            return false;

        D3DSURFACE_DESC desc;
        rt->GetDesc( &desc );

        ScopedRelease<IDirect3DTexture9> gputex;
        if( !ConvertRenderTargetToTexture( iDevice, rt.mPtr, &gputex.mPtr ) )
            return false;

        GetTextureThumbnailInternal( iDevice, gputex.mPtr, iVisualizerType, iEventNum, iTexture );

        return true;
    }

    bool EndFrameExternal( void* iDevice )
    {
        return EndFrame( iDevice, true );
    }

    char* GetPrimitiveTypeString( D3DPRIMITIVETYPE iType )
    {
        switch( iType )
        {
        case( D3DPT_POINTLIST ):        return "POINTLIST";
        case( D3DPT_LINELIST ):         return "LINELIST";
        case( D3DPT_LINESTRIP ):        return "LINESTRIP";
        case( D3DPT_TRIANGLELIST ):     return "TRIANGLELIST";
        case( D3DPT_TRIANGLESTRIP ):    return "TRIANGLESTRIP";
        case( D3DPT_TRIANGLEFAN ):      return "TRIANGLEFAN";
        default:                        return "Unknown";
        }
    }

    struct DX9Callbacks : public Dissector::RenderCallbacks
    {
        virtual void FillRenderStates(void* iDevice, int iEventType, const void* iEventData,  unsigned int iDataSize)
        {
            GetRenderStates( iDevice, iEventType, iEventData,  iDataSize );
        }

        virtual void ResimulateStart(void* iDevice)
        {
            sDX9Data.mLastEvent = NULL;
            BeginFrame( iDevice );
        }

        virtual void ResimulateEvent(void* iDevice, const Dissector::DrawCallData& iData, bool iRecordTiming, Dissector::TimingData* oTiming )
        {
            DissectorDX9::ResimulateEvent( iDevice, iData, iRecordTiming, oTiming );
            sDX9Data.mLastEvent = &iData;
        }

        virtual void GatherStatsForEvent(void* iDevice, const Dissector::DrawCallData& iData)
        {
            IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
            SetRenderStates( D3DDevice, &iData );

            D3DDEVINFO_D3D9BANDWIDTHTIMINGS bandwidth;
            D3DDEVINFO_D3D9CACHEUTILIZATION cache;
            D3DDEVINFO_D3D9PIPELINETIMINGS  pipeline;
            D3DDEVINFO_VCACHE               vcache;
            D3DDEVINFO_D3DVERTEXSTATS       vertstats;
            D3DDEVINFO_D3D9STAGETIMINGS     stagetimings;

            if( sDX9Data.mBandwidthTimingsQuery ) sDX9Data.mBandwidthTimingsQuery->Issue(D3DISSUE_BEGIN);
            if( sDX9Data.mCacheUtilizationQuery ) sDX9Data.mCacheUtilizationQuery->Issue(D3DISSUE_BEGIN);
            if( sDX9Data.mPipelineTimingsQuery ) sDX9Data.mPipelineTimingsQuery->Issue(D3DISSUE_BEGIN);
            if( sDX9Data.mVcacheQuery ) sDX9Data.mVcacheQuery->Issue(D3DISSUE_BEGIN);
            if( sDX9Data.mVertexStatsQuery ) sDX9Data.mVertexStatsQuery->Issue(D3DISSUE_BEGIN);
            if( sDX9Data.mVertexTimingsQuery ) sDX9Data.mVertexTimingsQuery->Issue(D3DISSUE_BEGIN);
            if( sDX9Data.mPixelTimingsQuery ) sDX9Data.mPixelTimingsQuery->Issue(D3DISSUE_BEGIN);

            ExecuteEvent( D3DDevice, iData );

            if( sDX9Data.mBandwidthTimingsQuery ) sDX9Data.mBandwidthTimingsQuery->Issue(D3DISSUE_END);
            if( sDX9Data.mCacheUtilizationQuery ) sDX9Data.mCacheUtilizationQuery->Issue(D3DISSUE_END);
            if( sDX9Data.mPipelineTimingsQuery ) sDX9Data.mPipelineTimingsQuery->Issue(D3DISSUE_END);
            if( sDX9Data.mVcacheQuery ) sDX9Data.mVcacheQuery->Issue(D3DISSUE_END);
            if( sDX9Data.mVertexStatsQuery ) sDX9Data.mVertexStatsQuery->Issue(D3DISSUE_END);
            if( sDX9Data.mVertexTimingsQuery ) sDX9Data.mVertexTimingsQuery->Issue(D3DISSUE_END);
            if( sDX9Data.mPixelTimingsQuery ) sDX9Data.mPixelTimingsQuery->Issue(D3DISSUE_END);

            if( sDX9Data.mBandwidthTimingsQuery ) 
            {
                while(S_FALSE == sDX9Data.mBandwidthTimingsQuery->GetData( &bandwidth, 
                    sizeof(D3DDEVINFO_D3D9BANDWIDTHTIMINGS), D3DGETDATA_FLUSH ));
            }
            if( sDX9Data.mCacheUtilizationQuery ) 
            {
                while(S_FALSE == sDX9Data.mCacheUtilizationQuery->GetData( &cache, 
                    sizeof(D3DDEVINFO_D3D9CACHEUTILIZATION), D3DGETDATA_FLUSH ));
            }
            if( sDX9Data.mPipelineTimingsQuery ) 
            {
                while(S_FALSE == sDX9Data.mPipelineTimingsQuery->GetData( &pipeline, 
                    sizeof(D3DDEVINFO_D3D9PIPELINETIMINGS), D3DGETDATA_FLUSH ));
            }
            if( sDX9Data.mVcacheQuery ) 
            {
                while(S_FALSE == sDX9Data.mVcacheQuery->GetData( &vcache, 
                    sizeof(D3DDEVINFO_VCACHE), D3DGETDATA_FLUSH ));
            }
            if( sDX9Data.mVertexStatsQuery ) 
            {
                while(S_FALSE == sDX9Data.mVertexStatsQuery->GetData( &vertstats, 
                    sizeof(D3DDEVINFO_D3DVERTEXSTATS), D3DGETDATA_FLUSH ));
            }
            if( sDX9Data.mVertexTimingsQuery ) 
            {
                while(S_FALSE == sDX9Data.mVertexTimingsQuery->GetData( &stagetimings, 
                    sizeof(D3DDEVINFO_D3D9STAGETIMINGS), D3DGETDATA_FLUSH ));
            }
            if( sDX9Data.mPixelTimingsQuery ) 
            {
                while(S_FALSE == sDX9Data.mPixelTimingsQuery->GetData( &stagetimings, 
                    sizeof(D3DDEVINFO_D3D9STAGETIMINGS), D3DGETDATA_FLUSH ));
            }
        }

        virtual bool ResimulateEnd(void* iDevice)
        { 
            if( sDX9Data.mLastEvent )
            {
                if( sDX9Data.mLastEvent->mEventType == ET_DRAWINDEXED ||
                    sDX9Data.mLastEvent->mEventType == ET_DRAW ||
                    sDX9Data.mLastEvent->mEventType == ET_DRAWINDEXEDUP ||
                    sDX9Data.mLastEvent->mEventType == ET_DRAWUP )
                {
                    // TODO: Make this use a special shader to show wireframe stuff.
                    //IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
                    //ResetDevice( D3DDevice );
                    //D3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
                    //ExecuteEvent( D3DDevice, *sDX9Data.mLastEvent );
                }
            }

            return EndFrameExternal( iDevice );
        }

        virtual void ShaderDebug(void* iDevice, int iEventId, const Dissector::DrawCallData& iData, unsigned int iShaderType, char* iDebugData, unsigned int iDataSize)
        {
            switch( iShaderType )
            {
            case(Dissector::ST_PIXEL): DebugShader( iDevice, iEventId, iData, iShaderType, 
                                           iDebugData, iDataSize, true ); break;
            case(Dissector::ST_VERTEX): DebugShader( iDevice, iEventId, iData, iShaderType, 
                                            iDebugData, iDataSize, false ); break;
            default: break;
            }
        }

        virtual void GetTextureThumbnail( void* iDevice, void* iTexture, unsigned int iVisualizerType, int iEventNum )
        {
            if( iVisualizerType == Dissector::RSTF_VISUALIZE_RESOURCE_RENDERTARGET )
            {
                GetRTThumbnail( iDevice, iTexture, iVisualizerType, iEventNum );
            }
            else
            {
                GetTextureThumbnailInternal( iDevice, iTexture, iVisualizerType, iEventNum );
            }
        }

        virtual void GetTextureImage( void* iDevice, void* iTexture, unsigned int iRSType, unsigned int iEventNum )
        {
            IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
            ScopedRelease<IDirect3DTexture9> src;

            if( iRSType == 0 )
            {
                if( !iTexture )
                    return;

                src.mPtr = (IDirect3DTexture9*)iTexture;
                src->AddRef();
            }
            else
            {
                if( iRSType >= RT_TEXTURE_BEGIN && iRSType < RT_TEXTURE_END )
                {
                    int num = iRSType - RT_TEXTURE_BEGIN;
                    ScopedRelease<IDirect3DBaseTexture9> base;
                    D3DDevice->GetTexture( num, &base.mPtr );
                    if( base.mPtr )
                        base->QueryInterface( IID_IDirect3DBaseTexture9, (void**)&src.mPtr );
                    else
                        return;
                }
                else if( iRSType >= RT_RENDERTARGET_BEGIN && iRSType < RT_RENDERTARGET_END )
                {
                    GetRTForTextureImage( D3DDevice, iTexture, iRSType, iRSType - RT_RENDERTARGET_BEGIN );
                    return;
                }
                else if( iRSType == RT_DEPTHSTENCILSURFACE ) 
                {
                    GetDepthForTextureImage( D3DDevice, iTexture, iRSType, iEventNum );
                    return;
                }
                else
                {
                    return; // Unsupported type.
                }
            }

            if( !src.mPtr )
                return;

            D3DSURFACE_DESC desc;
            src->GetLevelDesc( 0, &desc );

            ScopedRelease<IDirect3DSurface9> dest;
            D3DDevice->CreateRenderTarget( desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, true, &dest.mPtr, NULL );
            if( dest.mPtr && src )
            {
                RenderTextureToRT( D3DDevice, src, dest );

                D3DLOCKED_RECT rect;
                HRESULT res = dest->LockRect( &rect, NULL, D3DLOCK_READONLY );
                Dissector::ImageRetrievalCallback( iTexture, iRSType, rect.pBits, desc.Width, desc.Height, rect.Pitch, Dissector::PIXEL_A8R8G8B8 );
                dest->UnlockRect();
            }

        }

        virtual void GetCurrentRT( void* iDevice )
        {
            IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

            ScopedRelease<IDirect3DSurface9> rt, newRt;
            D3DDevice->GetRenderTarget( 0, &rt.mPtr );
            if( !rt )
                return;

            D3DSURFACE_DESC desc;

            rt->GetDesc( &desc );

            if( desc.Format != D3DFMT_A8R8G8B8 )
            {
                // TODO: Make this work.
            }

            D3DDevice->CreateRenderTarget( desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, true, &newRt.mPtr, NULL );
            if( !newRt )
                return;

            D3DDevice->StretchRect( rt, NULL, newRt, NULL, D3DTEXF_POINT );

            D3DLOCKED_RECT rect;
            newRt->LockRect( &rect, NULL, 0 );
            if( rect.pBits )
            {
                Dissector::CurrentRTRetrievalCallback( rect.pBits, desc.Width, desc.Height, rect.Pitch );
            }
            newRt->UnlockRect();
        }

        virtual bool TestPixelsWritten(void* iDevice, const Dissector::DrawCallData& iData,
                        Dissector::PixelLocation* iPixels, unsigned int iNumPixels)
        {
            return TestPixelsWrittenInternal( iDevice, iData, iPixels, iNumPixels, true );
        }

        virtual void SlaveBegin( void* iDevice )
        {
            InitializeD3DResources( (IDirect3DDevice9*)( iDevice) );
        }

        virtual void SlaveEnd( void* iDevice )
        {
            DestroyD3DResources();

            // Set all the render states for the last draw call to make sure we didn't dirty anything that the application needs
            IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

            Dissector::DrawCallData* dcData = Dissector::GetCaptureData();
            int size = Dissector::GetCaptureSize()-1;

            if( size >= 0 )
            {
                sDX9Data.mNoDepthReplacements = true;
                Dissector::DrawCallData* dc = &dcData[ size ];
                SetRenderStates( D3DDevice, dc );
                sDX9Data.mNoDepthReplacements = false;
            }
        }

        virtual void TestPixelFailure(void* iDevice, unsigned int iEventId, Dissector::PixelLocation iPixel,
                        unsigned int *oFails )
        {
            if( (int)iEventId >= Dissector::GetCaptureSize() )
                return;

            IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

            bool testAllPixels = ((iPixel.mX == iPixel.mY) && (iPixel.mY == iPixel.mZ) && (iPixel.mZ == (unsigned int)-1));

            // Find how many render targets we have
            ShaderDebugDX9::ShaderInfo info;
            UINT shaderDataSize = 0, shaderDataPatchedSize = 0;
            ScopedFree<void> shaderData = GetPixelShaderByteCode( D3DDevice, shaderDataSize );
            info = ShaderDebugDX9::GetShaderInfo( shaderData, shaderDataSize, D3DDevice );

            // set all the disabled render targets to true.
            bool  colorWritten[4] = { (info.mOutputRegisterMask & 0x1) == 0, (info.mOutputRegisterMask & 0x2) == 0,
                                      (info.mOutputRegisterMask & 0x4) == 0, (info.mOutputRegisterMask & 0x8) == 0 };

            unsigned int failReasons[4] = { 0, 0, 0, 0 };

            DWORD colorWriteMasks[4];
            D3DDevice->GetRenderState( D3DRS_COLORWRITEENABLE,  &colorWriteMasks[0] );
            D3DDevice->GetRenderState( D3DRS_COLORWRITEENABLE1, &colorWriteMasks[1] );
            D3DDevice->GetRenderState( D3DRS_COLORWRITEENABLE2, &colorWriteMasks[2] );
            D3DDevice->GetRenderState( D3DRS_COLORWRITEENABLE3, &colorWriteMasks[3] );

            for( int ii = 0; ii < 4; ++ii )
            {
                ScopedRelease<IDirect3DSurface9> rt;
                D3DDevice->GetRenderTarget( ii, &rt.mPtr );
                if( rt == NULL )
                {
                    failReasons[ii] |= Dissector::PixelTests::RenderTargetsNotEnabled;
                }
            }

            for( int ii = 0; ii < 4; ++ii )
            {
                if( (info.mOutputRegisterMask & (1 << ii)) )
                {
                    if( colorWriteMasks[ii] )
                    {
                        ScopedRelease<IDirect3DSurface9> rt;
                        D3DDevice->GetRenderTarget( ii, &rt.mPtr );
                        if( rt == NULL )
                        {
                            if( info.mOutputRegisterMask & (1 << ii) )
                            {
                                info.mOutputRegisterMask &= ~(1 << ii);
                                failReasons[ii] |= Dissector::PixelTests::RenderTargetsBound;
                            }
                            colorWritten[ii] = true;
                            colorWriteMasks[ii] = 0;
                        }
                        else
                        {
                            D3DSURFACE_DESC desc;
                            rt->GetDesc( &desc );
                            unsigned int writeMaskRT = GetFormatWriteMask( desc.Format );
                            colorWriteMasks[ii] &= writeMaskRT;
                        }
                    }
                }
                else
                {
                    colorWriteMasks[ii] = 0;
                }
            }

            //Check for color write masks on valid render targets being 0.
            for( int ii = 0; ii < 4; ++ii )
            {
                if( (info.mOutputRegisterMask & (1 << ii)) && !colorWriteMasks[ii] )
                {
                    failReasons[ii] |= Dissector::PixelTests::ColorWriteMask;
                }
            }


            // Do pixel tests, start from the end of the pixel pipeline.
            for( int ii = 0; ii < 4; ++ii )
            {
                if( info.mOutputRegisterMask & (1 << ii) )
                {
                    DWORD tests = Dissector::PixelTests::AllPipeline;

                    if( TestPixelWrittenInternal( 0, ii, iDevice, iEventId, info, iPixel ) )
                    {
                        // Color was written successfully.
                        continue;
                    }

                    // Test with all gates open. If it isn't written then it must not be rasterized.
                    if( !TestPixelWrittenInternal( tests, ii, iDevice, iEventId, info, iPixel ) )
                    {
                        //TODO: Provide possible reasons here, like degenerate indices or vertices.
                        failReasons[ii] = Dissector::PixelTests::Rasterization;
                        continue;
                    }

                    // If the pixel is passing then do one test at a time to see if it fails that test.
                    for( int testMask = 1; testMask <= Dissector::PixelTests::LastPipeline; testMask <<= 1 )
                    {
                        if( !TestPixelWrittenInternal( tests & ~testMask, ii, iDevice, iEventId, info, iPixel ) )
                        {
                            failReasons[ii] |= testMask;
                        }
                    }
                }
            }

            // Get the render target pixel colors.
            // Get color output value for each render target
            float backBufferColor[4][4];
            float colors[4][4];

            bool  backBufferColorValid[4] = { true, true, true, true };

            {
                BeginFrame( iDevice );

                Dissector::DrawCallData* drawIter = NULL;

                drawIter = ExecuteToEvent( iDevice, iEventId );
                if( !drawIter )
                {
                    return;
                }

                for( int ii = 0; ii < 4; ++ii )
                {
                    if( info.mOutputRegisterMask & (1 << ii) )
                    {
                        if( !GetRenderTargetPixelColor( iDevice, ii, iPixel, backBufferColor[ii] ) )
                        {
                            backBufferColorValid[ii] = false;
                            backBufferColor[ii][0] = backBufferColor[ii][1] = 
                            backBufferColor[ii][2] = backBufferColor[ii][3] = 0.f;
                        }
                    }
                }

                EndFrame( iDevice );
            }

            // Get output color for each RT
            for( int ii = 0; ii < 4; ++ii )
            {
                if( info.mOutputRegisterMask & (1 << ii) )
                {
                    BeginFrame( iDevice );

                    Dissector::DrawCallData* drawIter = NULL;

                    drawIter = ExecuteToEvent( iDevice, iEventId );
                    if( !drawIter )
                    {
                        return;
                    }

                    SetRenderStates( D3DDevice, drawIter );
                    colorWritten[ii] = TestPixelWrittenInternal( failReasons[ii], ii, iDevice, iEventId, info, iPixel, colors[ii] );

                    EndFrame( iDevice );
                }
            }

            // Check to see if alpha blending is enabled and is causing the pixel writes to be invisible.
            for( int ii = 0; ii < 4; ++ii )
            {
                if( info.mOutputRegisterMask & (1 << ii) )
                {
                    BeginFrame( iDevice );
                    Dissector::DrawCallData* drawIter = NULL;
                    drawIter = ExecuteToEvent( iDevice, iEventId );
                    SetRenderStates( D3DDevice, drawIter );

                    // Gather alpha info.
                    DWORD blendEnable, srcBlend, dstBlend, blendOp, sepAlpha, alphaSrcBlend, alphaDstBlend, blendOpAlpha;
                    DWORD blendFactor;
                    D3DDevice->GetRenderState( D3DRS_ALPHABLENDENABLE, &blendEnable );
                    D3DDevice->GetRenderState( D3DRS_SRCBLEND, &srcBlend );
                    D3DDevice->GetRenderState( D3DRS_DESTBLEND, &dstBlend );
                    D3DDevice->GetRenderState( D3DRS_BLENDOP, &blendOp );
                    D3DDevice->GetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, &sepAlpha );
                    D3DDevice->GetRenderState( D3DRS_SRCBLENDALPHA, &alphaSrcBlend );
                    D3DDevice->GetRenderState( D3DRS_DESTBLENDALPHA, &alphaDstBlend );
                    D3DDevice->GetRenderState( D3DRS_BLENDOPALPHA, &blendOpAlpha );
                    D3DDevice->GetRenderState( D3DRS_BLENDFACTOR, (DWORD*)&blendFactor );
                    float factor[4];
                    if( backBufferColorValid[ii] && GetBlendFactor( srcBlend, blendFactor, backBufferColor[ii], 
                        colors[ii], factor ) )
                    {
                        ApplyColorWriteMask( colorWriteMasks[ii], factor );

                        static const float minAlpha = 1.f/255.f;
                        if( factor[0] < minAlpha && factor[1] < minAlpha && factor[2] < minAlpha && factor[3] < minAlpha )
                        {
                            failReasons[ii] |= Dissector::PixelTests::BlendFactor;
                        }

                        float blendedColor[4];
                        if( ApplyD3DBlending( srcBlend, dstBlend, blendOp, sepAlpha, alphaSrcBlend, alphaDstBlend, blendOpAlpha,
                            blendFactor, backBufferColor[ii], colors[ii], blendedColor ) )
                        {
                            for( int jj = 0; jj < 3; ++jj )
                            {
                                blendedColor[jj] = abs(blendedColor[jj] - backBufferColor[ii][jj]);
                            }
                            ApplyColorWriteMask( colorWriteMasks[ii], blendedColor );

                            // Check to see if the difference in RGB is less than a certain threshold, if it is warn
                            // the user that the color was written but may not be obviously visible.
                            static const float minColorDelta = 5.f/255.f;
                            if( blendedColor[0] <= minColorDelta && blendedColor[1] <= minColorDelta &&
                                blendedColor[2] <= minColorDelta )
                            {
                                failReasons[ii] |= Dissector::PixelTests::ColorMatches;
                            }
                        }
                    }

                    EndFrame( iDevice );
                }
            }

            for( int ii = 0; ii < 4; ++ii )
                oFails[ii] = failReasons[ii];
        }

        virtual void GetMesh(void* iDevice, unsigned int iEventId, unsigned int iType )
        {
            Dissector::DrawCallData* drawIter = ExecuteToEvent( iDevice, iEventId );
            if( !drawIter )
                return;

            IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
            SetRenderStates( D3DDevice, drawIter );
            UINT bufferSize = 0, indexSize = 0;

            float* meshData = NULL;
            unsigned int* indexData = NULL;
            ScopedFree<float> meshFreePtr;
            ScopedFree<UINT>  indexFreePtr;

            unsigned int primType, numVerts, numIndices, numPrimitives;

            if( drawIter->mEventType == ET_DRAWINDEXED )
            {
                DrawIndexedData* baseData = ((DrawIndexedData*)drawIter->mDrawCallData);
                primType = GetDissectorPrimitiveType( baseData->mPrimitiveType );
                numVerts = baseData->mNumVertices + baseData->mMinIndex;
                numPrimitives = baseData->mPrimitiveCount;
                numIndices = GetVertexCountFromPrimitiveCount( baseData->mPrimitiveType, baseData->mPrimitiveCount );
                meshFreePtr.mPtr = GetVertexShaderInputPositions( iDevice, baseData->mBaseVertexIndex, baseData->mNumVertices + baseData->mMinIndex, bufferSize );
                meshData = meshFreePtr.mPtr;
                indexFreePtr.mPtr = GetIndices( iDevice, baseData->mStartElement, numIndices, indexSize );
                indexData = indexFreePtr.mPtr;
                if( !meshFreePtr.mPtr || !indexFreePtr.mPtr )
                    return;
            }
            else if( drawIter->mEventType == ET_DRAW )
            {
                DrawBase* baseData = ((DrawBase*)drawIter->mDrawCallData);
                primType = GetDissectorPrimitiveType( baseData->mPrimitiveType );
                numPrimitives = baseData->mPrimitiveCount;
                numVerts = GetVertexCountFromPrimitiveCount( baseData->mPrimitiveType, baseData->mPrimitiveCount );
                numIndices = 0;
                meshFreePtr.mPtr = GetVertexShaderInputPositions( iDevice, baseData->mStartElement, numVerts, bufferSize );
                meshData = meshFreePtr.mPtr;
                if( !meshFreePtr.mPtr )
                    return;
            }
            else if( drawIter->mEventType == ET_DRAWINDEXEDUP )
            {
            }
            else if( drawIter->mEventType == ET_DRAWUP )
            {
            }
            else
            {
                return; // Nothing to do
            }

            Dissector::MeshDebugDataCallback( iEventId, Dissector::PrimitiveType::Type(primType), 
                numPrimitives, (char*)meshData, bufferSize, (char*)indexData, indexSize );
        }
       
        virtual void GetEventText(Dissector::DrawCallData* iDC, unsigned int iMaxSize, char* oBuffer, unsigned int& oSize)
        {
            DrawBase* dc = (DrawBase*)(iDC->mDrawCallData);
            switch( iDC->mEventType )
            {
            case( ET_DRAWINDEXED ):
                {
                    DrawIndexedData* di = (DrawIndexedData*)dc;
                    _snprintf( oBuffer, iMaxSize-1, "DrawIndexedPrimitive( %s, %d, %u, %u, %u, %u )",
                        GetPrimitiveTypeString( di->mPrimitiveType ), di->mBaseVertexIndex, di->mMinIndex, di->mNumVertices,
                        di->mStartElement, di->mPrimitiveCount );
                }break;
            case( ET_DRAW ):
                {
                    _snprintf( oBuffer, iMaxSize-1, "DrawPrimitive( %s, %u, %u )", 
                        GetPrimitiveTypeString( dc->mPrimitiveType ), dc->mStartElement, dc->mPrimitiveCount );
                }break;
            case( ET_DRAWINDEXEDUP ):
                {
                    DrawIndexedUpData* diu = (DrawIndexedUpData*)dc;
                    _snprintf( oBuffer, iMaxSize-1, "DrawIndexedUp( %s, %u, %u %u, %s, %u )",
                        GetPrimitiveTypeString( diu->mPrimitiveType ), diu->mStartElement, diu->mNumVertices,
                        diu->mPrimitiveCount, diu->mIndexDataFormat == D3DFMT_INDEX16 ? "16BitIndices" : "32BitIndices",
                        diu->mStride );
                }break;
            case( ET_DRAWUP ):
                {
                    DrawUpData* du = (DrawUpData*)dc;
                    _snprintf( oBuffer, iMaxSize-1, "DrawPrimitiveUP( %s, %u, %u )", 
                        GetPrimitiveTypeString( du->mPrimitiveType ), du->mPrimitiveCount, du->mStride );
                }break;
            case( ET_CLEAR ):
                {
                    ClearData* cd = (ClearData*)dc;
                    _snprintf( oBuffer, iMaxSize-1, "Clear( 0x%x, 0x%x, %f, 0x%x )",
                        cd->mFlags, cd->mColor, cd->mZ, cd->mStencil );
                }break;
            case( ET_ENDFRAME ):
                {
                    _snprintf( oBuffer, iMaxSize-1, "End Frame" );
                }break;
            default:
                _snprintf( oBuffer, iMaxSize-1, "Unsupported" );
                break;
            }

            oBuffer[iMaxSize-1] = 0;
            oSize = (unsigned int)strlen(oBuffer) + 1;
        }

        virtual void SlaveFrameBegin()
        {
            QueryPerformanceCounter( &mStartFrame );
        }

        virtual void SlaveFrameEnd()
        {
            // Limit frame to 60fps so we let Hulkamania run wild on the CPU.
            LARGE_INTEGER endFrame;
            QueryPerformanceCounter( &endFrame );
            float timeInMS = float( endFrame.QuadPart - mStartFrame.QuadPart ) * sDX9Data.mTimingFrequency;

            int sleepTime = int(timeInMS) - 16;
            if( sleepTime > 0 )
                Sleep( sleepTime );
        }

        virtual void BeginCaptureFrame( void* iDevice )
        {
        }

        virtual void EndCaptureFrame( void* iDevice )
        {
            int dummyData = 0;
            Dissector::RegisterEvent( iDevice, ET_ENDFRAME, (const char*)&dummyData, sizeof(int) );
        }

        LARGE_INTEGER mStartFrame;
    };

    static DX9Callbacks sDX9Callbacks;

    // ------------------------------------------------------------------------
    // Setup
    // ------------------------------------------------------------------------
    void Initialize( IDirect3DDevice9* iD3DDevice )
    {
        if( sDX9Data.mInitialized )
        {
            return;
        }

        iD3DDevice->GetDeviceCaps( &sDX9Data.mCaps );
        Dissector::SetRenderCallbacks( &sDX9Callbacks );

        LARGE_INTEGER freq;
        QueryPerformanceFrequency( &freq );
        sDX9Data.mTimingFrequency = 1000.f / float(freq.QuadPart);

        // Setup the render types
        Dissector::RegisterEventType( ET_DRAWINDEXED,   0, "DrawIndexed" );
        Dissector::RegisterEventType( ET_DRAW,          0, "Draw" );
        Dissector::RegisterEventType( ET_DRAWINDEXEDUP, 0, "DrawIndexedUP" );
        Dissector::RegisterEventType( ET_DRAWUP,        0, "DrawUP" );
        Dissector::RegisterEventType( ET_CLEAR,         0, "Clear" );
        Dissector::RegisterEventType( ET_ENDFRAME,      0, "EndFrame" );

        Dissector::RegisterRenderStateEnum( ENUM_ZBUFFER,
            "Off", D3DZB_FALSE,
            "On",  D3DZB_TRUE,
            "W-buffer", D3DZB_USEW,
            NULL );

        Dissector::RegisterRenderStateEnum( ENUM_FILLMODE, 
            "Point",        D3DFILL_POINT,
            "Wireframe",    D3DFILL_WIREFRAME,
            "Solid",        D3DFILL_SOLID,
            NULL );

        Dissector::RegisterRenderStateEnum( ENUM_BLEND,
            "Zero",                         D3DBLEND_ZERO,
            "One",                          D3DBLEND_ONE,
            "Source Color",                 D3DBLEND_SRCCOLOR,
            "Inverse Source Color",         D3DBLEND_INVSRCCOLOR,
            "Source Alpha",                 D3DBLEND_SRCALPHA,
            "Inverse Source Alpha",         D3DBLEND_INVSRCALPHA,
            "Destination Alpha",            D3DBLEND_DESTALPHA,
            "Inverse Destination Alpha",    D3DBLEND_INVDESTALPHA,
            "Destination Color",            D3DBLEND_DESTCOLOR,
            "Inverse Destination Color",    D3DBLEND_INVDESTCOLOR,
            "Source Alpha Sat",             D3DBLEND_SRCALPHASAT,
            "Blend Factor",                 D3DBLEND_BLENDFACTOR,
            "Inverse Blend Factor",         D3DBLEND_INVBLENDFACTOR,
            NULL );

        Dissector::RegisterRenderStateEnum( ENUM_BLENDOP,
            "Add",              D3DBLENDOP_ADD,
            "Subtract",         D3DBLENDOP_SUBTRACT,
            "Reverse Subtract", D3DBLENDOP_REVSUBTRACT,
            "Minimum",          D3DBLENDOP_MIN,
            "Maximum",          D3DBLENDOP_MAX,
            NULL );

        Dissector::RegisterRenderStateEnum( ENUM_CULL,
            "Off",                  D3DCULL_NONE,
            "Clockwise",            D3DCULL_CW,
            "Counter Clockwise",    D3DCULL_CCW,
            NULL );

        Dissector::RegisterRenderStateEnum( ENUM_CMP,
            "Never",                    D3DCMP_NEVER,
            "Less",                     D3DCMP_LESS,
            "Equal",                    D3DCMP_EQUAL,
            "Less than or equal",       D3DCMP_LESSEQUAL,
            "Greater",                  D3DCMP_GREATER,
            "Not equal",                D3DCMP_NOTEQUAL,
            "Greater than or equal",    D3DCMP_GREATEREQUAL,
            "Always",                   D3DCMP_ALWAYS,
            NULL );

        Dissector::RegisterRenderStateEnum( ENUM_STENCILOP,
            "Keep",                 D3DSTENCILOP_KEEP,
            "Zero",                 D3DSTENCILOP_ZERO,
            "Replace",              D3DSTENCILOP_REPLACE,
            "Increment (clamp)",    D3DSTENCILOP_INCRSAT,
            "Decrement (clamp)",    D3DSTENCILOP_DECRSAT,
            "Invert",               D3DSTENCILOP_INVERT,
            "Increment (wrap)",     D3DSTENCILOP_INCR,
            "Decrement (wrap)",     D3DSTENCILOP_DECR,
            NULL );

        Dissector::RegisterRenderStateEnum( ENUM_SAMPLER,
            "Wrap",         D3DTADDRESS_WRAP,
            "Mirror",       D3DTADDRESS_MIRROR,
            "Clamp",        D3DTADDRESS_CLAMP,
            "Border",       D3DTADDRESS_BORDER,
            "Mirror once",  D3DTADDRESS_MIRRORONCE,
            NULL );

        Dissector::RegisterRenderStateEnum( ENUM_FILTER,
            "None",             D3DTEXF_NONE,
            "Point",            D3DTEXF_POINT,
            "Linear",           D3DTEXF_LINEAR,
            "Anisotropic",      D3DTEXF_ANISOTROPIC,
            "Pyramidal Quad",   D3DTEXF_PYRAMIDALQUAD,
            "Gaussian Quad",    D3DTEXF_GAUSSIANQUAD,
            NULL );

        // Register type identifications with dissector
        Dissector::RegisterRenderStateTypes( rts, sizeof(rts) / sizeof(Dissector::RenderStateType) );

        sDX9Data.mDepthBuffers = NULL;
        sDX9Data.mAssetHandles = NULL;
        sDX9Data.mAssetHandlesCount = 0;

        sDX9Data.mInitialized = true;
        sDX9Data.mNoDepthReplacements = false;
    }

    void Shutdown()
    {
        if( sDX9Data.mInitialized )
        {
            Dissector::ClearTypeInformation();
            DestroyD3DResources();
            sDX9Data.mInitialized = false;
        }
    }

    void SetUIPumpFunction( bool (*iPumpFunction)() )
    {
        sDX9Data.mPumpFunction = iPumpFunction;
    }

};