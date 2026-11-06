[[vk::binding(0, 0)]]
RWStructuredBuffer<float> threadPiBuffer;

struct PushConstant {
  uint threadCount;
  uint sampleCount;
};

[[vk::push_constant]]
PushConstant pushConstant;

uint Hash(uint x) {
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x;
}

float Random(uint threadIndex, uint sampleIndex) {
  uint seed = Hash(threadIndex) ^ sampleIndex;
  uint bits = Hash(seed);
  return float(bits >> 8) * (1.0f / 16777216.0f);
}

[shader("compute")]
[numthreads(256, 1, 1)]
void ComputeMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
  const uint threadIndex = dispatchThreadId.x;

  if (threadIndex >= pushConstant.threadCount) {
    return;
  }

  uint inside = 0;
  uint outside = 0;

  for (uint i = 0; i < pushConstant.sampleCount; ++i) {
    const float x = Random(threadIndex, i * 2);
    const float y = Random(threadIndex, i * 2 + 1);
    if (x * x + y * y <= 1) {
      inside += 1;
    } else {
      outside += 1;
    }
  }

  const float r = (float)inside / (float)outside;

  threadPiBuffer[threadIndex] = 4 * r / (1 + r);
}