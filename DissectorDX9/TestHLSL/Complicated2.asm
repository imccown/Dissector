Microsoft (R) Direct3D Shader Compiler 9.29.952.3111
Copyright (C) Microsoft Corporation 2002-2009. All rights reserved.

//
// Generated by Microsoft (R) HLSL Shader Compiler 9.29.952.3111
//
//   fxc /Zi /Tps_3_0 /Od /EComplicated2PS Complicated2.hlsl
//
//
// Parameters:
//
//   float4x4 gMatrix;
//   float4x4 gMatrix2;
//   sampler2D gShadow;
//   sampler2D gTex;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   gMatrix      c0       4
//   gMatrix2     c4       4
//   gTex         s0       1
//   gShadow      s1       1
//

    ps_3_0
    dcl_texcoord v0.xy  // iTex0<0,1>
    dcl_2d s0
    dcl_2d s1

#line 12 "D:\code\Dissector\DissectorDX9\TestHLSL\Complicated2.hlsl"
    texld r0, v0, s0  // ::val1<0,1,2,3>
    texld r1, v0, s1  // ::val2<0,1,2,3>
    mov r2, c0  // ::gMatrix<0,4,8,12>
    mul r3, r2, c4.x
    mov r4, c1  // ::gMatrix<1,5,9,13>
    mul r5, r4, c4.y
    add r3, r3, r5
    mov r5, c2  // ::gMatrix<2,6,10,14>
    mul r6, r5, c4.z
    add r3, r3, r6
    mov r6, c3  // ::gMatrix<3,7,11,15>
    mul r7, r6, c4.w
    add r3, r3, r7  // ::tempMatrix<0,4,8,12>
    mul r7, r2, c5.x
    mul r8, r4, c5.y
    add r7, r7, r8
    mul r8, r5, c5.z
    add r7, r7, r8
    mul r8, r6, c5.w
    add r7, r7, r8  // ::tempMatrix<1,5,9,13>
    mul r8, r2, c6.x
    mul r9, r4, c6.y
    add r8, r8, r9
    mul r9, r5, c6.z
    add r8, r8, r9
    mul r9, r6, c6.w
    add r8, r8, r9  // ::tempMatrix<2,6,10,14>
    mul r2, r2, c7.x
    mul r4, r4, c7.y
    add r2, r2, r4
    mul r4, r5, c7.z
    add r2, r2, r4
    mul r4, r6, c7.w
    add r2, r2, r4  // ::tempMatrix<3,7,11,15>
    nop
    mov r0, r0  // DoSomeBullshit::iVal1<0,1,2,3>
    mov r1, r1  // DoSomeBullshit::iVal2<0,1,2,3>

#line 4 "Common.hlsl"
    add r4, r0, r1  // ::rvalue<0,1,2,3>
    mul r4, r0, r4
    mul r4, r1, r4  // ::rvalue<0,1,2,3>
    mov r4, r4  // ::DoSomeBullshit<0,1,2,3>

#line 16 "D:\code\Dissector\DissectorDX9\TestHLSL\Complicated2.hlsl"
    add r0, r0, r4
    add r0, r1, r0  // ::rvalue<0,1,2,3>
    dp4 r1.x, r0, r3  // ::rvalue<0>
    dp4 r1.y, r0, r7  // ::rvalue<1>
    dp4 r1.z, r0, r8  // ::rvalue<2>
    dp4 r1.w, r0, r2  // ::rvalue<3>
    mov oC0, r1  // ::Complicated2PS<0,1,2,3>

// approximately 47 instruction slots used (2 texture, 45 arithmetic)
