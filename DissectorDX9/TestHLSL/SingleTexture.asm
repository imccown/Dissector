Microsoft (R) Direct3D Shader Compiler 9.29.952.3111
Copyright (C) Microsoft Corporation 2002-2009. All rights reserved.

//
// Generated by Microsoft (R) HLSL Shader Compiler 9.29.952.3111
//
//   fxc /Tps_3_0 /Zi /ESingleTexturePS SingleTexture.hlsl
//
//
// Parameters:
//
//   sampler2D gTex;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   gTex         s0       1
//

    ps_3_0
    dcl_texcoord v0.xy  // iTex0<0,1>
    dcl_2d s0

#line 5 "D:\code\Dissector\DissectorDX9\SingleTexture.hlsl"
    texld oC0, v0, s0  // ::SingleTexturePS<0,1,2,3>

// approximately 1 instruction slot used (1 texture, 0 arithmetic)
