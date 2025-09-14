import sys

with open(sys.argv[1],'rb') as fin:
    with open(sys.argv[2], 'w') as fout:
        buff = fin.read()
        fout.write("const unsigned char bootstrap_dll[" + str(len(buff)) + "] = {\n")
        for b in buff:
            fout.write(str(b) + ",")
        fout.write("\n};\n")
