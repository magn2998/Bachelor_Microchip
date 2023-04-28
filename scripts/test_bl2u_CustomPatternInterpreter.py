import sys,os
import lan966x
import random

import javax.script.ScriptEngineManager
import java.nio.file.Files as Files
import java.nio.file.Paths as Paths

from arm_ds.debugger_v1 import Debugger
from arm_ds.debugger_v1 import DebugException

debugger = Debugger()
ctx = debugger.getCurrentExecutionContext()
breakpointService = ctx.getBreakpointService()
execution_service = ctx.getExecutionService()

# Ensure target is stopped
execution_service.stop()

# Clear old breakpoints
debugger.removeAllBreakpoints()

buildSrc = "build/lan966x_b0/debug"

# Load ELF of BL2U
lan966x.reload_symbols(debugger, buildSrc + "/bl2u/bl2u.elf")

# Setup Values for Operands
BOOTSTRAP_INTERP_MOV = 6
BOOTSTRAP_INTERP_MOVI = 7
BOOTSTRAP_INTERP_MOVETOP = 8
BOOTSTRAP_INTERP_ADD = 9
BOOTSTRAP_INTERP_ADDI = 10
BOOTSTRAP_INTERP_SUB = 11
BOOTSTRAP_INTERP_SUBI = 12
BOOTSTRAP_INTERP_AND = 13
BOOTSTRAP_INTERP_ANDI = 14
BOOTSTRAP_INTERP_OR = 15
BOOTSTRAP_INTERP_ORI = 16
BOOTSTRAP_INTERP_XOR = 17
BOOTSTRAP_INTERP_XORI = 18
BOOTSTRAP_INTERP_LSL = 19
BOOTSTRAP_INTERP_LSLI = 20
BOOTSTRAP_INTERP_LSR = 21
BOOTSTRAP_INTERP_LSRI = 22
BOOTSTRAP_INTERP_MUL = 23
BOOTSTRAP_INTERP_MULI = 24
BOOTSTRAP_INTERP_NEGATE = 25
BOOTSTRAP_INTERP_CMP = 5

# Setup variables used to confirm
operand = (0, '')
rd = (0, '')
r1 = (0, '')
r2 = (0, '')
imm = (0, '')
iType = (0, '')

commandStr = ""

# Setup where to put the values in memory
instrAddr = 0x0010E0A4 
PcAddr =    0x0010E02E

operandAddr = 0x0010E030
rdAddr = 0x0010E031
r1Addr = 0x0010E032
r2Addr = 0x0010E033
immAddr = 0x0010E034


# Where to set breakpoints - before and after reading instruction
BeforeReading = 0x00102102
AfterReading = 0x0010214E

BeforeReadingBreakpoint = debugger.setHardwareAddressBreakpoint(BeforeReading).getId()
AfterReadingBreakpoint  = debugger.setHardwareAddressBreakpoint(AfterReading).getId()

def GetRandomRegister():
    return random.choice([(0,"m"), (1,"p"), (2,"n"), (3,"r1"), (4,"r2"), (5,"r3"), (6,"r4"), (7,"r5")]) 
def GetRandomImm():
    immVal = random.randint(0x0, 0xFFFF)
    return (immVal, "#"+hex(immVal))

def TestMOV():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MOV
    iType = 3
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    commandStr = "MOV "+rd[1]+" "+r1[1]
    print("Testing " + commandStr)
    return commandStr

def TestMOVI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MOVI
    iType = 4
    rd = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "MOVI "+rd[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestMOVETOP():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MOVETOP
    iType = 4
    rd = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "MOVETOP "+rd[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestADD():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_ADD
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "ADD "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return commandStr

def TestADDI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_ADDI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "ADDI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestSUB():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_SUB
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "SUB "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return commandStr

def TestSUBI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_SUBI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "SUBI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestAND():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_AND
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "AND "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return commandStr

def TestANDI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_ANDI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "ANDI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestOR():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_OR
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "OR "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return commandStr

def TestORI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_ORI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "ORI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestXOR():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_XOR
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "XOR "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return commandStr

def TestXORI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_XORI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "XORI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestLSL():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_LSL
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "LSL "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return commandStr

def TestLSLI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_LSLI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "LSLI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestLSR():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_LSR
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "LSR "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return commandStr

def TestLSRI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_LSRI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "LSRI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestMUL():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MUL
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "MUL "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return commandStr

def TestMULI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MULI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "MULI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return commandStr

def TestNEGATE():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_NEGATE
    iType = 3
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    commandStr = "NEGATE "+rd[1]+" "+r1[1]
    print("Testing " + commandStr)
    return commandStr

def TestCompare():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_CMP
    iType = 3
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    commandStr = "CMP "+rd[1]+" "+r1[1]
    print("Testing " + commandStr)
    return commandStr

operations = [TestMOV, TestMOVI, TestMOVETOP, TestADD, TestADDI, TestSUB, TestSUBI, TestAND, TestANDI, TestOR, TestORI, TestXOR, TestXORI, TestLSL, TestLSLI, TestLSR, TestLSRI, TestMUL, TestMULI, TestNEGATE, TestCompare];


def SetUpOperation():
    operation = random.choice(operations)
    instr = AssembleInstruction(operation())
    print("Instruction Value: "+hex(instr))
    # Set Instruction of 0 to command and set PC to 0
    debugger.writeMemoryValue(instrAddr, instr, 32)
    debugger.writeMemoryValue(PcAddr, 0x0, 8)
    

def AfterOperation():
    # Read Different Stuff
    readOperand = int(debugger.readMemoryValue(operandAddr, 8))
    readRd = int(debugger.readMemoryValue(rdAddr, 8))
    readR1 = int(debugger.readMemoryValue(r1Addr, 8))
    readR2 = int(debugger.readMemoryValue(r2Addr, 8))
    readImm = int(debugger.readMemoryValue(immAddr, 32))

    if readOperand != operand:
        print("OPERAND IS WRONG")
        return False

    if iType == 3:
        if(rd[0] == readRd and r1[0] == readR1):
            print("Success! Correctly read out operand="+str(operand)+" and rd="+str(rd[1]))
            return True
        else:
            print("!!!FAILED!!! Incorrectly read out operand="+str(operand)+" and rd="+str(rd[1]))
            return False
    elif iType == 4:
        if(rd[0] == readRd and imm[0] == readImm):
            print("Success! Correctly read out operand="+str(operand)+", rd="+str(rd[1])+" and imm="+str(readImm))
            return True
        else:
            print("!!!FAILED!!! Incorrectly read out operand="+str(operand)+", rd="+str(rd[1])+" and imm="+str(readImm))
            return False
    elif iType == 5:
        if(rd[0] == readRd and r1[0] == readR1 and r2[0] == readR2):
            print("Success! Correctly read out operand="+str(operand)+", rd="+str(rd[1])+", r1="+str(r1[1])+" and r2="+str(r2[1]))
            return True
        else:
            print("!!!FAILED!!! Incorrectly read out operand="+str(operand)+", rd="+str(rd[1])+", r1="+str(r1[1])+" and r2="+str(r2[1]))
            return False
    elif iType == 6:
        if(rd[0] == readRd and r1[0] == readR1 and imm[0] == readImm):
            print("Success! Correctly read out operand="+str(operand)+", rd="+str(rd[1])+", r1="+str(r1[1])+" and imm="+str(readImm))
            return True
        else:
            print("!!!FAILED!!! Incorrectly read out operand="+str(operand)+", rd="+str(rd[1])+", r1="+str(r1[1])+" and imm="+str(readImm))
            return False


script_file = "//wsl.localhost/Ubuntu-18.04/home/magn2998/arm-trusted-firmware/scripts/fwu/ddr-test.js"
manager = javax.script.ScriptEngineManager()
engine = manager.getEngineByName("nashorn")

with open(script_file, "r") as file:
    # Read the JavaScript file
    script = file.read()
    # Prepare the engine with the Javascript Functions
    engine.eval(script)

def AssembleInstruction(instrTxt):
    result = engine.eval("assemble_program('"+instrTxt+"')")
    return int(result["0"]) # Get Assembled Instruction as an integer

firstExecution = True
while(True):
    
    execution_service.waitForStop(0)
    hitBreakPoint = breakpointService.getHitBreakpoint()

    if hitBreakPoint == None:
        execution_service.resume()
        continue

    hitBreakId = hitBreakPoint.getId()
    if hitBreakId == BeforeReadingBreakpoint:
        SetUpOperation()
    if hitBreakId == AfterReadingBreakpoint:
        if not AfterOperation():
            break
        execution_service.setExecutionAddress(BeforeReading-4) # Return to first address
   

debugger.removeAllBreakpoints()
print("Done running Interpreter Testing Script")