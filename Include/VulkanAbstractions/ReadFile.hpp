#pragma once

#include <vector>
#include <filesystem>
#include <format>
#include <fstream>
#include <cstring>
#include <cstddef>
#include <expected>

inline std::expected<std::vector<std::byte>, std::string> ReadFileBinary(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    return std::unexpected{ std::format("Failed to open file \"{}\" because it does not exist", path.string()) };
  }

  std::ifstream file{ path, std::ios::binary };
  if (!file) {
    return std::unexpected{ std::format("Failed to open file \"{}\"", path.string()) };
  }

  const auto size{ std::filesystem::file_size(path) };
  if (size == 0) {
    return std::vector<std::byte>{};
  }

  std::vector<std::byte> buffer(size);
  if (!file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size))) {
    return std::unexpected{ std::format("Failed to open file \"{}\"", path.string()) };
  }

  return buffer;
}

inline std::expected<std::vector<std::uint32_t>, std::string> ReadFileShader(const std::filesystem::path& path) {
  const auto readResult = ReadFileBinary(path);

  if (!readResult) {
    return std::unexpected{ readResult.error() };
  }

  const auto shaderCode = *readResult;

  std::vector<std::uint32_t> alignedShaderCode(shaderCode.size() / sizeof(std::uint32_t));
  std::memcpy(alignedShaderCode.data(), shaderCode.data(), shaderCode.size());

  return alignedShaderCode;
}
