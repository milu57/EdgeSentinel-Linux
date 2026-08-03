#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "output.h"

/*
 * 测试输出格式字符串解析。
 */
static void test_output_parse_format(void)
{
    OutputFormat format;

    assert(
        output_parse_format(
            "text",
            &format
        ) == 0
    );

    assert(format == OUTPUT_FORMAT_TEXT);

    assert(
        output_parse_format(
            "json",
            &format
        ) == 0
    );

    assert(format == OUTPUT_FORMAT_JSON);

    /*
     * 当前格式名称区分大小写。
     */
    assert(
        output_parse_format(
            "TEXT",
            &format
        ) == -1
    );

    assert(
        output_parse_format(
            "xml",
            &format
        ) == -1
    );

    /*
     * 空指针必须被拒绝。
     */
    assert(
        output_parse_format(
            NULL,
            &format
        ) == -1
    );

    assert(
        output_parse_format(
            "json",
            NULL
        ) == -1
    );
}

/*
 * 测试枚举值转换为字符串。
 */
static void test_output_format_to_string(void)
{
    assert(
        strcmp(
            output_format_to_string(
                OUTPUT_FORMAT_TEXT
            ),
            "text"
        ) == 0
    );

    assert(
        strcmp(
            output_format_to_string(
                OUTPUT_FORMAT_JSON
            ),
            "json"
        ) == 0
    );

    assert(
        strcmp(
            output_format_to_string(
                (OutputFormat)99
            ),
            "unknown"
        ) == 0
    );
}

/*
 * 测试输出函数对无效参数的处理。
 */
static void test_output_invalid_arguments(void)
{
    MonitorSnapshot snapshot = {0};

    assert(
        output_print_text(NULL) == -1
    );

    assert(
        output_print_json(NULL) == -1
    );

    assert(
        output_print(
            NULL,
            OUTPUT_FORMAT_TEXT
        ) == -1
    );

    assert(
        output_print(
            &snapshot,
            (OutputFormat)99
        ) == -1
    );
}

int main(void)
{
    test_output_parse_format();
    test_output_format_to_string();
    test_output_invalid_arguments();

    printf("All output tests passed.\n");

    return 0;
}
