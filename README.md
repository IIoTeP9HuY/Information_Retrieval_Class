# Information Retrieval Course [![Build Status](https://travis-ci.org/IIoTeP9HuY/Information_Retrieval_Class.png?branch=master)](https://travis-ci.org/IIoTeP9HuY/Information_Retrieval_Class)

Information Retrieval YSDA course programming assignments repository
http://shad.yandex.ru/

Introduction
============

These are my solutions for programming exercises from YSDA Information Retrieval classes. Written in Python/C++.

Requirements
============
* gcc >= 4.8 or clang >= 3.4
* cmake
* boost >= 1.55
* glib2
* libglibmm-2.4
* libxml++2.6
* libtidy

Usage tutorial
============

Build everything using
```bash
    mkdir build && cd build
    cmake ..
    make
```

####Part1, Download wiki and analyze webgraph

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
and also create file urls with mapping 1.html`<tab>`url_1, ...

#####Strip hypertext tags and extract text from webpages
```bash
    extract --urlsDir flat_site --outDir text_wiki --urlsMapping urls
```
This will fill the folder "text_wiki" with files 1.txt, 2.txt, ... corresponding to extracted
text from 1.html, 2.html, ...
Also it will produce file token_frequency containing lines token_1`<tab>`frequency_1 ...

#####Finally, analyze webgraph
```bash
    flat_webgraph --path flat_site --urlMapping urls --domain http://simple.wikipedia.org/ --start_page http://simple.wikipedia.org/wiki/Main_Page
```
This will create files:
* "distances" - distance from start_page to every other page
* "in_out_stats" - input and output degrees for each page
* "pageranks" - pagerank for each page (damping factor = 0.85)

####Part2, Find duplicates among documents

Next steps allow you to find duplicates among downloaded pages using technique called "Simhashing".

#####Preprocess data a bit

During this step you should remove all frequent and wiki-specific words from documents.
There is more general ways to do it using word frequency and scoring, but in this case
it's easier to run simple sed command :)

```bash
    cp -r text_site text_site_clean && cd text_site_clean
    find . -type f -exec gsed -n -i "/Navigation menu/q;p" {} \;
```
This will remove all footers that start with string "Navigation menu" till the end of file.
That's where extracted wiki-specific words are located.

#####Build simhash signatures

Now we can build simhashes for documents
```bash
    Simhash -b --path=text_site_clean --dest=results
```
This will produce file "simhashes" with following format: **url**  **length\_in\_words**  **simhash**

For example: http://simple.wikipedia.org/wiki/Nathalia_Dill 383 11074093965332231517

#####Cluster documents:

To cluster documents we need to specify maximum allowed simhash distance between documents
in cluster. In this case we will use "-s 5", which means that documents that differ no more then
in 5 positions will be considered similar.
```bash
    Simhash -f -s 5 --dest=results
```
This will create files:
* "clusters_5" - containts list of found clusters
* "clusters_5_sizes" - contains sizes of found clusters

Collaboration Policy
==========

I opensourced this code because I believe it can help people learn something new and improve their skills.
You can use it on your conscience, but I encourage you not to copy-paste this sources and use
them only for educational purposes :)
