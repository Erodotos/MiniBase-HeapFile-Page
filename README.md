# MiniBase-HeapFile-Page
In this repository you can find an implementation of the hfpage.c (heap file page) which is part of MiniBase. That was part of course Assignment on Databases. In the following lines you can find the assignment description.

##Introduction

In this assignment, you will implement the Page Structure for the Heap File layer. You will be given libraries for the Buffer Manager and Disk Space Manager, and some driver routines to test the code.

##Preliminary Work

You will be implementing just the HFPage class, and not all of the HeapFile code. Read the description in the text of how variable length records can be stored on a slotted page, and follow this page organization.

##Compiling Your Code and Running the Tests

Please Download the the files for Phase 2 from here phase2.tar.gz into your working directory.

cd project
tar -zxvf phase2.tar.gz
Now you will see 3 generated directories:
lib/
include/
src/

src/ contains the files you will be working on. If you cd src/ and then make the project, it will create an executable named hfpage . Right now, it does not work; you will need to fill in the bodies of the HFPage class methods. The methods are defined (empty) in file hfpage.C.
Sample output of a correct implementation is available in sample_output.

##Design Overview and Implementation Details

Have a look at the file hfpage.h in include/. It contains the interfaces for the HFPage class. This class implements a "heap-file page" object. Note that the protected data members of the page are given to you. All you should need to do is implement the public member functions. You should put all your code into the file src/hfpage.C.

![Heap File Representation](https://i.ibb.co/YyNFx7c/dire.gif)



