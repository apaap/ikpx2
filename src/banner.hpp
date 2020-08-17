#pragma once

#include <iostream>

void print_banner() {

    std::cerr << "\033[31;1m\n           :'',\n          ,: '' '' ,,\n         :     : ,, ' ,\n        :    ,:   : : :\n"
                "         : , : , ', ,\n              ,,' ' ':  ,,\n                  ,  :'''',\n                 ,: :,'' ,,\n"
                "                    :, :',\n                  ,, :  '' :\n                   '''','': ,\n                          ',:\n"
                "                   '''     '  :\n                   ,    :'  , '\n                   ''', ',:,\n"
                "          \x1b[0m\x1b[34;1m*---*   *---*  \x1b[0m\x1b[31;1m'\x1b[0m\x1b[34;1m*---------*   *---*   *---*\n"
                "         /   /|  /   /\x1b[0m\x1b[31;1m'   \x1b[0m\x1b[34;1m|\\         \\   \\   \\ /   /\n"
                "        /   / * /   *---*\x1b[0m\x1b[31;1m:\x1b[0m\x1b[34;1m* \\   v2.0  \\   \\   v   /\n"
                "       /   / / /         \\ \\ \\         *   *     *\n"
                "      /   / / /       *---* \\ \\   *---*   /       \\\n"
                "     /   / / /   / \\   \\  \x1b[0m\x1b[31;1m,,,\x1b[0m\x1b[34;1m\\ \\   \\\x1b[0m\x1b[31;1m,    \x1b[0m\x1b[34;1m/   / \\   \\\n"
                "    *---* / *---* | *---* \x1b[0m\x1b[31;1m: ,:\x1b[0m\x1b[34;1m\\ *---*   *---* | *---*\n"
                "    |   |/  |   |/ \\|   | \x1b[0m\x1b[31;1m'   :\x1b[0m\x1b[34;1m\\|   |\x1b[0m\x1b[31;1m'' \x1b[0m\x1b[34;1m|   |/ \\|   |\n"
                "    *---*   *---*   *---*  \x1b[0m\x1b[31;1m'''  \x1b[0m\x1b[34;1m*---*   *---*   *---*\n"
                "                           \x1b[0m\x1b[31;1m,''',\n"
                "                            ' ::,\n                         ::' , ,:'\n                           '',  '\n"
                "                         ,''''':'',\n                          , ,:  :,:\n                                ''' ,,\n"
                "                              ':  , :\n                            ':,  ': :'\n                             ':',,' :\n"
                "                              ' ' :   ,\n                               ::,, '\n                                  :::\n"
                "                                ,,,''\n                                ',,\n\033[0m" << std::endl;

}


void print_help() {

    std::cerr << "Incremental Knightship Partial Extender, version 2.0\n"
                "====================================================\n\n"

                "Options:\n"

                "   -v, --velocity  specify the velocity, e.g. '(2,1)c/6' or 'c/5d'\n"
                "   -h, --help      show this message and exit\n" << std::endl;

}
