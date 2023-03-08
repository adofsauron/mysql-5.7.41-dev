
#!/bin/bash

cd mysql-5.7.41

clang-format -style=file -i   ./dbug/*.c
clang-format -style=file -i   ./include/*.h

clang-format -style=file -i   ./sql/*.h
clang-format -style=file -i   ./sql/*.cc


clang-format -style=file -i ./storage/innobase/btr/*.h                  
clang-format -style=file -i ./storage/innobase/buf/*.h  
clang-format -style=file -i ./storage/innobase/data/*.h  
clang-format -style=file -i ./storage/innobase/dict/*.h  
clang-format -style=file -i ./storage/innobase/dyn/*.h  
clang-format -style=file -i ./storage/innobase/eval/*.h  
clang-format -style=file -i ./storage/innobase/fil/*.h  
clang-format -style=file -i ./storage/innobase/fsp/*.h  
clang-format -style=file -i ./storage/innobase/fut/*.h  
clang-format -style=file -i ./storage/innobase/ha/*.h  
clang-format -style=file -i ./storage/innobase/ibuf/*.h  
clang-format -style=file -i ./storage/innobase/include/*.h  
clang-format -style=file -i ./storage/innobase/lock/*.h  
clang-format -style=file -i ./storage/innobase/log/*.h  
clang-format -style=file -i ./storage/innobase/mach/*.h  
clang-format -style=file -i ./storage/innobase/mem/*.h  
clang-format -style=file -i ./storage/innobase/mtr/*.h  
clang-format -style=file -i ./storage/innobase/os/*.h  
clang-format -style=file -i ./storage/innobase/page/*.h  
clang-format -style=file -i ./storage/innobase/pars/*.h  
clang-format -style=file -i ./storage/innobase/que/*.h  
clang-format -style=file -i ./storage/innobase/read/*.h  
clang-format -style=file -i ./storage/innobase/rem/*.h  
clang-format -style=file -i ./storage/innobase/row/*.h  
clang-format -style=file -i ./storage/innobase/srv/*.h  
clang-format -style=file -i ./storage/innobase/sync/*.h  
clang-format -style=file -i ./storage/innobase/thr/*.h  
clang-format -style=file -i ./storage/innobase/trx/*.h  
clang-format -style=file -i ./storage/innobase/usr/*.h  
clang-format -style=file -i ./storage/innobase/ut/*.h      


clang-format -style=file -i ./storage/innobase/btr/*.cc                  
clang-format -style=file -i ./storage/innobase/buf/*.cc  
clang-format -style=file -i ./storage/innobase/data/*.cc  
clang-format -style=file -i ./storage/innobase/dict/*.cc  
clang-format -style=file -i ./storage/innobase/dyn/*.cc  
clang-format -style=file -i ./storage/innobase/eval/*.cc  
clang-format -style=file -i ./storage/innobase/fil/*.cc  
clang-format -style=file -i ./storage/innobase/fsp/*.cc  
clang-format -style=file -i ./storage/innobase/fut/*.cc  
clang-format -style=file -i ./storage/innobase/ha/*.cc  
clang-format -style=file -i ./storage/innobase/ibuf/*.cc  
clang-format -style=file -i ./storage/innobase/include/*.cc  
clang-format -style=file -i ./storage/innobase/lock/*.cc  
clang-format -style=file -i ./storage/innobase/log/*.cc  
clang-format -style=file -i ./storage/innobase/mach/*.cc  
clang-format -style=file -i ./storage/innobase/mem/*.cc  
clang-format -style=file -i ./storage/innobase/mtr/*.cc  
clang-format -style=file -i ./storage/innobase/os/*.cc  
clang-format -style=file -i ./storage/innobase/page/*.cc  
clang-format -style=file -i ./storage/innobase/pars/*.cc  
clang-format -style=file -i ./storage/innobase/que/*.cc  
clang-format -style=file -i ./storage/innobase/read/*.cc  
clang-format -style=file -i ./storage/innobase/rem/*.cc  
clang-format -style=file -i ./storage/innobase/row/*.cc  
clang-format -style=file -i ./storage/innobase/srv/*.cc  
clang-format -style=file -i ./storage/innobase/sync/*.cc  
clang-format -style=file -i ./storage/innobase/thr/*.cc  
clang-format -style=file -i ./storage/innobase/trx/*.cc  
clang-format -style=file -i ./storage/innobase/usr/*.cc  
clang-format -style=file -i ./storage/innobase/ut/*.cc


cd -
