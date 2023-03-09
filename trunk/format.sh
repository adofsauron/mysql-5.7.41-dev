
#!/bin/bash

cd mysql-5.7.41

# clang-format -style=file -i   ./include/*.h
clang-format -style=file -i   ./dbug/*.c

# clang-format -style=file -i   ./sql/*.h
clang-format -style=file -i   ./sql/*.cc


# clang-format -style=file -i ./storage/innobase/api/*
# clang-format -style=file -i ./storage/innobase/btr/*
# clang-format -style=file -i ./storage/innobase/buf/*
# clang-format -style=file -i ./storage/innobase/data/*
# clang-format -style=file -i ./storage/innobase/dict/*
# clang-format -style=file -i ./storage/innobase/eval/*
# clang-format -style=file -i ./storage/innobase/fil/*
# clang-format -style=file -i ./storage/innobase/fsp/*
# clang-format -style=file -i ./storage/innobase/fts/*
# clang-format -style=file -i ./storage/innobase/fut/*
# clang-format -style=file -i ./storage/innobase/gis/*
# clang-format -style=file -i ./storage/innobase/ha/*
# clang-format -style=file -i ./storage/innobase/handler/*
# clang-format -style=file -i ./storage/innobase/ibuf/*
# clang-format -style=file -i ./storage/innobase/include/*
# clang-format -style=file -i ./storage/innobase/lock/*
# clang-format -style=file -i ./storage/innobase/log/*
# clang-format -style=file -i ./storage/innobase/mach/*
# clang-format -style=file -i ./storage/innobase/mem/*
# clang-format -style=file -i ./storage/innobase/mtr/*
# clang-format -style=file -i ./storage/innobase/os/*
# clang-format -style=file -i ./storage/innobase/page/*
# clang-format -style=file -i ./storage/innobase/pars/*
# clang-format -style=file -i ./storage/innobase/que/*
# clang-format -style=file -i ./storage/innobase/read/*
# clang-format -style=file -i ./storage/innobase/rem/*
# clang-format -style=file -i ./storage/innobase/row/*
# clang-format -style=file -i ./storage/innobase/srv/*
# clang-format -style=file -i ./storage/innobase/sync/*
# clang-format -style=file -i ./storage/innobase/trx/*
# clang-format -style=file -i ./storage/innobase/usr/*
# clang-format -style=file -i ./storage/innobase/ut/*


cd -
