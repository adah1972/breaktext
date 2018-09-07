**breaktext** is a small program that uses [libunibreak](https://github.com/adah1972/libunibreak) (formerly liblinebreak) to break the lines of input text. One may type `breaktext` to see the usage.

Some usage examples follow.

Modern Unix systems (including Linux and macOS):

- `breaktext input.txt output.txt` breaks a UTF-8 text file with no explicit language info

Windows:

- `breaktext input.txt output.txt` breaks a UTF-16 text file with no explicit language info;
- `breaktext -LChinese_China.936 -lzh - < input.txt > output.txt` breaks a Chinese text file encoded in CP936.

The ‘native’ wide character type `wchar_t` is used in I/O routines, which causes this platform-dependent behaviour. On POSIX-compliant systems, the environment variables LANG, LC_ALL, and LC_CTYPE control the locale/encoding (unless overridden with the `-L` option), and UTF-8 will probably be used by default on modern systems. On Windows, the encoding is dependent on whether stdin/stdout is used for I/O: console I/O will be automatically converted to/from `wchar_t` (which is UTF-16) according to the system locale setting (overridable with `-L`), but files (excepting the stdin/stdout case) will always be in just `wchar_t` (UTF-16).
