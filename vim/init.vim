" Install vundle
set nocompatible
filetype off
set rtp+=$DOT/vim/bundle/Vundle.vim
call vundle#begin()

Plugin 'VundleVim/Vundle.vim'
Plugin 'bogado/file-line'
Plugin 'christoomey/vim-tmux-navigator'
Plugin 'ctrlpvim/ctrlp.vim'
Plugin 'dhruvasagar/vim-marp' " marp presentations
Plugin 'dhruvasagar/vim-table-mode' " tables with plain text
Plugin 'leafgarland/typescript-vim'
Plugin 'pangloss/vim-javascript'
Plugin 'Quramy/vim-js-pretty-template'
Plugin 'othree/javascript-libraries-syntax.vim'
Plugin 'othree/yajs.vim'
Plugin 'neoclide/coc.nvim', {'branch': 'release'}
Plugin 'rust-lang/rust.vim'
Plugin 'sheerun/vim-polyglot'
Plugin 'tpope/vim-fugitive'
Plugin 'tpope/vim-repeat'
Plugin 'tpope/vim-surround'
Plugin 'kburdett/vim-nuuid'
Plugin 'vim-airline/vim-airline'
Plugin 'yggdroot/indentLine' " vertical line for indentation
Plugin 'zirrostig/vim-schlepp'
Plugin 'editorconfig/editorconfig-vim'
Plugin 'alvan/vim-closetag'
Plugin 'morhetz/gruvbox'
Plugin 'chriskempson/base16-vim'
Plugin 'dawikur/base16-vim-airline-themes'
if has('nvim') || has('patch-8.0.902')
  Plugin 'mhinz/vim-signify'
else
  Plugin 'mhinz/vim-signify', { 'branch': 'legacy' }
endif
Plugin 'mhinz/vim-grepper', { 'on': ['Grepper', '<plug>(GrepperOperator)'] }

call vundle#end()
filetype plugin indent on

" General Vim settings
set nomodeline
syntax on
let mapleader=","
set dir=/tmp/
set relativenumber
set number
set termguicolors
colorscheme base16-3024

hi Cursor ctermfg=White ctermbg=Yellow cterm=bold guifg=white guibg=yellow gui=bold
set guicursor= " cursor stays a block even in insert mode

nnoremap <C-_> :nohl<CR>:echo "Search Cleared"<CR>
nnoremap <leader><Space> :MRU

nnoremap n nzzzv
nnoremap N Nzzzv

nnoremap H 0
nnoremap L $
nnoremap J G
nnoremap K gg
nnoremap ; :

set backspace=indent,eol,start

nnoremap <leader>z zMzvzz

nnoremap vv 0v$

set listchars=tab:\|\ 
nnoremap <leader><tab> :set list!<cr>
set pastetoggle=<F2>
set mouse-=a

"=====[ Load some useful plugins ] =================================

runtime automkdir.vim
runtime vmath.vim
runtime dragvisuals.vim
runtime comment_toggle.vim
runtime arrownavigation.vim
runtime blockwise.vim
runtime colalignsimple.vim
runtime eqalignsimple.vim
runtime fillabbr.vim
runtime foldsearches.vim
runtime normalized_search.vim
runtime persitentvisuals.vim
runtime todo_support.vim
runtime warmmargin.vim
runtime yankmatches.vim

"=====[ Convert to Unicode defaults ]===============================

setglobal termencoding=utf-8 fileencodings=
scriptencoding utf-8
set encoding=utf-8

autocmd BufNewFile,BufRead  *   try
autocmd BufNewFile,BufRead  *		 set encoding=utf-8
autocmd BufNewFile,BufRead  *   endtry

"=====[ A bit of netrw config ]=======================

let g:netrw_banner=0
" hide hidden files by default
" let g:netrw_list_hide = '\(^\|\s\s\)\zs\.\S\+,\(^\|\s\s\)ntuser\.\S\+'
" show hidden files by default
let g:netrw_list_hide = '\(^\|\s\s\)ntuser\.\S\+'
let g:netrw_keepdir=0
autocmd FileType netrw set nolist
" - to open up an explorer in current buffer
nnoremap - :Exp<CR>

"=====[ Skip stuff in ctrlp ]==========================

let g:ctrlp_custom_ignore = 'node_modules\|DS_Store\|git|__pycache'

"=====[ Enable Nmap command for documented mappings ]================

runtime documap.vim

"=====[ Comments are important ]==================

highlight Comment term=bold cterm=italic ctermfg=white gui=italic guifg=white

"====[ Escape insert mode via 'jj' ]=============================

imap jj <ESC>

"====[ Change line number bg ]=======================
highlight clear LineNr
highlight clear SignColumn

"====[ Mapping that bloody :W to :w ]=================

cmap Wq wq
cmap WQ wq
cmap wQ wq

"====[ Edit and auto-update this config file and plugins ]==========

augroup VimReload
autocmd!
    autocmd BufWritePost $MYVIMRC source $MYVIMRC
augroup END

Nmap <silent>  ;v   [Edit .vimrc]          :next $MYVIMRC<CR>
Nmap           ;vv  [Edit .vim/plugin/...] :next $HOME/dotfiles/vim/plugin/

"=====[ Some of Vim's defaults are just annoying ]============

" :read and :write shouldn't set #
set cpo-=aA

" Make /g the default on :s/.../../ commands (use /gg to disable)
"set gdefault

" Prefer vertical orientation when using :diffsplit
set diffopt+=vertical

"====[ I'm sick of typing :%s/.../.../g ]=======

Nmap S  [Shortcut for :s///g]  :%s//g<LEFT><LEFT>
xmap S                         :s//g<LEFT><LEFT>

Nmap <expr> M  [Shortcut for :s/<last match>//g]  ':%s/' . @/ . '//g<LEFT><LEFT>'
xmap <expr> M                                     ':s/' . @/ . '//g<LEFT><LEFT>'


"====[ Toggle visibility of naughty characters ]============

" Make naughty characters visible...
" (uBB is right double angle, uB7 is middle dot)
set lcs=tab:»·,trail:␣,nbsp:˷
highlight InvisibleSpaces ctermfg=Black ctermbg=Black
call matchadd('InvisibleSpaces', '\S\@<=\s\+\%#\ze\s*$')

augroup VisibleNaughtiness
    autocmd!
    autocmd BufEnter  *		set list
    autocmd BufEnter  *		set list
    autocmd BufEnter  *.txt	set nolist
    autocmd BufEnter  *.vp*	set nolist
    autocmd BufEnter  *		if !&modifiable
    autocmd BufEnter  *		    set nolist
    autocmd BufEnter  *		endif
augroup END


"====[ Set up smarter search behaviour ]=======================

set incsearch		"Lookahead as search pattern is specified
set ignorecase		"Ignore case in all searches...
set smartcase		"...unless uppercase letters used

set hlsearch		"Highlight all matches
"highlight clear Search
highlight Search ctermfg=White  ctermbg=Red  cterm=bold
highlight IncSearch ctermfg=White  ctermbg=Red cterm=bold

" Absolute direction for n and N...
runtime hlnext.vim
nnoremap	<silent><expr> n  'Nn'[v:searchforward] . ":call HLNext()\<CR>"
nnoremap	<silent><expr> N  'nN'[v:searchforward] . ":call HLNext()\<CR>"

"=======[ Fix smartindent stupidities ]============

set autoindent						    "Retain indentation on next line
set smartindent					    "Turn on autoindenting of blocks

let g:vim_indent_cont = 0			    " No magic shifts on Vim line continuations

"And no shift magic on comments...
nmap <silent>	>>  <Plug>ShiftLine
nnoremap <Plug>ShiftLine :call ShiftLine()<CR>
function! ShiftLine() range
    set nosmartindent
    exec "normal! " . v:count . ">>"
    set smartindent
    silent! call repeat#set( "\<Plug>ShiftLine" )
endfunction


"====[ I hate modelines ]===================

set modelines=0

"=====[ Make Visual modes work better ]==================

" Visual Block mode is far more useful that Visual mode (so swap the commands)...
nnoremap v <C-V>
nnoremap <C-V> v

xnoremap v <C-V>
xnoremap <C-V> v

"Square up visual selections...
set virtualedit=block

" Make BS/DEL work as expected in visual modes (i.e. delete the selected text)...
xmap <BS> x

" Make vaa select the entire file...
xmap aa VGo1G

let s:closematch = [ '', '', '}', ']', ')', '>', '/', "'", '"', '`' ]
let s:ldelim = '\< \%(q [qwrx]\= \| [smy] \| tr \) \s*
\			 \%(
\				\({\) \| \(\[\) \| \((\) \| \(<\) \| \(/\)
\			 \)
\			 \|
\				\(''\) \| \("\) \| \(`\)
\'
let s:ldelim = substitute(s:ldelim, '\s\+', '', 'g')

function! ExtendVisualString ()
    let [lline, lcol, lmatch] = searchpos(s:ldelim, 'bWp')
    if lline == 0
	   return
    endif
    let rdelim = s:closematch[lmatch]
    normal `>
    let rmatch = searchpos(rdelim, 'W')
    normal! v
    call cursor(lline, lcol)
endfunction

"=====[ Make arrow keys move visual blocks around ]======================

xmap <up>    <Plug>SchleppUp
xmap <down>  <Plug>SchleppDown
xmap <left>  <Plug>SchleppLeft
xmap <right> <Plug>SchleppRight

xmap D	   <Plug>SchleppDupLeft
xmap <C-D>   <Plug>SchleppDupLeft    

"=====[ Configure % key (via matchit plugin) ]==============================

" Match angle brackets...
set matchpairs+=<:>,«:»,｢:｣

"=====[ Miscellaneous features (mainly options) ]=====================

set title			"Show filename in titlebar of window
set titleold=
"set titlestring=%t%(\ %M%)%(\ (%{expand(\"%:~:.:h\")})%)%(\ %a%)
set title titlestring=

set nomore		"Don't page long listings

set cpoptions-=a	"Don't set # after a :read

set autowrite		"Save buffer automatically when changing files
set autoread		"Always reload buffer when external changes detected

"		  +--Disable hlsearch while loading viminfo
"		  | +--Remember marks for last 500 files
"		  | |    +--Remember up to 10000 lines in each register
"		  | |    |	 +--Remember up to 1MB in each register
"		  | |    |	 |	  +--Remember last 1000 search patterns
"		  | |    |	 |	  |	   +---Remember last 1000 commands
"		  | |    |	 |	  |	   |
"		  v v    v	 v	  v	   v
set viminfo=h,'500,<10000,s1000,/1000,:1000

set backspace=indent,eol,start	 "BS past autoindents, line boundaries,
							 "	  and even the start of insertion

set fileformats=unix,mac,dos		 "Handle Mac and DOS line-endings
							 "but prefer Unix endings

set wildignorecase				 "Case-insensitive completions

set wildmode=list:longest,full	 "Show list of completions
							 "  and complete as much as possible,
							 "  then iterate full completions

set complete-=t				 " I don't use tags, so no need to search for them

set infercase					 "Adjust completions to match case

set noshowmode					 "Suppress mode change messages

set updatecount=10				 "Save buffer every 10 chars typed


" Keycodes and maps timeout in 3/10 sec...
set timeout timeoutlen=300  " for fast leader response
set ttimeoutlen=10          " for fast escape key

" "idleness" is 2 sec
set updatetime=2000

" set thesaurus+=~/Documents/thesaurus	  "Add thesaurus file for ^X^T
" set dictionary+=~/Documents/dictionary  "Add dictionary file for ^X^K


set scrolloff=2				 "Scroll when 3 lines from top/bottom

"=====[ Remap various keys to something more useful ]========================

" Use space to jump down a page (like browsers do)...
nnoremap	 <Space> <PageDown>
xnoremap	 <Space> <PageDown>

" Indent/outdent current block...
nmap %% $>i}``
nmap $$ $<i}``

"=====[ Convert file to different tabspacings ]=====================

function! InferTabspacing ()
    return min(filter(map(getline(1,'$'),'strlen(matchstr(v:val, ''^\s\+''))'),'v:val != 0'))
endfunction

function! NewTabSpacing (newtabsize)
    " Determine apparent tabspacing, if necessary...
    if &tabstop == 4
	   let &tabstop = InferTabspacing()
    endif

    " Preserve expansion, if expanding...
    let was_expanded = &expandtab

    " But convert to tabs initially...
    normal TT

    " Change the tabsizing...
    execute "set ts="  . a:newtabsize
    execute "set sw="  . a:newtabsize

    " Update the formatting commands to mirror than new tabspacing...
    execute "map		 F !Gformat -T" . a:newtabsize . " -"
    execute "map <silent> f !Gformat -T" . a:newtabsize . "<CR>"

    " Re-expand, if appropriate...
    if was_expanded
	   normal TS
    endif
endfunction

" Note, these are all T-<SHIFTED-DIGIT>, which is easier to type...
nmap <silent> T@ :call NewTabSpacing(2)<CR>
nmap <silent> T# :call NewTabSpacing(3)<CR>
nmap <silent> T$ :call NewTabSpacing(4)<CR>
nmap <silent> T% :call NewTabSpacing(5)<CR>
nmap <silent> T^ :call NewTabSpacing(6)<CR>
nmap <silent> T& :call NewTabSpacing(7)<CR>
nmap <silent> T* :call NewTabSpacing(8)<CR>
nmap <silent> T( :call NewTabSpacing(9)<CR>

" Convert to/from spaces/tabs...
nmap <silent> TS :set   expandtab<CR>:%retab!<CR>
nmap <silent> TT :set noexpandtab<CR>:%retab!<CR>
nmap <silent> TF TST$

"=====[ Correct common mistypings in-the-fly ]=======================

iab    retrun  return
iab     pritn  print
iab       teh  the
iab      liek  like
iab  liekwise  likewise
iab      Pelr  Perl
iab      pelr  perl
iab        ;t  't
iab    Jarrko  Jarkko
iab    jarrko  jarkko
iab      moer  more
iab  previosu  previous


"=====[ Tab handling ]======================================

set tabstop=4      "Tab indentation levels every four columns
set shiftwidth=4   "Indent/outdent by four columns
set expandtab      "Convert all tabs that are typed into spaces
set shiftround     "Always indent/outdent to nearest tabstop
set smarttab       "Use shiftwidths at left margin, tabstops everywhere else

autocmd Filetype html setlocal sw=2 expandtab
autocmd Filetype javascript setlocal sw=2 expandtab

" Make the completion popup look menu-ish on a Mac...
highlight  Pmenu        ctermbg=white   ctermfg=black
highlight  PmenuSel     ctermbg=blue    ctermfg=white   cterm=bold
highlight  PmenuSbar    ctermbg=grey    ctermfg=grey
highlight  PmenuThumb   ctermbg=blue    ctermfg=blue

" Make diffs less glaringly ugly...
highlight DiffAdd     cterm=bold ctermfg=green     ctermbg=black
highlight DiffChange  cterm=bold ctermfg=grey      ctermbg=black
highlight DiffDelete  cterm=bold ctermfg=black     ctermbg=black
highlight DiffText    cterm=bold ctermfg=magenta   ctermbg=black

"=====[ Highlight cursor ]===================

" Inverse highlighting for cursor...
highlight CursorInverse ctermfg=black ctermbg=white

"====[ Make digraphs easier to get right (various versions) ]=================

"inoremap <expr>  <C-J>       HUDG_GetDigraph()
runtime betterdigraphs_utf8.vim
inoremap <expr>  <C-K>       BDG_GetDigraph()
"inoremap <expr>  <C-L>       HUDigraphs()

function! HUDigraphs ()
    digraphs
    call getchar()
    return "\<C-K>"
endfunction

"====[ Extend a previous match ]=====================================

nnoremap //   /<C-R>/
nnoremap ///  /<C-R>/\<BAR>

"====[ Toggle between lists and bulleted lists ]======================

runtime listtrans.vim
Nmap     <silent> ;l [Toggle list format (bullets <-> commas)]  :call ListTrans_toggle_format()<CR>f
xnoremap <silent> ;l                                            :call ListTrans_toggle_format('visual')<CR>f

"=====[ Select a table column in visual mode ]========================
runtime tablecellselect.vim
xnoremap <silent><expr> c  VTC_select()

"=====[ Make * respect smartcase and also set @/ (to enable 'n' and 'N') ]======

nmap *  :let @/ = '\<'.expand('<cword>').'\>' ==? @/ ? @/ : '\<'.expand('<cword>').'\>'<CR>n

"======[ Fix colourscheme for 256 colours ]============================

highlight Visual       ctermfg=Yellow ctermbg=26    " 26 = Dusty blue background
highlight SpecialKey   cterm=bold ctermfg=Blue

"======[ Tweak highlighted yank plugin ]====================================

highlight HighlightedyankRegion cterm=NONE ctermfg=white ctermbg=darkyellow

let g:highlightedyank_highlight_duration = -1

let g:highlightedyank_quench_when = [ ['CursorMoved', '<buffer>'] ]

"======[ Add a Y command for incremental yank in Visual mode ]==============

xnoremap <silent>       Y   <ESC>:silent let @y = @"<CR>gv"Yy:silent let @" = @y<CR>
nnoremap <silent>       YY  :call Incremental_YY()<CR>
nnoremap <silent><expr> Y         Incremental_Y()

function! Incremental_YY () range
    let @" .= join(getline(a:firstline, a:lastline), "\n") . "\n"
endfunction

function! Incremental_Y ()
    let motion = nr2char(getchar())
    if motion == 'Y'
        call Incremental_YY()
        return
    elseif motion =~ '[ia]'
        let motion .= nr2char(getchar())
    elseif motion =~ '[/?]'
        let motion .= input(motion) . "\<CR>"
    endif

    let @y = @"
    return '"Yy' . motion . ':let @" = @y' . "\<CR>"
endfunction


"======[ Add a $$ command in Visual mode ]==============================

xmap     <silent>       ]   $"yygv_$
xnoremap <silent><expr> _$  Under_dollar_visual()

function! Under_dollar_visual ()
    " Locate block being shifted...
    let maxcol = max(map(split(@y, "\n"), 'strlen(v:val)')) + getpos("'<")[2] - 2

    " Return the selection that does the job...
    return maxcol . '|'
endfunction

"=====[ ,, as -> without delays ]===================

inoremap <expr><silent>  ,  Smartcomma()

function! Smartcomma ()
    let [bufnum, lnum, col, off, curswant] = getcurpos()
    if getline('.') =~ (',\%' . (col+off) . 'c')
        return "\<C-H>->"
    else
        return ','
    endif
endfunction


"=====[ Interface with ag ]======================

set grepprg=ag\ --vimgrep\ $*
set grepformat=%f:%l:%c:%m

" Also use ag in GVI...
let g:GVI_use_ag = 1

"======[ Breakindenting ]========

set breakindentopt=shift:2,sbr
set showbreak=↪
set breakindent
set linebreak

"=====[ Let <UP> and <DOWN> iterate the quickfix buffer list too ]=========

let g:ArrNav_arglist_fallback = 1


"=====[ Blockwise mode on : in visual mode ]===============================

let g:Blockwise_autoselect = 1

"=====[ Make jump-selections work better in visual block mode ]=================

xnoremap <expr>  G   'G' . virtcol('.') . "\|"
xnoremap <expr>  }   '}' . virtcol('.') . "\|"
xnoremap <expr>  {   '{' . virtcol('.') . "\|"

"=====[ Bracketed paste mode ]=======================================

if &term =~ "xterm.*"
    let &t_ti = &t_ti . "\e[?2004h"
    let &t_te = "\e[?2004l" . &t_te

    function! XTermPasteBegin(ret)
        set pastetoggle=<Esc>[201~
        set paste
        return a:ret
    endfunction

    map <expr> <Esc>[200~ XTermPasteBegin("i")
    imap <expr> <Esc>[200~ XTermPasteBegin("")
    xmap <expr> <Esc>[200~ XTermPasteBegin("c")

    cmap        <Esc>[200~ <nop>
    cmap        <Esc>[201~ <nop>
endif

"=====[ Completion during search (via Command window) ]======================

function! s:search_mode_start()
    cnoremap <tab> <c-f>:resize 1<CR>a<c-n>
    let s:old_complete_opt = &completeopt
    let s:old_last_status = &laststatus
    set completeopt-=noinsert
    set laststatus=0
endfunction

function! s:search_mode_stop()
    try
        silent cunmap <tab>
    catch
    finally
        let &completeopt = s:old_complete_opt
        let &laststatus  = s:old_last_status
    endtry
endfunction

augroup SearchCompletions
    autocmd!
    autocmd CmdlineEnter [/\?] call <SID>search_mode_start()
    autocmd CmdlineLeave [/\?] call <SID>search_mode_stop()
augroup END


"=====[ Make multi-selection incremental search prettier ]======================

augroup SearchIncremental
    autocmd!
    autocmd CmdlineEnter [/\?]   highlight  Search  ctermfg=DarkRed   ctermbg=Black cterm=NONE
    autocmd CmdlineLeave [/\?]   highlight  Search  ctermfg=White ctermbg=Black cterm=bold
augroup END

"=====[ Configure table-mode ]=================================================

let g:table_mode_corner                 = '|'
let g:table_mode_corner_corner          = '|'
let g:table_mode_header_fillchar        = '='
let g:table_mode_fillchar               = '-'
let g:table_mode_align_char             = ':'
let g:table_mode_cell_text_object_a_map = 'ac'
let g:table_mode_cell_text_object_i_map = 'ic'
let g:table_mode_syntax                 = 1
let g:table_mode_delimiter              = ' \{2,}'

" nmap <TAB> :TableModeToggle<CR>
xmap <TAB> <ESC><TAB>gv
xmap <silent> T :<C-U>call ToggleTabularization()<CR>

function! ToggleTabularization ()
    let range = getpos('''<')[1] .','. getpos('''>')[1]
    if getline("'<") =~ '\\\@!|'
        silent TableModeEnable
        exec 'silent! ' . range . 's/[-= ]\@<=+\|+[-= ]\@=/  /g'
        exec 'silent! ' . range . 's/[-= ]|[-= ]\|[^\\]\zs|[-= ]\|[-= ]|/  /g'
        exec 'silent! ' . range . 's/\s\+$//'
        nohlsearch
        TableModeDisable
    else
        TableModeEnable
        '<,'>Tableize
    endif
    normal gv
endfunction


"=======[ Prettier tabline ]============================================

highlight Tabline      cterm=underline       ctermfg=40     ctermbg=22
highlight TablineSel   cterm=underline,bold  ctermfg=white  ctermbg=28
highlight TablineFill  cterm=NONE            ctermfg=black  ctermbg=black

"=======[ Swap <C-A> and g<C-A>, improve <C-A>, and persist in visual mode ]============

xnoremap   <C-A>   g<C-A>gv<C-X>gv
xnoremap  g<C-A>    <C-A>gv


"=======[ Make literal spaces match any whitespace in searches ]============

cnoremap <C-M> <C-\>e('/?' =~ getcmdtype() ? substitute(getcmdline(), '\\\@<! ', '\\_s\\+', 'g') : getcmdline())<CR><CR>



" Language Specific
" Tabs
so ~/dotfiles/vim/tabs.vim

" Typescript
autocmd BufNewFile,BufRead *.ts set syntax=javascript
autocmd BufNewFile,BufRead *.tsx set syntax=javascript

" File and Window Management
inoremap <leader>w <Esc>:w<CR>
nnoremap <leader>w :w<CR>

inoremap <leader>q <ESC>:q<CR>
nnoremap <leader>q :q<CR>

nnoremap <leader>t :Tex<CR>
nnoremap <leader>v :Vex<CR>

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
" Triggers `autoread` when files changes on disk
" https://unix.stackexchange.com/questions/149209/refresh-changed-content-of-file-opened-in-vim/383044#383044
" https://vi.stackexchange.com/questions/13692/prevent-focusgained-autocmd-running-in-command-line-editing-mode
autocmd FocusGained,BufEnter,CursorHold,CursorHoldI * if mode() != 'c' | checktime | endif
set autoread 
" Notification after file change
" https://vi.stackexchange.com/questions/13091/autocmd-event-for-autoread
autocmd FileChangedShellPost *
    \ echohl WarningMsg | echo "File changed on disk. Buffer reloaded." | echohl None

" Fix for: https://github.com/fatih/vim-go/issues/1509
filetype plugin indent on

" Fix for editorconfig-vim & fugitive
let g:EditorConfig_exclude_patterns = ['fugitive://.*']

" Alert when line too long
highlight ColorColumn ctermbg=magenta
call matchadd('ColorColumn', '\%80v', 100)

"=====[ Toggle between angular ts and html]============
nnoremap nh :e %<.html<CR>
nnoremap nt :e %<.ts<CR>
nnoremap nc :e %<.css<CR>
nnoremap ns :e %<.spec.ts<CR>
nnoremap nl :e %<.less<CR>

" Custom syntastic settings
let g:syntastic_python_pylint_post_args="--max-line-length=79"
let g:syntastic_check_on_open = 1
let g:syntastic_check_on_wq = 0

" add yaml stuff
au! BufNewFile,BufReadPost *.{yaml,yml} set filetype=yaml
autocmd FileType yaml setlocal ts=2 sts=2 sw=2 expandtab

" CoC
" if hidden is not set, TextEdit might fail.
set hidden
hi CocWarningSign ctermfg=Gray guifg=#555555

let g:coc_config_home="$HOME/dotfiles/vim"
let g:airline#extensions#coc#enabled=1
let b:coc_root_patterns = ['.git']

" Some servers have issues with backup files, see #649
set nobackup
set nowritebackup

" Better display for messages
set cmdheight=2

" You will have bad experience for diagnostic messages when it's default 4000.
set updatetime=300

" don't give |ins-completion-menu| messages.
set shortmess+=c

" always show signcolumns
set signcolumn=yes

" Use tab for trigger completion with characters ahead and navigate.
" Use command ':verbose imap <tab>' to make sure tab is not mapped by other plugin.
inoremap <silent><expr> <TAB>
     \ pumvisible() ? "\<C-n>" :
     \ <SID>check_back_space() ? "\<TAB>" :
     \ coc#refresh()
inoremap <expr><S-TAB> pumvisible() ? "\<C-p>" : "\<C-h>"

function! s:check_back_space() abort
  let col = col('.') - 1
  return !col || getline('.')[col - 1]	=~# '\s'
endfunction

" Use <c-space> to trigger completion.
inoremap <silent><expr> <c-space> coc#refresh()

" Use <cr> to confirm completion, `<C-g>u` means break undo chain at current position.
" Coc only does snippet and additional edit on confirm.
inoremap <expr> <cr> pumvisible() ? "\<C-y>" : "\<C-g>u\<CR>"
" Or use `complete_info` if your vim support it, like:
" inoremap <expr> <cr> complete_info()["selected"] != "-1" ? "\<C-y>" : "\<C-g>u\<CR>"

" Use `[g` and `]g` to navigate diagnostics
nmap <silent> [g <Plug>(coc-diagnostic-prev)
nmap <silent> ]g <Plug>(coc-diagnostic-next)

" Remap keys for gotos
nmap <silent> gd <Plug>(coc-definition)
nmap <silent> gy <Plug>(coc-type-definition)
nmap <silent> gi <Plug>(coc-jmplementation)
nmap <silent> gr <Plug>(coc-references)
nmap <silent> gh <Plug>(coc-float-hide)

" Use K to show documentation in preview window
nnoremap <silent> K :call <SID>show_documentation()<CR>

function! s:show_documentation()
  if (index(['vim','help'], &filetype) >= 0)
    execute 'h '.expand('<cword>')
  else
    call CocAction('doHover')
  endif
endfunction

" Highlight symbol under cursor on CursorHold
autocmd CursorHold * silent call CocActionAsync('highlight')

" Remap for rename current word
nmap <leader>rn <Plug>(coc-rename)

" Remap for format selected region
xmap <leader>f  <Plug>(coc-format-selected)
nmap <leader>f  <Plug>(coc-format-selected)

augroup mygroup
  autocmd!
  " Setup formatexpr specified filetype(s).
  autocmd FileType typescript,json setl formatexpr=CocAction('formatSelected')
  " Update signature help on jump placeholder
  autocmd User CocJumpPlaceholder call CocActionAsync('showSignatureHelp')
augroup end

" Remap for do codeAction of selected region, ex: `<leader>aap` for current paragraph
xmap <leader>aa  <Plug>(coc-codeaction-selected)
nmap <leader>aa  <Plug>(coc-codeaction-selected)

" Remap for do codeAction of current line
nmap <leader>ac  <Plug>(coc-codeaction)
" Fix autofix problem of current line
nmap <leader>qf  <Plug>(coc-fix-current)

" Create mappings for function text object, requires document symbols feature of languageserver.
xmap if <Plug>(coc-funcobj-i)
omap if <Plug>(coc-funcobj-i)
xmap af <Plug>(coc-funcobj-a)
omap af <Plug>(coc-funcobj-a)
xmap ic <Plug>(coc-classobj-i)
omap ic <Plug>(coc-classobj-i)
xmap ac <Plug>(coc-classobj-a)
omap ac <Plug>(coc-classobj-a)

" Use <TAB> for select selections ranges, needs server support, like: coc-tsserver, coc-python
" nmap <silent> <TAB> <Plug>(coc-range-select)
" xmap <silent> <TAB> <Plug>(coc-range-select)

" Use `:Format` to format current buffer
command! -nargs=0 Format :call CocAction('format')

" Use `:Fold` to fold current buffer
command! -nargs=? Fold :call	   CocAction('fold', <f-args>)

" use `:OR` for organize import of current buffer
command! -nargs=0 OR   :call	   CocAction('runCommand', 'editor.action.organizeImport')

" Add status line support, for integration with other plugin, checkout `:h coc-status`
" set statusline^=%{coc#status()}%{get(b:,'coc_current_function','')}

" Using CocList
" Show all diagnostics
nnoremap <silent> <space>a  :<C-u>CocList diagnostics<cr>
" Manage extensions
nnoremap <silent> <space>e  :<C-u>CocList extensions<cr>
" Show commands
nnoremap <silent> <space>c  :<C-u>CocList commands<cr>
" Find symbol of current document
nnoremap <silent> <space>o  :<C-u>CocList outline<cr>
" Search workspace symbols
nnoremap <silent> <space>s  :<C-u>CocList -I symbols<cr>
" Do default action for next item.
nnoremap <silent> <space>j  :<C-u>CocNext<CR>
" Do default action for previous item.
nnoremap <silent> <space>k  :<C-u>CocPrev<CR>
" Resume latest coc list
nnoremap <silent> <space>p  :<C-u>CocListResume<CR>
" Search
nnoremap <leader>s :CocSearch 

" ====[ vim-airline customization ] ===========
let g:airline_symbols_ascii = 1
let g:airline#parts#ffenc#skip_expected_string='utf-8[unix]'
let g:airline#extensions#whitespace#enabled = 0
let g:airline#extensions#branch#enabled = 0

" ====[ signify customization ] ===============
set updatetime=100
