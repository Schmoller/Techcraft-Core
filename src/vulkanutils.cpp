#include "vulkanutils.hpp"
#include <fstream>

namespace Engine {

bool checkValidationLayerSupport() {
    auto props = vk::enumerateInstanceLayerProperties();

    for (const char *layerName : VALIDATION_LAYERS) {
        bool layerFound = false;

        for (const auto& layerProps: props) {
            if (strcmp(layerName, layerProps.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats) {
    for (const auto &format : formats) {
        if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }

    return formats[0];
}

vk::PresentModeKHR chooseSwapSurfacePresentMode(const std::vector<vk::PresentModeKHR> &modes) {
    vk::PresentModeKHR best = vk::PresentModeKHR::eFifo;

    // TODO: Allow configuration of this
    bool vsync = true;

    for (const auto &mode : modes) {
        if (mode == vk::PresentModeKHR::eMailbox) {
            return mode;
        } else if (mode == vk::PresentModeKHR::eImmediate && !vsync) {
            best = mode;
        }
    }

    return best;
}

bool hasStencilComponent(vk::Format format) {
    switch (format) {
        case vk::Format::eD16UnormS8Uint:
        case vk::Format::eD32SfloatS8Uint:
        case vk::Format::eD24UnormS8Uint:
            return true;
        default:
            return false;
    }
}

vk::ShaderModule createShaderModule(vk::Device device, const std::vector<char> &code) {
    vk::ShaderModuleCreateInfo createInfo(
        {}, static_cast<uint32_t>(code.size()), reinterpret_cast<const uint32_t*>(code.data())
    );

    return device.createShaderModule(createInfo);
}

std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void* alignedAlloc(size_t size, size_t alignment) {
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else 
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void alignedFree(void* data) {
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else 
	free(data);
#endif
}

}