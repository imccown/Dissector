Microsoft (R) Direct3D Shader Compiler 9.29.952.3111
Copyright (C) Microsoft Corporation 2002-2009. All rights reserved.

//
// Generated by Microsoft (R) HLSL Shader Compiler 9.29.952.3111
//
//   fxc /Tps_3_0 /Zi /EDoubleColorPS DoubleColor.hlsl
//
//
// Parameters:
//
//   float4 kBlend;
//   float4 kColor;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   kColor       c0       1
//   kBlend       c1       1
//

    ps_3_0

#line 6 "D:\code\Dissector\DissectorDX9\DoubleColor.hlsl"
    mov r0, c0  // ::kColor<0,1,2,3>
    mul oC0, r0, c1  // ::DoubleColorPS<0,1,2,3>

// approximately 2 instruction slots used
