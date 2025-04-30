import os
import random
import string
from pathlib import Path

def generate_random_string(length):
    return ''.join(random.choice(string.ascii_letters + string.digits) for _ in range(length))

def create_memset_test(directory):
    test_data = [
        ("small buffer", 10, 'A'),
        ("medium buffer", 100, 'B'),
        ("large buffer", 1000, 'C'),
        ("zero buffer", 0, 'D'),  # Edge case
        ("single byte", 1, 'E')
    ]
    
    test_code = '''#include "trap.h"

int main() {
'''
    
    for name, size, char in test_data:
        # Generate reference using Python's tools
        expected = char * size
        test_code += f'''
    // Test {name}
    char buf_{size}[{size + 1}];
    memset(buf_{size}, '{char}', {size});
    buf_{size}[{size}] = '\\0';
    check(memcmp(buf_{size}, "{expected}", {size}) == 0);
'''
    
    test_code += '''
    return 0;
}
'''
    
    with open(os.path.join(directory, 'memset_test.c'), 'w') as f:
        f.write(test_code)

def create_memcpy_test(directory):
    test_data = [
        ("small copy", 10),
        ("medium copy", 100),
        ("large copy", 1000),
        ("zero copy", 0),  # Edge case
        ("single byte copy", 1)
    ]
    
    test_code = '''#include "trap.h"

int main() {
'''
    
    for name, size in test_data:
        random_str = generate_random_string(size)
        test_code += f'''
    // Test {name}
    char src_{size}[] = "{random_str}";
    char dst_{size}[{size + 1}];
    memcpy(dst_{size}, src_{size}, {size});
    dst_{size}[{size}] = '\\0';
    check(memcmp(dst_{size}, src_{size}, {size}) == 0);
'''
    
    test_code += '''
    return 0;
}
'''
    
    with open(os.path.join(directory, 'memcpy_test.c'), 'w') as f:
        f.write(test_code)

def create_memmove_test(directory):
    test_code = '''#include "trap.h"

int main() {
    // Test non-overlapping regions
    char src[] = "This is a test string for memmove";
    char dst[50];
    memmove(dst, src, strlen(src) + 1);
    check(memcmp(dst, src, strlen(src) + 1) == 0);

    // Test overlapping regions (src < dst)
    char buf1[100] = "Overlapping test string";
    memmove(buf1 + 10, buf1, 15);
    // Can't easily check correctness, but shouldn't crash

    // Test overlapping regions (dst < src)
    char buf2[100] = "Another overlapping test string";
    memmove(buf2, buf2 + 5, 15);
    // Can't easily check correctness, but shouldn't crash

    // Test zero-length move
    char buf3[10] = "Test";
    memmove(buf3, buf3, 0);
    check(strcmp(buf3, "Test") == 0);

    return 0;
}
'''
    
    with open(os.path.join(directory, 'memmove_test.c'), 'w') as f:
        f.write(test_code)

def create_memcmp_test(directory):
    test_code = '''#include "trap.h"

int main() {
    char str1[] = "ABC";
    char str2[] = "ABC";
    char str3[] = "ABD";
    char str4[] = "ABB";
    char str5[] = "ABCD";

    // Equal strings
    check(memcmp(str1, str2, 3) == 0);
    check(memcmp(str1, str2, 0) == 0);  // zero-length compare

    // Different strings
    check(memcmp(str1, str3, 3) < 0);  // str1 < str3
    check(memcmp(str3, str1, 3) > 0);  // str3 > str1

    check(memcmp(str1, str4, 3) > 0);  // str1 > str4
    check(memcmp(str4, str1, 3) < 0);  // str4 < str1

    // Different lengths
    check(memcmp(str1, str5, 3) == 0);  // first 3 bytes equal
    check(memcmp(str1, str5, 4) < 0);   // str1 < str5 (shorter)

    // Test with binary data (including null bytes)
    char bin1[] = {0x00, 0x01, 0x02, 0x03};
    char bin2[] = {0x00, 0x01, 0x02, 0x04};
    check(memcmp(bin1, bin2, 3) == 0);
    check(memcmp(bin1, bin2, 4) < 0);

    return 0;
}
'''
    
    with open(os.path.join(directory, 'memcmp_test.c'), 'w') as f:
        f.write(test_code)

def create_strlen_test(directory):
    test_strings = [
        ("empty string", ""),
        ("short string", "hello"),
        ("long string", "this is a much longer string that will test boundary conditions"),
        ("string with spaces", "string with spaces"),
        ("special chars", "!@#$%^&*()")
    ]
    
    test_code = '''#include "trap.h"

int main() {
'''
    
    for name, s in test_strings:
        test_code += f'''
    // Test {name}
    char {name.replace(' ', '_')}[] = "{s}";
    check(strlen({name.replace(' ', '_')}) == {len(s)});
'''
    
    test_code += '''
    return 0;
}
'''
    
    with open(os.path.join(directory, 'strlen_test.c'), 'w') as f:
        f.write(test_code)

def create_strcat_test(directory):
    test_pairs = [
        ("empty + empty", "", ""),
        ("empty + non-empty", "", "world"),
        ("non-empty + empty", "hello", ""),
        ("regular strings", "hello", "world"),
        ("long strings", "this is a long string", " and this is another long string")
    ]
    
    test_code = '''#include "trap.h"

int main() {
'''
    
    for name, s1, s2 in test_pairs:
        expected = s1 + s2
        test_code += f'''
    // Test {name}
    char src_{name.replace(' ', '_')}[] = "{s2}";
    char dst_{name.replace(' ', '_')}[100] = "{s1}";
    check(strcmp(strcat(dst_{name.replace(' ', '_')}, src_{name.replace(' ', '_')}), "{expected}") == 0);
'''
    
    test_code += '''
    return 0;
}
'''
    
    with open(os.path.join(directory, 'strcat_test.c'), 'w') as f:
        f.write(test_code)

def create_strcpy_test(directory):
    test_strings = [
        ("empty string", ""),
        ("short string", "hello"),
        ("long string", "this is a much longer string that will test boundary conditions"),
        ("string with spaces", "string with spaces"),
        ("special chars", "!@#$%^&*()")
    ]
    
    test_code = '''#include "trap.h"

int main() {
    char dst[100];
'''
    
    for name, s in test_strings:
        test_code += f'''
    // Test {name}
    char src_{name.replace(' ', '_')}[] = "{s}";
    check(strcmp(strcpy(dst, src_{name.replace(' ', '_')}), "{s}") == 0);
'''
    
    test_code += '''
    return 0;
}
'''
    
    with open(os.path.join(directory, 'strcpy_test.c'), 'w') as f:
        f.write(test_code)

def create_strncpy_test(directory):
    test_cases = [
        ("full copy", "hello world", 11),
        ("partial copy", "hello world", 5),
        ("zero copy", "hello world", 0),
        ("copy more than length", "hello", 10),
        ("empty string", "", 5)
    ]
    
    test_code = '''#include "trap.h"

int main() {
    char dst[100];
'''
    
    for name, s, n in test_cases:
        expected = s[:n] if n <= len(s) else s + '\0' * (n - len(s))
        test_code += f'''
    // Test {name}
    char src_{name.replace(' ', '_')}[] = "{s}";
    memset(dst, '\\0', sizeof(dst));
    strncpy(dst, src_{name.replace(' ', '_')}, {n});
    check(memcmp(dst, "{expected}", {n}) == 0);
'''
    
    test_code += '''
    return 0;
}
'''
    
    with open(os.path.join(directory, 'strncpy_test.c'), 'w') as f:
        f.write(test_code)

def create_strcmp_test(directory):
    test_pairs = [
        ("equal strings", "hello", "hello"),
        ("different case", "hello", "Hello"),
        ("different strings", "hello", "world"),
        ("empty strings", "", ""),
        ("one empty", "hello", ""),
        ("different lengths", "hello", "hello world")
    ]
    
    test_code = '''#include "trap.h"

int main() {
'''
    
    for name, s1, s2 in test_pairs:
        expected = 1 if s1 > s2 else (-1 if s1 < s2 else 0)
        test_code += f'''
    // Test {name}
    char s1_{name.replace(' ', '_')}[] = "{s1}";
    char s2_{name.replace(' ', '_')}[] = "{s2}";
    check(strcmp(s1_{name.replace(' ', '_')}, s2_{name.replace(' ', '_')}) == {expected});
'''
    
    test_code += '''
    return 0;
}
'''
    
    with open(os.path.join(directory, 'strcmp_test.c'), 'w') as f:
        f.write(test_code)

def create_strncmp_test(directory):
    test_cases = [
        ("equal strings", "hello", "hello", 5),
        ("partial equal", "hello world", "hello there", 5),
        ("different", "apple", "apricot", 3),
        ("different case", "Hello", "hello", 5),
        ("zero length", "hello", "world", 0),
        ("compare beyond length", "short", "shorter", 10)
    ]
    
    test_code = '''#include "trap.h"

int main() {
'''
    
    for name, s1, s2, n in test_cases:
        s1_part = s1[:n]
        s2_part = s2[:n]
        expected = 1 if s1_part > s2_part else (-1 if s1_part < s2_part else 0)
        test_code += f'''
    // Test {name}
    char s1_{name.replace(' ', '_')}[] = "{s1}";
    char s2_{name.replace(' ', '_')}[] = "{s2}";
    check(strncmp(s1_{name.replace(' ', '_')}, s2_{name.replace(' ', '_')}, {n}) == {expected});
'''
    
    test_code += '''
    return 0;
}
'''
    
    with open(os.path.join(directory, 'strncmp_test.c'), 'w') as f:
        f.write(test_code)

def create_printf_test(directory):
    test_code = '''#include "trap.h"
#include <stdio.h>
#include <string.h>

int main() {
    char buf[100];
    int len;
    
    // Test basic string
    len = printf("Hello, world!\\n");
    check(len == 13);
    
    // Test format specifiers
    len = printf("Int: %d, String: %s, Char: %c\\n", 42, "test", 'X');
    check(len == 29);
    
    // Test edge cases
    len = printf("");
    check(len == 0);
    
    len = printf("%%");
    check(len == 1);
    
    return 0;
}
'''
    
    with open(os.path.join(directory, 'printf_test.c'), 'w') as f:
        f.write(test_code)

def create_sprintf_test(directory):
    test_code = '''#include "trap.h"
#include <stdio.h>
#include <string.h>

int main() {
    char buf[100];
    int len;
    
    // Test basic string
    len = sprintf(buf, "Hello, world!");
    check(len == 13);
    check(strcmp(buf, "Hello, world!") == 0);
    
    // Test format specifiers
    len = sprintf(buf, "Int: %d, String: %s, Char: %c", 42, "test", 'X');
    check(len == 28);
    check(strcmp(buf, "Int: 42, String: test, Char: X") == 0);
    
    // Test edge cases
    len = sprintf(buf, "");
    check(len == 0);
    check(strcmp(buf, "") == 0);
    
    len = sprintf(buf, "%%");
    check(len == 1);
    check(strcmp(buf, "%") == 0);
    
    return 0;
}
'''
    
    with open(os.path.join(directory, 'sprintf_test.c'), 'w') as f:
        f.write(test_code)

def create_snprintf_test(directory):
    test_code = '''#include "trap.h"
#include <stdio.h>
#include <string.h>

int main() {
    char buf[10];
    int len;
    
    // Test basic string that fits
    len = snprintf(buf, sizeof(buf), "Hello");
    check(len == 5);
    check(strcmp(buf, "Hello") == 0);
    
    // Test string that's too long
    len = snprintf(buf, sizeof(buf), "Hello, world!");
    check(len == 13);  // Returns length that would have been written
    check(strncmp(buf, "Hello, wo", sizeof(buf) - 1) == 0);
    check(buf[sizeof(buf) - 1] == '\\0');
    
    // Test exact fit
    len = snprintf(buf, sizeof(buf), "123456789");
    check(len == 9);
    check(strcmp(buf, "123456789") == 0);
    
    // Test zero size
    len = snprintf(buf, 0, "Hello");
    check(len == 5);
    
    return 0;
}
'''
    
    with open(os.path.join(directory, 'snprintf_test.c'), 'w') as f:
        f.write(test_code)

def create_vsprintf_test(directory):
    test_code = '''#include "trap.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void test_vsprintf(char *buf, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);
}

int main() {
    char buf[100];
    
    // Test basic string
    test_vsprintf(buf, "Hello, world!");
    check(strcmp(buf, "Hello, world!") == 0);
    
    // Test format specifiers
    test_vsprintf(buf, "Int: %d, String: %s, Char: %c", 42, "test", 'X');
    check(strcmp(buf, "Int: 42, String: test, Char: X") == 0);
    
    return 0;
}
'''
    
    with open(os.path.join(directory, 'vsprintf_test.c'), 'w') as f:
        f.write(test_code)

def create_vsnprintf_test(directory):
    test_code = '''#include "trap.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void test_vsnprintf(char *buf, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(buf, size, format, args);
    va_end(args);
}

int main() {
    char buf[10];
    
    // Test basic string that fits
    test_vsnprintf(buf, sizeof(buf), "Hello");
    check(strcmp(buf, "Hello") == 0);
    
    // Test string that's too long
    test_vsnprintf(buf, sizeof(buf), "Hello, world!");
    check(strncmp(buf, "Hello, wo", sizeof(buf) - 1) == 0);
    check(buf[sizeof(buf) - 1] == '\\0');
    
    // Test exact fit
    test_vsnprintf(buf, sizeof(buf), "123456789");
    check(strcmp(buf, "123456789") == 0);
    
    return 0;
}
'''
    
    with open(os.path.join(directory, 'vsnprintf_test.c'), 'w') as f:
        f.write(test_code)

def generate_tests(directory):
    # Create directory if it doesn't exist
    Path(directory).mkdir(parents=True, exist_ok=True)
    
    # Generate all test files
    create_memset_test(directory)
    create_memcpy_test(directory)
    create_memmove_test(directory)
    create_memcmp_test(directory)
    create_strlen_test(directory)
    create_strcat_test(directory)
    create_strcpy_test(directory)
    create_strncpy_test(directory)
    create_strcmp_test(directory)
    create_strncmp_test(directory)
    create_printf_test(directory)
    create_sprintf_test(directory)
    create_snprintf_test(directory)
    create_vsprintf_test(directory)
    create_vsnprintf_test(directory)
    
    print(f"Generated test files in {directory}")

if __name__ == "__main__":
    output_dir = "/home/tourist/diy/cs/core/nju-pa/am-kernels/tests/klib-tests/tests"

    import sys
    if len(sys.argv) == 2:
        output_dir = sys.argv[1]

    generate_tests(output_dir)