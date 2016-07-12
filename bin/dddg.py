import os
import shlex
import subprocess
import sys

# LLVM instruction categories
# We ignore phi nodes since those are required
# because of the SSA style of the LLVM IR
controlInsn = ['br','switch','indirectbr','ret']
terminatorInsn = ['invoke','resume','catchswitch','catchret','cleanupret','unreachable']
binaryInsn = ['fadd','fsub','fmul','fdiv','frem','add','sub','mul','udiv','sdiv','urem','srem','shl','lshr','ashr','and','or','xor']
vectorInsn = ['extractelement','insertelement','shufflevector']
aggregateInsn = ['extractvalue','insertvalue']
loadstoreInsn = ['load','store']
getelementptrInsn = ['getelementptr']
memoryInsn = ['alloca','fence','cmpxchg','atomicrmw']
conversionInsn = ['trunc','zext','sext','fptrunc','fpext','fptoui','fptosi','uitofp','sitofp','ptrtoint','inttoptr','bitcast','addrspacecast']
cmpInsn = ['icmp','fcmp']
callInsn = ['call']
phiInsn = ['phi']
otherInsn = ['select','va_arg','landingpad','catchpad','cleanuppad']

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

def isApprox(i):
    if i["qual"]=="approx":
        return True
    else:
        return False

def hasPredecessor(i, insns):
    if len(getPredecessors(reg, insn)):
        return True
    else:
        return False

def getPredecessors(i, insns):
    predecessors = []
    for iid in insns:
        insn = insns[iid]
        for src in i["src"]:
            if src==insn["dst"]:
                predecessors.append(insn["id"])
    return predecessors

def getSuccessors(i, insns):
    successors = []
    for iid in insns:
        insn = insns[iid]
        for src in insn["src"]:
            if src==i["dst"]:
                successors.append(insn["id"])
    return successors

def insertDotEdge(fp, src, dst, label=None):
    fp.write("\t\"{}\"->\"{}\"".format(src, dst))
    if label:
        fp.write("\t[ label=\"{}\" ]\n".format(label))

def labelDotNode(fp, insn):
    # Derive the node label
    label = ""
    if insn["dst"]:
        label += insn["dst"] + " = "
    label += insn["op"] + " "
    if insn["op"] == "call":
        label += insn["fn"] + "("
    if len(insn["src"]):
        label += ", ".join(insn["src"])
    if insn["op"] == "call":
        label += ")"
    if "addr" in insn:
        label += insn["addr"]
    # Derive the node color
    color = "forestgreen" if isApprox(insn) else "black"
    # Derive the node shape
    shape = "box"
    if insn["op"]=="phi":
        shape = "diamond"
    elif insn["op"]=="store":
        shape = "triangle"
    elif insn["op"]=="load":
        shape = "invtriangle"
    elif insn["op"]=="call":
        shape = "doubleoctagon"
    # Add the dot edge
    fp.write("\t{} [ ".format(insn["id"]))
    fp.write("label=\"{}\" ".format(label))
    fp.write("color=\"{}\" ".format(color))
    fp.write("shape=\"{}\" ".format(shape))
    fp.write("style=\"rounded\"]\n")

def trim(insns):
    # Trim the graph
    epoch = 0
    while(trimStep(insns)):
        epoch+=1
    print "Trimmed the graph in {} steps".format(epoch)

def trimStep(insns):
    trimList = []
    for iid in insns:
        insn = insns[iid]
        if not hasApproxPredecessor(insn, insns) and not hasApproxSuccessor(insn, insns):
            trimList.append(iid)
            trimmed = True
        elif insn["op"] != "store" and insn["op"] != "ret":
            if len(getSuccessors(insn, insns))==0:
                trimList.append(iid)
                trimmed = True
    if len(trimList):
        for iid in trimList:
            del insns[iid]
        return True
    else:
        return False

def hasApproxPredecessor(i, insns):
    visited = []
    predecessors = [i["id"]]

    while len(predecessors):
        new_predecessors = []
        for p_id in predecessors:
            # Visit the node
            visited.append(p_id)
            # Check if it is approx
            if isApprox(insns[p_id]):
                return True
            # Prepare the new predecessors list
            for pp_id in getPredecessors(insns[p_id], insns):
                if pp_id not in visited and pp_id not in new_predecessors:
                    new_predecessors.append(pp_id)
        predecessors = new_predecessors
    return False

def hasApproxSuccessor(i, insns):
    visited = []
    successors = [i["id"]]

    while len(successors):
        new_successors = []
        for s_id in successors:
            # Visit the node
            visited.append(s_id)
            # Check if it is approx
            if isApprox(insns[s_id]):
                return True
            # Prepare the new successors list
            for ss_id in getSuccessors(insns[s_id], insns):
                if ss_id not in visited and ss_id not in new_successors:
                    new_successors.append(ss_id)
        successors = new_successors
    return False

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
                params["id"] = tokens[1]
                params["qual"] = tokens[2]
                # if opcode == "br":
                #     params["id"] = tokens[1]
                #     params["src"] = [tokens[2].split("->")[0].split("i")[0]]
                #     params["dst"] = tokens[2].split("->")[1].split("i")[0]
                if opcode == "ret":
                    params["ty"] = tokens[3]
                    params["dst"] = None
                    params["src"] = [tokens[4]]
                elif opcode in callInsn:
                    print tokens
                    params["fn"] = tokens[3]
                    params["dstty"] = tokens[4]
                    params["dst"] = None
                    params["srcty"] = []
                    params["src"] = []
                    idx = 5
                    if params["dstty"]!="void":
                        params["dst"] = tokens[idx]
                        idx+=1
                    for i in range(int(tokens[idx])):
                        token_idx = idx+1+i*2
                        params["srcty"].append(tokens[token_idx+0])
                        params["src"].append(tokens[token_idx+1])
                elif opcode in phiInsn:
                    params["ty"] = tokens[3]
                    params["dst"] = tokens[4]
                    params["src"] = []
                    for i in range(int(tokens[5])):
                        params["src"].append(tokens[6+i])
                elif opcode == "load":
                    params["ty"] = tokens[3]
                    params["dst"] = tokens[4]
                    params["src"] = []
                    params["addr"] = tokens[5]
                elif opcode == "store":
                    params["ty"] = tokens[3]
                    params["src"] = [tokens[4]]
                    params["dst"] = None
                    params["addr"] = tokens[5]
                elif opcode in conversionInsn:
                    params["dstty"] = tokens[3]
                    params["srcty"] = tokens[4]
                    params["dst"] = tokens[5]
                    params["src"] = [tokens[6]]
                elif opcode in binaryInsn or opcode in cmpInsn:
                    params["ty"] = tokens[3]
                    params["dst"] = tokens[4]
                    params["src"] = [tokens[5], tokens[6]]
                else:
                    print opcode

                if params["op"]!="br":
                    insns[params["id"]] = params
    else:
        print "file not found!"

    return insns

def CFG(insns, fn):
    # Dumpt DOT description of CFG
    with open(fn, "w") as fp:
        fp.write("digraph graphname {")
        for iid in insns:
            insn = insns[iid]
            if insn["op"]=="br":
                insertDotEdge(fp, insn["src"][0], insn["dst"])
        fp.write("}")

def DFG(insns, fn, nodeMode="reg"):
    # Dump the DOT description of DFG
    with open(fn, "w") as fp:
        fp.write("digraph graphname {")
        # Print all instructions
        for iid in insns:
            insn = insns[iid]
            if insn["op"]!="br":
                if nodeMode=="reg":
                    # reg mode - DFG nodes are registers
                    for src in insn["src"]:
                        # Check if is is a phi node - if it is, make sure it
                        # has no successors (because all phi nodes are dumped)
                        if insn["op"]!="phi" or insn["op"]=="phi" and hasPredecessor(insn, insns):
                            insertDotEdge(fp, src, insn["dst"], insn["op"])
                elif nodeMode=="insn":
                    # instruction mode - DFG nodes are instructions
                    for succ in getSuccessors(insn, insns):
                        # Insert edge
                        label = insn["dst"]
                        color = "forestgreen" if insn["qual"]=="approx" else None
                        insertDotEdge(fp, insn["id"], succ)
                        labelDotNode(fp, insn)
                    # Corner case: store instructions don't have successors
                    if insn["op"]=="store":
                        labelDotNode(fp, insn)
                    # Other corner case: terminators
                    elif insn["op"]=="ret":
                        labelDotNode(fp, insn)


        fp.write("}")


if __name__ == '__main__':
    insns = process('accept_static.txt')

    trim(insns)

    # CFG(insns, "cfg.dot")
    DFG(insns, "dfg.dot", "insn")
    # Convert to png with graphviz
    # subprocess.call(shlex.split('dot cfg.dot -Tpng -ocfg.png'))
    subprocess.call(shlex.split('dot dfg.dot -Tpng -odfg.png'))


