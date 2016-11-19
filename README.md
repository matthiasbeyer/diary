# Simple CLI Diary

This is a text based diary, inspired by [khal](https://github.com/pimutils/khal). Diary entries are stored in raw text. You may say C & ncurses are old, I say paper is older..

![Diary Demo](https://raw.githubusercontent.com/in0rdr/diary/master/demo.gif)

## Usage
1. Set the EDITOR environment variable to your favourite text editor:
    ```
    export EDITOR=vim
    ```

2. Compile (requires ncurses):
    ```
    make clean; make;
    ```

3. Run the diary with the folder for the text files as first argument:
    ```
    ./diary ~/.diary
    ```

  The text files in this folder will be named 'yyyy-mm-dd'.

  (Optionally create an alias for convencience: `alias diary="~/.bin/diary ~/.diary")`

4. Use the keypad or VIM-like shortcuts to move between dates:

	e,enter	Edit the current entry
	t,n		Jump to today

	j		go forward by 1 week
	k		go backward by 1 week
	h		go left by 1 day
	l		go right by 1 day

	g		go to the first date
	G		go to the last date

	J		Go forward by 1 month
	K		Go backward by 1 month
