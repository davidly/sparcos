ARCH=$(uname -m)
LDFLAGS="-lm"

if [ "$ARCH" = "x86_64" ]; then
            LDFLAGS="$LDFLAGS -lquadmath"
fi

g++ -DSPARCOS -DNDEBUG -O3 -fno-builtin -I . sparcos.cxx sparc.cxx -o sparcos -static $LDFLAGS
