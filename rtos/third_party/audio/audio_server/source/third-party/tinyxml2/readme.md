# TinyXML-2

TinyXML-2 is a simple, small, efficient, C++ XML parser that can be easily integrated into other programs.

The master is hosted on github: https://github.com/leethomason/tinyxml2

The online HTML version of these docs: http://leethomason.github.io/tinyxml2/

Examples are in the "related pages" tab of the HTML docs.

## Code Page

TinyXML-2 uses UTF-8 exclusively when interpreting XML. All XML is assumed to be UTF-8.

Filenames for loading / saving are passed unchanged to the underlying OS.

## Memory Model

An XMLDocument is a C++ object like any other, that can be on the stack, or new'd and deleted on the heap.

However, any sub-node of the Document, XMLElement, XMLText, etc, can only be created by calling the appropriate XMLDocument::NewElement, NewText, etc. method. Although you have pointers to these objects, they are still owned by the Document. When the Document is deleted, so are all the nodes it contains.

## Using and Installing

There are 2 files in TinyXML-2:
- tinyxml2.cpp
- tinyxml2.h

And additionally a test file:
- xmltest.cpp

## License

TinyXML-2 is released under the zlib license