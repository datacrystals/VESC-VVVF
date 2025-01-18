#!/usr/bin/python3

import os

# Info Message
os.system("clear")
print("-- Starting, note that you need the arm toolchain for linux: apt install gcc-arm-none-eabi --\n\n")

# Build c into lisp
print(" -- Building C Code --\n")
assert(os.system("cd ../C/VVVF/ && make") == 0)
print(" -- Done Building C Code --\n")

# Now, get the binary and insert it into the lisp code to be used
print(" -- Getting Lisp C Binary --\n")
LispBinary = ""
with open("../C/VVVF/vvvf.lisp", "r") as f:
    LispBinary = f.read()
print(LispBinary)
print(" -- Done Getting Lisp C Binary --\n")


## Next, put it into the main lisp code
print(" -- Inserting Binary Into Lisp Code --\n")
LispCode = ""
with open("../Lisp/Main.lisp", "r") as f:
    LispCode = f.read()

LispCode = LispCode.replace("%COMPILED_C_BINARY", LispBinary)
print(LispCode)

## Finally, put it into the output Lisp Code
with open("../VVVF_COMPILED.lisp", "w") as f:
    f.write(LispCode)
print(" -- Done, Saved To VVVF_Compiled.lisp --\n")
