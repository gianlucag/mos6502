" vimscript to convet hexdecimal array into apple1 'commandline'
" usage: in vim, type :source arr2hex.vim, and then select some lines
" in visual mode, then call Arr2Hex()
" the array must be merely a sequence of hexdecimal literals separated by
" commas. braces are not allowed.

function Arr2Hex()
	:normal 0v$10<
	:s/0x//g
	:s/,/ /g
	:normal 0i: 
endfunction
