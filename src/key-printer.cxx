#include <assert.h>
#include <unistd.h>
#include <termios.h>

#include "fmt.hxx"

int main(int argc, char *argv[]) {
#define BUF_SIZE 128
    char c[BUF_SIZE];
    struct termios termios_old, termios_new;

    tcgetattr(0, &termios_old);
    termios_new = termios_old;
    cfmakeraw(&termios_new);
    tcsetattr(0, TCSANOW, &termios_new);

    std::cout << "Press 'q' to exit\r\n";
    while (1) {
        int cnt = read(0, &c, BUF_SIZE);
        std::string s(c, cnt);
        std::cout << TermHub::Fmt::EscapedString(s);
        std::cout << "\r\n";

        if (c[0] == 'q')
            break;
    }

    tcsetattr(0, TCSANOW, &termios_old);
    return 0;
}

