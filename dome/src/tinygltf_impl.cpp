#include "tinygltf_impl.hpp"

namespace tinygltf
{

bool WriteImageData(const std::string *basepath, const std::string *filename,
                    const Image *image, bool embedImages,
                    const FsCallbacks* fs_cb, const URICallbacks *uri_cb,
                    std::string *out_uri, void *)
{
    return true;
}

} // namespace tinygltf
