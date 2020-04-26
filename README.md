dotfiles
===

This repository contains my personal configuration for my system. It is designed to work with a macOS system but it should be compatible with UNIX too (I used to have it on a Arch Linux machine before adapting it for my mac).

It contains configuration for mainly:

- brew
- zsh
- tmux
- vim
- alacritty
- bookmarker
- git
- OSX (macOS)

## brew and cask

You can specify which packages and apps to install when you switch to a new system. It works with two files `brew.txt` and `cask.txt` (each line in the former will trigger a `brew install ...` and each line in the latter will trigger a `brew cask install ...`).


## zsh

I use zsh as my shell. It comes with a bunch of plugins I find useful and some aliases I am used to use. The customization in terms of style is mainly done via ohmyzsh.

### prompt

I like a very simple and short prompt.

Here's the shortest prompt you can get (from the home dir):
```
~ $ 
```

Here's instead the full prompt:

```
(venv) ~/dev/dir-a 127 a-very-looooooo...oooooong-branch+5 4s $
```

1. virtual environment, if active between parenthesis, colored
2. directory, colored
3. return code, if != 0, colored
4. git branch, length adjustable, shortened in the middle, colored
5. dirty files counter, colored
6. time that the previous command took, if greater than a customizable threshold, colored
7. prompt char (I like the $ sign for it, you can change it to > if you want)


## tmux

The configuration has been altered in style to make it sleek and flat, I added some keybindings and replaced the prefix from `C-b` to `C-a` because I care about my fingers. For the rest I tried to stick with the defaults (with some exceptions).

## vim

I configured it quite a bit, you can read the `.vimrc` to read the customization.

## alacritty

I only saved a slight customization in terms of font size and chosen font. Unfortunately, alacritty does not support ligatures yet, so sometimes I use iTerm 2. When the ligature support will be added I plan to fully switch to it.

## bookmarker

This is a special html page with a bit of javascript in it. It serves me to speed up the process of opening web pages I access frequently for which only one part of the URL is dynamic (think about JIRA issues for instance "https://jira.company.tld/browse/1002").

#### How does it work?

- Add a bookmark on your browser pointing to: `file:///Users/matt/dotfiles/bookmarker.html#https://jira.company.tld/browse/PROJECT-$`
  1. link to `bookmarker.html` via `file://` protocol
  2. add destination URL as an anchor
  3. in the destination URL, you can use `$` as a placeholder for the substitution
- Visit the page with the particular anchor
- `bookmarker.html` will prompt the variable part in a dialog
- the variable part will be subsituted in the destination URL and you'll be redirected to it

## git

I have defined some alias for recurring git commands, some of which are peculiar for me. I bet that some of them are also good for you though.

## OSX (macOS)

I like some specific settings to be set for my OS. This file helps me to set them all at once.


# Installation (untested)

Once the repo is cloned, execute the deploy script:
```
./deploy
```

This script guides you through the following:

1. Checks to see if you have zsh, tmux, and vim installed. 
2. Installs them using your default package manager if you don't have some of them installed.
3. Checks to see if your default shell is zsh.
4. Sets zsh to your default shell.
5. Backs up your old configuration files.

Pretty convenient for configuring new servers.

## Basic runtime opperations

Upon launching a new shell, `zsh` first launches tmux. The session is retained even if the terminal crashes or if you force-quit it.
