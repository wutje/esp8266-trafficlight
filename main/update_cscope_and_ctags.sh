#/bin/bash
cscope -Rb -s . -s $IDF_PATH
ctags -R . $IDF_PATH
