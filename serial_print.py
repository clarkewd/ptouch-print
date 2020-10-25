from __future__ import print_function
import subprocess

printed = 1

fz = input("fontsize: ")
if type(fz) is not int:
    print("input fontsize has to be integer")
    exit()

fn = input("First Number: ")
if type(fn) is not int:
    print("First Number has to be integer")
    exit()

ln = input("Last Number: ")
if type(ln) is not int:
    print("Last Number has to be integer")
    exit()

cp = input("Copies: ")
if type(cp) is not int:
    print("Copies has to be integer")
    exit()

printsum = (ln - fn + 1) * cp
print("printing...")

while fn <= ln:
    out = str(fn).zfill(6)
    for x in range(cp):
        print(str(printed)+"/"+str(printsum))
        printed = printed + 1
        subprocess.call(["./ptouch-print", "--halfcut", "--fontsize", str(fz), "--blocking", "--text", "B"+out], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        #print("B"+out, end='\r')
    
    fn = fn + 1
