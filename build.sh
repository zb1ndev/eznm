SRC="
    src/main.c 
    src/scenes/eznm-home.c 
    src/scenes/eznm-saved.c 
    src/scenes/eznm-new-conns.c 
    src/scenes/eznm-cur-conns.c 
    src/scenes/eznm-credentials.c
"

gcc $SRC -o build/eznm $(pkg-config --cflags --libs libnm)
./build/eznm