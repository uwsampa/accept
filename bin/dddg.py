import copy
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

def isInt(reg):
    try:
        int(reg)
        return True
    except:
        return False

def isFloat(reg):
    try:
        float(reg)
        return True
    except:
        return False

def isConstant(reg):
    if reg=="null":
        return True
    elif isInt(reg):
        return True
    elif isFloat(reg):
        return True
    else:
        return False

def isApprox(i):
    if i["qual"]=="approx":
        return True
    else:
        return False

def getBBPredecessors(node, branches):
    predecessors = []
    for bb in branches:
        for succ in branches[bb]["tgt"]:
            if succ==branches[node]["src"]:
                predecessors.append(branches[bb]["id"])
    return predecessors

def hasPredecessor(i_key, insns):
    if len(getPredecessors(i_key, insns)):
        return True
    else:
        return False

def hasSuccessor(i_key, insns):
    if len(getSuccessors(i_key, insns)):
        return True
    else:
        return False

def replaceAllUsesOfWith(before, after, insns):
    for insn in insns.itervalues():
        for idx, src in enumerate(insn["src"]):
            if src==before:
                insn["src"][idx] = after

def getPredecessors(i_key, insns):
    predecessors = []
    for src in insns[i_key]["src"]:
        if src in insns:
            predecessors.append(src)
    return predecessors

def getSuccessors(i_key, insns):
    successors = []
    for insn in insns.itervalues():
        for src in insn["src"]:
            if src==i_key:
                successors.append(insn["dst"])
    return successors

def getEntryInstructions(insns):
    entryInsns = []
    for i_key in insns:
        if not hasPredecessor(i_key, insns):
            entryInsns.append(i_key)
    return entryInsns

def getExitInstructions(insns):
    exitInsns = []
    for i_key in insns:
        if not hasSuccessor(i_key, insns):
            exitInsns.append(i_key)
    return exitInsns

def getInsnFromBB(bbid, insns):
    # Excludes phi nodes!
    insnList = []
    for i_key in insns:
        if insns[i_key]["bbid"]==bbid:
            insnList.append(i_key)
    return insnList

def getInsnfromReg(reg, insns):
    for i_key in insns:
        if insns[i_key]["dst"]==reg:
            return i_key
    return ""

def getSrcFromPhi(i_key, insns):
    assert len(insns[i_key]["src"])==1
    return insns[i_key]["src"][0]

def cleanupPhiNodes(dddg):
    # Delete list
    delList = []
    # Look for phi nodes
    for dst in dddg:
        insn = dddg[dst]
        if insn["op"]=="phi":
            src = getSrcFromPhi(dst, dddg)
            replaceAllUsesOfWith(dst, src, dddg)
            delList.append(dst)
    # Remove all phi nodes
    for d in delList:
        del dddg[d]

def insertDotEdge(fp, src, dst, label=""):
    fp.write("\t\"{}\"->\"{}\"\n".format(src, dst))
    if label!="":
        fp.write("\t[ label=\"{}\" ]\n".format(label))

def getLabel(insn):
    label="("+insn["bbid"]+") "
    if insn["dst"]:
        label += insn["dst"] + " = "
    label += insn["op"] + " "
    if insn["op"] == "call":
        label += insn["fn"] + "("
    if insn["op"] == "phi":
        src = ["[" + x[0] + ", " + x[1] + "]" for x in zip(insn["src"], insn["srcBB"])]
        label += ", ".join(src)
    else:
        label += ", ".join(insn["src"])
    if insn["op"] == "call":
        label += ")"
    if "addr" in insn:
        label += insn["addr"]
    return label

def labelDotNode(fp, i_key, insns):
    insn = insns[i_key]
    print "{}, {}".format(i_key, insn)
    # Derive the node label
    label = getLabel(insn)
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
    fp.write("\t\"{}\" [ ".format(i_key))
    fp.write("label=\"{}\" ".format(label))
    fp.write("color=\"{}\" ".format(color))
    fp.write("shape=\"{}\" ".format(shape))
    fp.write("style=\"rounded\"]\n")

def trimStep(insns):
    trimList = []
    for i_key, insn in insns.iteritems():
        if not hasApproxPredecessor(i_key, insns) and not hasApproxSuccessor(i_key, insns):
            trimList.append(i_key)
            trimmed = True
        elif insn["op"] != "store" and insn["op"] != "ret":
            if len(getSuccessors(i_key, insns))==0:
                trimList.append(i_key)
                trimmed = True
    if len(trimList):
        for i_key in trimList:
            del insns[i_key]
        return True
    else:
        return False

def trim(insns):
    # Trim the graph
    epoch = 0
    while(trimStep(insns)):
        epoch+=1
    print "Trimmed the graph in {} steps".format(epoch)

def hasApproxPredecessor(i_key, insns):
    visited = []
    predecessors = [i_key]

    while len(predecessors):
        new_predecessors = []
        for p_key in predecessors:
            # Visit the node
            visited.append(p_key)
            # Check if it is approx
            if isApprox(insns[p_key]):
                return True
            # Prepare the new predecessors list
            for pp_key in getPredecessors(p_key, insns):
                if pp_key not in visited and pp_key not in new_predecessors:
                    new_predecessors.append(pp_key)
        predecessors = new_predecessors
    return False

def hasApproxSuccessor(i_key, insns):
    visited = []
    successors = [i_key]

    while len(successors):
        new_successors = []
        for s_key in successors:
            # Visit the node
            visited.append(s_key)
            # Check if it is approx
            if isApprox(insns[s_key]):
                return True
            # Prepare the new successors list
            for ss_key in getSuccessors(s_key, insns):
                if ss_key not in visited and ss_key not in new_successors:
                    new_successors.append(ss_key)
        successors = new_successors
    return False

def processBBExecs(fn):
    # Process bbstats file line by line
    bbExecs = {}
    if (os.path.isfile(fn)):
        with open(fn) as fp:
            for line in fp:
                # Tokenize
                tokens = line.strip().split("\t")
                if tokens[0]!="BB":
                    bbExecs["bb"+tokens[0]] = int(tokens[1])
    return bbExecs

def processBranchOutcomes(fn, insn, branches):

    phiPrevDict = {}
    branchOutcomes = {}
    # outgoingStat = {}
    # incomingStat = {}

    print "Processing branch outcomes"
    if (os.path.isfile(fn)):
        with open(fn) as fp:
            for line in fp:
                # Tokenize
                tokens = line.strip().split(",")
                srcBB = tokens[0]
                outcome = int(tokens[1])
                numOutcomes = int(tokens[2])
                dstBB =  branches[srcBB]["tgt"][outcome]
                # if not srcBB in outgoingStat:
                #     outgoingStat[srcBB] = {}
                # if not dstBB in outgoingStat[srcBB]:
                #     outgoingStat[srcBB][dstBB] = 0
                # outgoingStat[srcBB][dstBB] += numOutcomes
                # if not dstBB in incomingStat:
                #     incomingStat[dstBB] = {}
                # if not srcBB in incomingStat[dstBB]:
                #     incomingStat[dstBB][srcBB] = 0
                # incomingStat[dstBB][srcBB] += numOutcomes

                # Log branch outcomes
                if not srcBB in branchOutcomes:
                    branchOutcomes[srcBB] = []
                branchOutcomes[srcBB].append([dstBB, numOutcomes])

    # print "Incoming Branch Statistics"
    # print incomingStat
    # print "Outgoing Branch Statistics"
    # print outgoingStat

    return branchOutcomes


def processBranches(fn):
    branches = {}
    # Process static dump line by line
    if (os.path.isfile(fn)):
        with open(fn) as fp:
            for line in fp:
                # Tokenize
                tokens = line.strip().split(", ")
                op = tokens[0]
                # Check for op type
                if op == "br":
                    iid = tokens[1]
                    src = tokens[2].split("->")[0]
                    dst = tokens[2].split("->")[1]
                    if src in branches:
                        branches[src]["tgt"].append(dst)
                    else:
                        params = {}
                        params["op"] = op
                        params["id"] = iid
                        params["src"] = src
                        params["tgt"] = [dst]
                        branches[src] = params

    return branches

def processInsn(fn):
    # List of unknown opcodes
    unknown = []
    # List of instructions
    insns = {}
    # Process static dump line by line
    if (os.path.isfile(fn)):
        with open(fn) as fp:
            for line in fp:
                # Skip instruction
                skip = False
                # Tokenize
                tokens = line.strip().split(", ")
                # Check for op type
                opcode = tokens[0]
                # Process token
                params = {}
                params["op"] = opcode
                params["id"] = tokens[1]
                params["bbid"] = params["id"].split("i")[0]
                params["qual"] = tokens[2]
                # If return instruction
                if opcode == "ret":
                    params["ty"] = tokens[3]
                    params["dst"] = None
                    params["src"] = [tokens[4]]
                # If call instruction
                elif opcode in callInsn:
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
                # If phi node
                elif opcode in phiInsn:
                    params["ty"] = tokens[3]
                    params["dst"] = tokens[4]
                    params["src"] = []
                    params["srcBB"] = []
                    for i in range(int(tokens[5])):
                        params["src"].append(tokens[6+2*i+0])
                        params["srcBB"].append(tokens[6+2*i+1])
                # If load instruction
                elif opcode == "load":
                    params["ty"] = tokens[3]
                    params["dst"] = tokens[4]
                    params["src"] = []
                    params["addr"] = tokens[5]
                # If store instruction
                elif opcode == "store":
                    params["ty"] = tokens[3]
                    params["src"] = [tokens[4]]
                    params["dst"] = tokens[5] # HACK
                    params["addr"] = tokens[5]
                # If cast instruction
                elif opcode in conversionInsn:
                    params["dstty"] = tokens[3]
                    params["srcty"] = tokens[4]
                    params["dst"] = tokens[5]
                    params["src"] = [tokens[6]]
                # If binary or comparison instruction
                elif opcode in binaryInsn or opcode in cmpInsn:
                    params["ty"] = tokens[3]
                    params["dst"] = tokens[4]
                    params["src"] = [tokens[5], tokens[6]]
                else:
                    if opcode not in unknown:
                        unknown.append(opcode)
                        print "Don't know how to handle {} opcode!".format(opcode)
                    skip = True


                if not skip and params["dst"]:
                    insns[params["dst"]] = params
    else:
        print "file not found!"

    # Cleanup instructions
    trim(insns)

    return insns

def CFG(branches, fn):
    # Dumpt DOT description of CFG
    with open(fn, "w") as fp:
        fp.write("digraph graphname {\n")
        for iid in branches:
            br = branches[iid]
            for idx, tgt in enumerate(br["tgt"]):
                if "outcome" in br:
                    if br["outcome"][idx]>0:
                        label = ""
                        if br["outcome"][idx]!=br["total"]:
                            label = str(br["outcome"][idx])+"/"+str(br["total"])
                        insertDotEdge(fp, br["src"], tgt, label)
                else:
                    insertDotEdge(fp, br["src"], tgt)
        fp.write("}")

def DFG(insns, fn):
    # Dump the DOT description of DFG
    with open(fn, "w") as fp:
        fp.write("digraph graphname {\n")
        # Print all instructions
        for i_key, insn in insns.iteritems():
            for s_key in getSuccessors(i_key, insns):
                # Insert edge
                insertDotEdge(fp, i_key, s_key)
                labelDotNode(fp, i_key, insns)
            # Corner case: store instructions don't have successors
            if insn["op"]=="store":
                labelDotNode(fp, i_key, insns)
            # Other corner case: terminators
            elif insn["op"]=="ret":
                labelDotNode(fp, i_key, insns)

        fp.write("}")

def DDDG(insns, branchOutcomes, limit=10000):

    # Get exit instructions
    exitInsns = getExitInstructions(insns)

    # Replay the program based on CFG outcomes
    DDDG = {}       # DDDG
    currBB = "bb0"  # Programs always start at bb0
    nextBB = ""     # Next basic block to visit
    prevBB = ""     # Previous basic block visited
    cntr = 0        # Counter to limit the size of the DDDG
    done = False    # Indicates that we have reached an exit
    bbIdx = 0       # Basic block index used in the traversal
    bbIdxMap = {}   # Map of the last index for a given BB

    # Replay the program based on control flow trace
    while currBB!="" and cntr<=limit:

        # Multiple possible branch outcomes
        if len(branches[currBB]["tgt"])>1:
            if len(branchOutcomes[currBB])>0:
                nextBB = branchOutcomes[currBB][0][0]
                branchOutcomes[currBB][0][1]-=1
                if branchOutcomes[currBB][0][1]==0:
                    branchOutcomes[currBB].pop(0)
            else:
                nextBB = ""
        # Single branch outcomes
        elif len(branches[currBB]["tgt"])==1:
            nextBB = branches[currBB]["tgt"][0]
        # No branch outcome (reached the exit)
        else:
            nextBB = ""

        print "{}->{}, prev={}".format(currBB, nextBB, prevBB)

        # Get all instructions from the current basic block
        for i_key in getInsnFromBB(currBB, insns):
            insn = copy.deepcopy(insns[i_key])
            # Modify the id and registers
            insn["id"]+="_"+str(bbIdx)
            if insn["dst"]:
                insn["dst"]+="_"+str(bbIdx)
            # If the instruction is a phi node, discard non-valid sources
            if insn["op"]=="phi":
                # Set the srcBB and src register to match dynamic execution
                for srcReg, srcBB in zip(insns[i_key]["src"], insns[i_key]["srcBB"]):
                    if srcBB==prevBB:
                        insn["srcBB"]=[prevBB]
                        # Check if we are dealing with a constant
                        if isConstant(srcReg):
                            # No need to derive producer instruction
                            insn["src"]=[srcReg]
                        else:
                            # Determine bbid of producer instruction
                            srcRegBB = insns[getInsnfromReg(srcReg, insns)]["bbid"]
                            insn["src"]=[srcReg+"_"+str(bbIdxMap[srcRegBB])]
            else:
                for idx, srcReg in enumerate(insn["src"]):
                    insn["src"][idx]+="_"+str(bbIdx)
            print "\t {}".format(getLabel(insn))
            DDDG[insn["dst"]] = insn
            cntr += 1

        # Check that we have reached a exit instruction
        for i_key in exitInsns:
            if insns[i_key]["bbid"] == currBB:
                done = True

        bbIdxMap[currBB] = bbIdx
        bbIdx += 1
        prevBB = currBB
        currBB = nextBB

        if done:
            break

    # Eliminate phi nodes
    cleanupPhiNodes(dddg)

    return DDDG

if __name__ == '__main__':
    # Extract instructions
    insns = processInsn('accept_static.txt')
    # Extract branches
    branches = processBranches('accept_static.txt')
    # Produce DFG and CFG graph based on static information
    DFG(insns, "dfg.dot")
    subprocess.call(shlex.split('dot dfg.dot -Tpng -odfg.png'))
    CFG(branches, "cfg.dot")
    subprocess.call(shlex.split('dot cfg.dot -Tpng -ocfg.png'))

    # Produce DDDG
    branchOutcomes = processBranchOutcomes('accept_dyntrace.txt', insns, branches)
    dddg = DDDG(insns, branchOutcomes)
    DFG(dddg, "dddg.dot")
    subprocess.call(shlex.split('dot dddg.dot -Tpng -odddg.png'))



