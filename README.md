dotfiles
===

This repository contains my personal configuration for my system. It is designed to work with a macOS system but it should be compatible with UNIX too (I used to have it on a Arch Linux machine before adapting it for my mac).

It contains configuration for:

- brew|cask and pacman|yay
- zsh
- tmux
- vim and nvim (I switched to neovim since a couple of years)
- alacritty
- bookmarker
- git
- OSX (macOS)
- xorg (X11)

Symlinks are managed by `stow`.

## my prompt

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

## the bookmarker

This is a special html page with a bit of javascript in it. It serves me to speed up the process of opening web pages I access frequently for which only one part of the URL is dynamic (think about JIRA issues for instance "https://jira.company.tld/browse/1002").

#### How does it work?

- Add a bookmark on your browser pointing to: `file:///Users/matt/bookmarker.html#https://jira.company.tld/browse/PROJECT-$`
  1. link to `bookmarker.html` via `file://` protocol
  2. add destination URL as an anchor
  3. in the destination URL, you can use `$` as a placeholder for the substitution
- Visit the page with the particular anchor
- `bookmarker.html` will prompt the variable part in a dialog
- the variable part will be subsituted in the destination URL and you'll be redirected to it

# Installation

See `install.sh` and `uninstall.sh` for the (un)installation script (untested).
