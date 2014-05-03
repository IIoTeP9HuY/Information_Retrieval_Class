IndexFiles is the program that finds all .hpp files in specified directory
and it's subdirectories, count frequences of all words in this files and
prints top10 most frequent words.

To run program you can use following commands:
```bash
cmake .
make
./IndexFiles /usr/include/boost
```

Program successfully runs on fresh Ubuntu Server 10.04
Dependencies:
GCC >= 4.4
Boost >= 1.4
CMake >= 2.6

Author: Kashin Andrey, email: kashin.andrej@gmail.com
