import os
import shlex
import subprocess
import sys

def isInt(var):
    try:
        int(var)
        return True
    except:
        return False

def isFloat(var):
    try:
        float(var)
        return True
    except:
        return False

def hasPredecessor(reg, insns):
    for opcat in insns:
        for insn in insns[opcat]:
            if insn["dst"]==reg:
                return True
    return False

def getSuccessors(i, insns):
    successors = []
    for opcat in insns:
        for insn in insns[opcat]:
            for src in insn["src"]:
                if src==i["dst"]:
                    successors.append(insn["id"])
    return successors

def insertDotEdge(fp, src, dst, label=None):
    fp.write("\t\"{}\"->\"{}\"".format(src, dst))
    if label:
        fp.write("\t[ label=\"{}\"]\n".format(label))

def labelDotNode(fp, insn):
    # Derive the node label
    label = ""
    if insn["dst"]:
        label += insn["dst"] + " = "
    label += insn["op"] + " "
    if len(insn["src"]):
        label += ", ".join(insn["src"])
    if "addr" in insn:
        label += insn["addr"]
    fp.write("\t{} [ label=\"{}\"]\n".format(insn["id"], label))

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
                params["op"] = opcode
                if opcode == "br":
                    params["id"] = tokens[1]
                    params["src"] = [tokens[2].split("->")[0].split("i")[0]]
                    params["dst"] = tokens[2].split("->")[1].split("i")[0]
                elif opcode == "phi":
                    params["id"] = tokens[1]
                    params["ty"] = tokens[2]
                    params["dst"] = tokens[3]
                    params["src"] = []
                    for i in range(int(tokens[4])):
                        # Check if src is a constant
                        src = tokens[5+i]
                        # if isInt(src) or isFloat(src):
                        #     src = "const("+src+")"
                        params["src"].append(src)
                elif opcode == "load":
                    params["id"] = tokens[1]
                    params["ty"] = tokens[2]
                    params["dst"] = tokens[3]
                    params["src"] = []
                    params["addr"] = tokens[4]
                elif opcode == "store":
                    params["id"] = tokens[1]
                    params["ty"] = tokens[2]
                    params["src"] = [tokens[3]]
                    params["dst"] = None
                    params["addr"] = tokens[4]
                elif opcode == "sitofp" or opcode == "fptosi":
                    params["id"] = tokens[1]
                    params["dstty"] = tokens[2]
                    params["srcty"] = tokens[3]
                    params["dst"] = tokens[4]
                    params["src"] = [tokens[5]]
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
    # Dumpt DOT description of CFG
    with open(fn, "w") as fp:
        fp.write("digraph graphname {")
        for br in insns["br"]:
            insertDotEdge(fp, br["src"][0], br["dst"])
        fp.write("}")

def DFG(insns, fn, nodeMode="reg"):
    # Dump the DOT description of DFG
    with open(fn, "w") as fp:
        fp.write("digraph graphname {")
        # Print all instructions
        for cat in insns:
            if cat!="br":
                for insn in insns[cat]:
                    if nodeMode=="reg":
                        # reg mode - DFG nodes are registers
                        for src in insn["src"]:
                            # Check if is is a phi node - if it is, make sure it
                            # has no successors (because all phi nodes are dumped)
                            if cat!="phi" or cat=="phi" and hasPredecessor(src, insns):
                                insertDotEdge(fp, src, insn["dst"], cat)
                    elif nodeMode=="insn":
                        # instruction mode - DFG nodes are instructions
                        for succ in getSuccessors(insn, insns):
                            # Insert edge
                            insertDotEdge(fp, insn["id"], succ, insn["dst"])
                            labelDotNode(fp, insn)
                        # Corner case: store instructions don't have successors
                        if cat=="store":
                            labelDotNode(fp, insn)


        fp.write("}")


if __name__ == '__main__':
    insns = process('static.txt')
    CFG(insns, "cfg.dot")
    DFG(insns, "dfg.dot", "reg")
    # Convert to png with graphviz
    subprocess.call(shlex.split('dot cfg.dot -Tpng -ocfg.png'))
    subprocess.call(shlex.split('dot dfg.dot -Tpng -odfg.png'))


