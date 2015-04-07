# Information Retrieval Course [![Build Status](https://travis-ci.org/IIoTeP9HuY/Information_Retrieval_Class.png?branch=master)](https://travis-ci.org/IIoTeP9HuY/Information_Retrieval_Class)

Information Retrieval YSDA course programming assignments repository
http://shad.yandex.ru/

Introduction
============

These are my solutions for programming exercises from YSDA Information Retrieval classes. Written in Python/C++.

Usage tutorial
============

Build everything using
```bash
    mkdir build && cd build
    cmake ..
    make
```

#####Download wiki
```bash
    crawler http://simple.wikipedia.org/ -o wiki -t 8
```
This will download whole domain to folder "wiki", preserving hierarchical structure and will
fill file "ready_urls.txt" with downloaded urls.

#####Flatten hierarchical structure to simplify parsing
```bash
    flatten --urlsList ready_urls.txt --urlsDir wiki --outDir flat_wiki --urlsMapping urls
```
This will create folder "flat_wiki" with files 1.html, 2.html, ...
and also create file urls with mapping 1.html<tab>url_1, ...

#####Strip hypertext tags and extract text from webpages
```bash
    extract --urlsDir flat_site --outDir text_wiki --urlsMapping urls
```
This will fill the folder "text_wiki" with files 1.txt, 2.txt, ... corresponding to extracted
text from 1.html, 2.html, ...
Also it will produce file token_frequency containing lines token_1<tab>frequency_1 ...

#####Finally, analyze webgraph
```bash
    flat_webgraph --path flat_site --urlMapping urls --domain http://simple.wikipedia.org/ --start_page http://simple.wikipedia.org/wiki/Main_Page
```
This will create files:
* "distances" - distance from start_page to every other page
* "in_out_stats" - input and output degrees for each page
* "pageranks" - pagerank for each page (damping factor = 0.85)

Collaboration Policy
==========

I opensourced this code because I believe it can help people learn something new and improve their skills.
You can use it on your conscience, but I encourage you not to copy-paste this sources and use
them only for educational purposes :)
