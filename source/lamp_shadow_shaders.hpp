#pragma once
namespace lamp_shadow_shaders {
inline constexpr char caster[]=R"HLSL(
row_major float4x4 Light:register(c0);
row_major float4x4 World:register(c5);
float4 RasterOffset:register(c4);
float4 Material:register(c9);
float4 GrassWind:register(c10),GrassScale:register(c11);
float4 GrassInstances[228]:register(c20);
float4 Alpha:register(c4);sampler2D Mask:register(s0);
struct VSOut {float4 p:POSITION;float2 uv:TEXCOORD0;float2 zw:TEXCOORD1;float a:TEXCOORD2;float3 world:TEXCOORD3;};
VSOut vs(float3 p:POSITION,float2 uv:TEXCOORD0,float a:TEXCOORD1){VSOut o;float4 world=mul(World,float4(p,1));o.world=world.xyz;o.p=mul(Light,world);o.zw=o.p.zw;o.p.xy+=RasterOffset.xy*o.p.w;o.uv=uv;o.a=a;return o;}
VSOut opaque(float3 p:POSITION){return vs(p,float2(0,0),1);}
VSOut masked(float3 p:POSITION,float2 uv:TEXCOORD0,float4 colour:COLOR0){return vs(p,uv,Material.x>0?colour.a:1);}
VSOut grass(float3 p:POSITION,float2 uv:TEXCOORD0,float windWeight:TEXCOORD1,float instance:TEXCOORD2){
 VSOut o;int index=(int)clamp(floor(instance),0,227);float4 at=GrassInstances[index];
 float3 world=p*(1+.01*at.w*GrassScale.xyz)+at.xyz;
 float bend=sin((at.x+at.y)/128+GrassWind.w)*GrassWind.z*windWeight*windWeight;
 world.xy+=bend*GrassWind.xy;
 o.world=world;o.p=mul(Light,float4(world,1));o.zw=o.p.zw;
 o.p.xy+=RasterOffset.xy*o.p.w;o.uv=uv;
 // Grass vertex alpha is a wind weight, not coverage. Only texture alpha
 // cuts the blades. Unused instances in the native capacity draw no pixels.
 o.a=index<GrassScale.w?1:-1;return o;
}
float4 ps(VSOut i,float2 pixel:VPOS):COLOR0{
 if(Alpha.x>0){
  float coverage=tex2D(Mask,i.uv).a*i.a;
  if(Alpha.x>1){
   // Four fixed sub-samples match the receiver's existing 2x2 depth taps.
   // No frame counter or temporal pattern, and opaque objects behind glass
   // can still write depth through the uncovered samples.
   float2 parity=frac(floor(pixel)*.5)*2;
   float threshold=(2*abs(parity.x-parity.y)+parity.y+.5)*.25;
   clip(coverage*Alpha.z-threshold);
  }else clip(coverage-Alpha.y);
 }
 float depth=i.zw.x/i.zw.y;
 return float4(depth,depth*depth,0,1);}
)HLSL";
}
