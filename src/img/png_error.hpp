#pragma once

#include <system_error>

namespace img::png 
{
    enum struct PngError
    {
        invalid_signature = 10,
        invalid_ihdr,
        bitdepth_not_support,
        color_type_not_support,
        interlace_not_support,
        invalid_idat,
        fail_open_file
    };

    struct PngCategory : std::error_category
    {
        const char* name() const noexcept override
        {
            return "png_error";
        }
        std::string message(int value) const override
        {
            switch (static_cast<PngError>(value))
            {
            case PngError::invalid_signature:
                return "invalid signature";
            case PngError::invalid_ihdr:
                return "no valid IHDR";
            case PngError::bitdepth_not_support:
                return "not support bit detph";
            case PngError::color_type_not_support:
                return "not support color type";
            case PngError::interlace_not_support:
                return "not support interlace";
            case PngError::invalid_idat:
                return "no valid IDAT";
            case PngError::fail_open_file:
                return "can't open file";
            default:
                return "unknown error";
            }
        }
    };

    std::error_category& png_category()
    {
        static PngCategory cate{};
        return cate;
    }

    std::error_code make_error_code(PngError value)
    {
        return { static_cast<int>(value), png_category() };
    }
}

namespace std
{
    template <>
    struct is_error_code_enum<img::png::PngError> : true_type
    {
    };
}