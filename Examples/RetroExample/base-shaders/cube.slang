#include "shared.slang"

[shader("vertex")]
float4 vs_main(uint vertex_id : SV_VertexID)
    : SV_Position
{
    // Adapted from https://gist.github.com/rikusalminen/9393151
    uint tri = vertex_id / 3;
    uint idx = vertex_id % 3;
    uint face = tri / 2;
    uint top = tri % 2;

    uint dir = face % 3;
    uint pos = face / 3;

    uint nz = dir >> 1;
    uint ny = dir & 1;
    uint nx = 1 ^ (ny | nz);

    float3 d = float3(nx, ny, nz);
    float flip = 1.0 - 2.0 * pos;

    float3 n = flip * d;
    float3 u = -d.yzx;
    float3 v = flip * d.zxy;

    float mirror = -1.0 + 2.0 * top;
    float3 xyz = n + mirror * (1.0 - 2.0 * (idx & 1)) * u + mirror * (1.0 - 2.0 * (idx >> 1)) * v;

    float4 world_position = mul(float4(xyz, 1.0), frame_constants.cube_world);
    return mul(world_position, frame_constants.camera_buffer.view_proj);
}

[shader("fragment")]
float4 ps_main()
    : SV_TARGET0
{
    return light_color;
}
