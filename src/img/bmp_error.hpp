#pragma once

#include <system_error>

namespace img::bmp {
    enum struct BmpError
    {
        unknow_signature = 10,
        dib_not_support,
        bitdepth_not_support,
        compression_method_not_support,
        fail_open_file
    };

    struct BmpCategory : std::error_category
    {
        const char* name() const noexcept override
        {
            return "bmp_error";
        }
        std::string message(int value) const override
        {
            switch (static_cast<BmpError>(value))
            {
            case BmpError::unknow_signature:
                return "unknow signature";
            case BmpError::dib_not_support:
                return "not support DIB";
            case BmpError::bitdepth_not_support:
                return "not support bitdepth";
            case BmpError::compression_method_not_support:
                return "not support compression method";
            case BmpError::fail_open_file:
                return "fail open file";
            default:
                return "unknown error";
            }
        }
    };

    std::error_category& bmp_category()
    {
        static BmpCategory cate{};
        return cate;
    }

    std::error_code make_error_code(BmpError value)
    {
        return { static_cast<int>(value), bmp_category() };
    }
}

namespace std
{
    template <>
    struct is_error_code_enum<img::bmp::BmpError> : true_type
    {
    };
}