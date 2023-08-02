# C-Shell - A Feature-Rich Shell written in C

## Introduction

Welcome to C-Shell! This impressive shell is written in C and offers a plethora of features to enhance your command-line experience. Whether you're a beginner or an experienced user, C-Shell has got you covered with its powerful functionalities.

## Features

C-Shell comes packed with several **handy features**:

1. Basic Commands: C-Shell supports essential commands like `exit` and `cd`.

2. Forks and Child Processes: creates child processes using forks.

3. I/O Redirection: redirects input and output using `<`, `>`, and `>>` operators.
   
4. Jobs and Foreground: supports jobs management, allowing you to run commands in the background and bring them to the foreground as needed.

5. Multiple Piping: enables multiple piping. 

6. Signal Handling: can handle signals and exceptions. 

## Usage

To start using C-Shell, follow these steps:

1. Clone this repository to your local machine.
2. Compile the C code. 
3. Run ./nyush.c 


## Example

Here's an example of using C-Shell:

```bash
$ ./c-shell
C-Shell$ cd /path/to/directory
C-Shell$ ls > filelist.txt
C-Shell$ cat filelist.txt
file1.txt
file2.txt
file3.txt
C-Shell$ gcc program.c -o program
C-Shell$ ./program
Output of the program
C-Shell$ exit
$
```
