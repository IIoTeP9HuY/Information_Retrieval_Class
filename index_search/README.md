irindexer is the program that allows you to search for relevant documents in big index
Relevancy metrics include simple boolean search, TF-IDF and BM25

To run program you can use following commands:
```bash
cmake .
make
./irindexer dictionary.txt index.txt
```

Program successfully runs on OSX 10.10

Dependencies:
* (gcc >= 4.8) or (clang >= 3.4)
* CMake >= 2.6

Author: Kashin Andrey, email: kashin.andrej@gmail.com
