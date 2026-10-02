static const float2 positions[3] = {
  float2( 0.0, -0.5),
  float2(-0.5,  0.5),
  float2( 0.5,  0.5)
};

static const float3 colors[3] = {
  float3(1, 0, 0),
  float3(0, 1, 0),
  float3(0, 0, 1),
};

struct VertexOutput {
  float4 position : SV_Position;
  float3 color;
};

[shader("vertex")]
VertexOutput VertexMain(const uint vertexId : SV_VertexID) {
  VertexOutput vertex;
  vertex.position = float4(positions[vertexId], 0, 1);
  vertex.color = colors[vertexId];
  return vertex;
}

[shader("fragment")]
float4 FragmentMain(const VertexOutput vertex) : SV_Target {
  return float4(vertex.color, 1);
}