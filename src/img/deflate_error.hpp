#pragma once

#include <system_error>

namespace img::deflate
{
    enum struct DeflateError
    {
        general_error = -20,
        distance_exceeded = -11,
        invalid_huffman_code,
        dynamic_huffman_missing_end_block,
        dynamic_huffman_invalid_distance,
        dynamic_huffman_invalid_literal_or_length,
        dynamic_huffman_exceed_length,
        dynamic_huffman_no_first_length,
        dynamic_huffman_incomplete_code_length,
        dynamic_huffman_to_many_length_or_distance,
        invalid_block = -1,
        success = 0
    };

    struct DeflateCategory : std::error_category
    {
        const char* name() const noexcept override
        {
            return "deflate_error";
        }
        std::string message(int value) const override
        {
            switch (static_cast<DeflateError>(value))
            {
            case DeflateError::general_error:
                return "general error";
            case DeflateError::distance_exceeded:
                return "distance is too far back in fixed or dynamic block";
            case DeflateError::invalid_huffman_code:
                return "invalid literal/length or distance code in fixed or dynamic block";
            case DeflateError::dynamic_huffman_missing_end_block:
                return "missing end-of-block code";
            case DeflateError::dynamic_huffman_invalid_distance:
                return "invalid distance code lengths";
            case DeflateError::dynamic_huffman_invalid_literal_or_length:
                return "invalid literal/length code lengths";
            case DeflateError::dynamic_huffman_exceed_length:
                return "repeat more than specified lengths";
            case DeflateError::dynamic_huffman_no_first_length:
                return "repeat lengths with no first length";
            case DeflateError::dynamic_huffman_incomplete_code_length:
                return "code lengths codes incomplete";
            case DeflateError::dynamic_huffman_to_many_length_or_distance:
                return "too many length or distance codes";
            case DeflateError::invalid_block:
                return "invalid block type";
            default:
                return "unknown error";
            }
        }
    };

    std::error_category& deflate_category()
    {
        static DeflateCategory cate{};
        return cate;
    }

    std::error_code make_error_code(DeflateError value)
    {
        return { static_cast<int>(value), deflate_category() };
    }
}

namespace std
{
    template <>
    struct is_error_code_enum<img::deflate::DeflateError> : true_type
    {
    };
}