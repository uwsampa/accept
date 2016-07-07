import os
import sys

def process(fn):
    insns = {}
    # Process static dump line by line
    if (os.path.isfile(fn)):
        with open(fn) as fp:
            for line in fp:
                # Tokenize
                tokens = line.strip().split(", ")
                # Check for op type
                opcode = tokens[0]
                # Process token
                params = {}
                if opcode == "br":
                    params["id"] = tokens[1]
                    params["src"] = tokens[2].split("->")[0].split("i")[0]
                    params["dst"] = tokens[2].split("->")[1].split("i")[0]
                elif opcode == "phi":
                    params["id"] = tokens[1]
                    params["ty"] = tokens[2]
                    params["dst"] = tokens[3]
                    params["numsrc"] = int(tokens[4])
                    params["src"] = []
                    for i in range(params["numsrc"]):
                        params["src"].append(tokens[5+i])
                elif opcode == "load":
                    params["id"] = tokens[1]
                    params["ty"] = tokens[2]
                    params["dst"] = tokens[3]
                    params["src"] = tokens[4]
                elif opcode == "store":
                    params["id"] = tokens[1]
                    params["ty"] = tokens[2]
                    params["src"] = tokens[3]
                    params["dst"] = tokens[4]
                elif opcode == "sitofp" or opcode == "fptosi":
                    params["id"] = tokens[1]
                    params["dstty"] = tokens[2]
                    params["srcty"] = tokens[3]
                    params["dst"] = tokens[4]
                    params["src"] = tokens[5]
                elif opcode == "fmul" or opcode == "fadd" or opcode == "fdiv":
                    params["id"] = tokens[1]
                    params["ty"] = tokens[2]
                    params["dst"] = tokens[3]
                    params["src"] = [tokens[4], tokens[5]]
                else:
                    print opcode
                if opcode not in insns:
                    insns[opcode] = [params]
                else:
                    insns[opcode].append(params)
    else:
        print "file not found!"

    return insns

def CFG(insns, fn):
    with open(fn, "w") as fp:
        fp.write("digraph graphname {")
        for br in insns["br"]:
            fp.write("\t\"{}\"->\"{}\"\n".format(br["src"], br["dst"]))
        fp.write("}")

def DFG(insns, fn):
    with open(fn, "w") as fp:
        fp.write("digraph graphname {")
        # phi instructions
        for br in insns["phi"]:
            print br
            for i in range(br["numsrc"]):
                fp.write("\t\"{}\"->\"{}\"\n".format(br["src"][i], br["dst"]))
        # unary instructions
        for opcat in ["load", "store", "sitofp", "fptosi"]:
            for insn in insns[opcat]:
                fp.write("\t\"{}\"->\"{}\"\n".format(insn["src"], insn["dst"]))
        # binary instructions
        for opcat in ["fmul", "fadd", "fdiv"]:
            for insn in insns[opcat]:
                for i in range(2):
                    fp.write("\t\"{}\"->\"{}\"\n".format(insn["src"][i], insn["dst"]))
        fp.write("}")


if __name__ == '__main__':
    insns = process('static.txt')
    print insns
    CFG(insns, "cfg.dot")
    DFG(insns, "dfg.dot")

