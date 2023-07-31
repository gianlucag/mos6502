most .txt and .bas files are from https://github.com/whscullin/apple1js/blob/main/tapes.
BASIC.txt is a hexdump of a1basic.bin
arr2hex.vim is a vimscript that helps to convert those tapes into Apple I 
commands. see arr2hex.vim for its usage.
to use these tapes, copy contents from those text files and paste it on
Apple I 'commandline'.
do not redirect these files to stdin. stdin will not switch back to
terminal automatically when it encounters EOF. the reason why I didn't
implement a feature to switch back to terminal is that it is impossible
redirect multiple files to stdin. you can only specify one file per 
execution, so why not rather paste files one by one by hand?
