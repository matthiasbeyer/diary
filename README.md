# Simple CLI Diary

This is a text based diary, inspired by [khal](https://github.com/pimutils/khal). Diary entries are stored in raw text. You may say C & ncurses are old, I say paper is older..

## Usage
1. Set the EDITOR environment variable to your favourite text editor:
    ```
    export EDITOR=vim
    ```
    
2. Compile the diary with ncurses library:
    ```
    gcc diary.c -o diary -lncurses -std=gnu11
    ```
    
3. Run the diary with the folder for the text files as first argument:
    ```
    ./diary ~/.diary
    ```
    
  The text files in this folder will be named 'yyyy-mm-dd'.
  
  (Optionally create an alias for convencience: `alias diary="~/.bin/diary ~/.diary")`

