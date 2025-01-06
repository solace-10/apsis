#pragma once

// clang-format off
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <nlohmann/json.hpp>
#include <tiny_gltf.h>
// clang-format on

namespace tinygltf
{

// This function gets called even if we try to export a GLTF with no textures, so we have to provide a stub.
bool WriteImageData(const std::string *basepath, const std::string *filename,
                    const Image *image, bool embedImages,
                    const FsCallbacks* fs_cb, const URICallbacks *uri_cb,
                    std::string *out_uri, void *);

} // namespace tinygltf
