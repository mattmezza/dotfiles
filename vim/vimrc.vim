" Install vundle
	set nocompatible
	filetype off
	set rtp+=$HOME/dotfiles/vim/bundle/Vundle.vim
	call vundle#begin()

	Plugin 'VundleVim/Vundle.vim'
	Plugin 'Shougo/deoplete.nvim', { 'do': ':UpdateRemotePlugins' }
	" deoplete for python, go, docker, clang, tag, zsh, make
	Plugin 'deoplete-plugins/deoplete-jedi'
	Plugin 'deoplete-plugins/deoplete-go'
	Plugin 'deoplete-plugins/deoplete-docker'
	Plugin 'deoplete-plugins/deoplete-clang'
	Plugin 'deoplete-plugins/deoplete-tag'
	Plugin 'deoplete-plugins/deoplete-zsh'
	Plugin 'deoplete-plugins/deoplete-make'
	Plugin 'tpope/vim-fugitive'
	Plugin 'tpope/vim-surround'
	Plugin 'tpope/vim-repeat'
	Plugin 'scrooloose/nerdtree'
	Plugin 'scrooloose/syntastic'
	Plugin 'jistr/vim-nerdtree-tabs'
	" To use ack within vim
	Plugin 'mileszs/ack.vim'
	" Plugin 'ambv/black'
	Plugin 'ctrlpvim/ctrlp.vim'
	" To create tables with plain text
	Plugin 'dhruvasagar/vim-table-mode'
	" To create marp presentations
	Plugin 'dhruvasagar/vim-marp'
	" For an awesome status bar
	Plugin 'vim-airline/vim-airline'
	" Fancy bindings for rust
	Plugin 'rust-lang/rust.vim'
	" Syntax highlighting for major languages
	Plugin 'sheerun/vim-polyglot'
	" For auto insertion of pairs of ({["'
	Plugin 'jiangmiao/auto-pairs'
	" To use vim in tmux easily
	Plugin 'christoomey/vim-tmux-navigator'
	" Clipboard enhanced
	Plugin 'svermeulen/vim-easyclip'
	" The following shows vertical line for indentation
	Plugin 'Yggdroot/indentLine'

	call vundle#end()
	filetype plugin indent on
" General Vim settings
	syntax on
	let mapleader=","
	set autoindent
	set tabstop=4
	set shiftwidth=4
	set dir=/tmp/
	set relativenumber 
	set number
	set background=dark
	colorscheme slate

	autocmd Filetype html setlocal sw=2 expandtab
	autocmd Filetype javascript setlocal sw=4 expandtab

	set nocursorline
	hi Cursor ctermfg=White ctermbg=Yellow cterm=bold guifg=white guibg=yellow gui=bold

	set hlsearch
	nnoremap <C-l> :nohl<CR><C-l>:echo "Search Cleared"<CR>
	nnoremap <C-c> :set norelativenumber<CR>:set nonumber<CR>:echo "Line numbers turned off."<CR>
	nnoremap <C-n> :set relativenumber<CR>:set number<CR>:echo "Line numbers turned on."<CR>
	nnoremap <leader><Space> :MRU 

	nnoremap n nzzzv
	nnoremap N Nzzzv

	nnoremap H 0
	nnoremap L $
	nnoremap J G
	nnoremap K gg

	map <tab> %

	set backspace=indent,eol,start

	nnoremap <Space> za
	nnoremap <leader>z zMzvzz

	nnoremap vv 0v$

	set listchars=tab:\|\ 
	nnoremap <leader><tab> :set list!<cr>
	set pastetoggle=<F2>
	set mouse=a
	set incsearch

" Language Specific
	" Tabs
		so ~/dotfiles/vim/tabs.vim

	" General
		inoremap <leader>for <esc>Ifor (int i = 0; i < <esc>A; i++) {<enter>}<esc>O<tab>
		inoremap <leader>if <esc>Iif (<esc>A) {<enter>}<esc>O<tab>
		

	" Java
		inoremap <leader>sys <esc>ISystem.out.println(<esc>A);
		vnoremap <leader>sys yOSystem.out.println(<esc>pA);

	" Java
		inoremap <leader>con <esc>Iconsole.log(<esc>A);
		vnoremap <leader>con yOconsole.log(<esc>pA);

	" C++
		inoremap <leader>cout <esc>Istd::cout << <esc>A << std::endl;
		vnoremap <leader>cout yOstd::cout << <esc>pA << std:endl;

	" C
		inoremap <leader>out <esc>Iprintf(<esc>A);<esc>2hi
		vnoremap <leader>out yOprintf(, <esc>pA);<esc>h%a

	" Typescript
		autocmd BufNewFile,BufRead *.ts set syntax=javascript
		autocmd BufNewFile,BufRead *.tsx set syntax=javascript

	" Markup
		inoremap <leader>< <esc>I<<esc>A><esc>yypa/<esc>O<tab>


" File and Window Management 
	inoremap <leader>w <Esc>:w<CR>
	nnoremap <leader>w :w<CR>

	inoremap <leader>q <ESC>:q<CR>
	nnoremap <leader>q :q<CR>

	inoremap <leader>x <ESC>:x<CR>
	nnoremap <leader>x :x<CR>

	nnoremap <leader>e :Ex<CR>
	nnoremap <leader>t :tabnew<CR>:Ex<CR>
	nnoremap <leader>v :vsplit<CR>:w<CR>:Ex<CR>
	nnoremap <leader>s :split<CR>:w<CR>:Ex<CR>

" Return to the same line you left off at
	augroup line_return
		au!
		au BufReadPost *
			\ if line("'\"") > 0 && line("'\"") <= line("$") |
			\	execute 'normal! g`"zvzz' |
			\ endif
	augroup END

let g:pymode_python = 'python3'

" Auto load
	" Triger `autoread` when files changes on disk
	" https://unix.stackexchange.com/questions/149209/refresh-changed-content-of-file-opened-in-vim/383044#383044
	" https://vi.stackexchange.com/questions/13692/prevent-focusgained-autocmd-running-in-command-line-editing-mode
	autocmd FocusGained,BufEnter,CursorHold,CursorHoldI * if mode() != 'c' | checktime | endif
	set autoread 
	" Notification after file change
	" https://vi.stackexchange.com/questions/13091/autocmd-event-for-autoread
	autocmd FileChangedShellPost *
	  \ echohl WarningMsg | echo "File changed on disk. Buffer reloaded." | echohl None

" Future stuff
	"Swap line
	"Insert blank below and above

" Fix for: https://github.com/fatih/vim-go/issues/1509

filetype plugin indent on


" Some nice to have nerd tree config
	let g:nerdtree_tabs_autofind=1
	" let g:nerdtree_tabs_open_on_console_startup=1

" Alert when line too long
	augroup vimrc_autocmds
		autocmd FileType python highlight OverLength ctermbg=red ctermfg=white
		autocmd FileType python match OverLength /\%88v.\+/
	augroup END
	let g:syntastic_python_pylint_post_args="--max-line-length=88"

" add yaml stuff
	au! BufNewFile,BufReadPost *.{yaml,yml} set filetype=yaml
	autocmd FileType yaml setlocal ts=2 sts=2 sw=2 expandtab
