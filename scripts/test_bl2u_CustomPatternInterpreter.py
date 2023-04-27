import sys,os
import lan966x
import random
import js2py

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
operand = 0
rd = 0
r1 = 0
r2 = 0
imm = 0
iType = 0

commandStr = ""

# Setup where to put the values in memory
operandAddr = 0x0010E02C
r1Addr = 0x0010E02E
r2Addr = 0x0010E02F
immAddr = 0x0010E034
rdAddr = 0x0010E02D #rd is actually r5
# Where their values are stored in memory
r1ValAddr = 0x0010E04C
r2ValAddr = 0x0010E050
rdValAddr = 0x0010E05C


def GetRandomRegister():
    return random.choice([(0,"m"), (1,"p"), (2,"n"), (3,"r1"), (4,"r2"), (5,"r3"), (6,"r4"), (7,"r5")]) 
def GetRandomImm():
    return "#"+hex(random.randint(0x0, 0xFFFF))

def TestMOV():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MOV
    iType = 3
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    commandStr = "MOV "+rd[1]+" "+r1[1]
    print("Testing " + commandStr)
    return False

def TestMOVI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MOVI
    iType = 4
    rd = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "MOVI "+rd[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestMOVETOP():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MOVETOP
    iType = 4
    rd = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "MOVETOP "+rd[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestADD():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_ADD
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "ADD "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return False

def TestADDI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_ADDI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "ADDI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestSUB():
    global operand, rd, r1, r2, imm, iType
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "SUB "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return False

def TestSUBI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_SUBI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "SUBI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestAND():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_AND
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "AND "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return False

def TestANDI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_ANDI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "ANDI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestOR():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_OR
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "OR "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return False

def TestORI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_ORI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "ORI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestXOR():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_XOR
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "XOR "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return False

def TestXORI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_XORI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "XORI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestLSL():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_LSL
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "LSL "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return False

def TestLSLI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_LSLI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "LSLI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestLSR():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_LSR
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "LSR "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return False

def TestLSRI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_LSRI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "LSRI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestMUL():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MUL
    iType = 5
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    r2 = GetRandomRegister()
    commandStr = "MUL "+rd[1]+" "+r1[1]+" "+r2[1]
    print("Testing " + commandStr)
    return False

def TestMULI():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_MULI
    iType = 6
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    imm = GetRandomImm()
    commandStr = "MULI "+rd[1]+" "+r1[1]+" "+imm[1]
    print("Testing " + commandStr)
    return False

def TestNEGATE():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_NEGATE
    iType = 3
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    commandStr = "NEGATE "+rd[1]+" "+r1[1]
    print("Testing " + commandStr)
    return False

def TestCompare():
    global operand, rd, r1, r2, imm, iType
    operand = BOOTSTRAP_INTERP_CMP
    iType = 3
    rd = GetRandomRegister()
    r1 = GetRandomRegister()
    commandStr = "CMP "+rd[1]+" "+r1[1]
    print("Testing " + commandStr)
    return True

operations = [TestMOV, TestMOVI, TestMOVETOP, TestADD, TestADDI, TestSUB, TestSUBI, TestAND, TestANDI, TestOR, TestORI, TestXOR, TestXORI, TestLSL, TestLSLI, TestLSR, TestLSRI, TestMUL, TestMULI, TestNEGATE, TestCompare];

def SetUpOperation():
    # Set Instruction of 0 to command and set PC to 0
    return True

def AfterOperation():
    # Read Different Stuff
    readOperand = debugger.readMemoryValue(operandAddr, 8)
    readRd = debugger.readMemoryValue(rdAddr, 8)
    readR1 = debugger.readMemoryValue(r1Addr, 8)
    readR2 = debugger.readMemoryValue(r2Addr, 8)
    readImm = debugger.readMemoryValue(immAddr, 32)

    print(str(readOperand))
    print(str(readRd))
    print(str(readR1))
    print(str(readR2))
    print(str(readImm))

    if readOperand != operand:
        print("OPERAND IS WRONG")
        return False

    if iType == 3:
        if(rd == readRd and r1 == readR1):
            return True
        else:
            return False
    elif iType == 4:
        if(rd == readRd and imm == readImm):
            return True
        else:
            return False
    elif iType == 5:
        if(rd == readRd and r1 == readR1 and r2 == readR2):
            return True
        else:
            return False
    elif iType == 6:
        if(rd == readRd and r1 == readR1 and imm == readImm):
            return True
        else:
            return False






StartOfTest = debugger.setHardwareAddressBreakpoint(0x0010017A).getId()
comparison = debugger.setHardwareAddressBreakpoint(0x0010038A).getId()

firstExecution = True
while(True):
    execution_service.waitForStop(0)
    hitBreakPoint = breakpointService.getHitBreakpoint()

    if hitBreakPoint == None:
        execution_service.resume()
        continue

    hitBreakId = hitBreakPoint.getId()
    if hitBreakId == StartOfTest:
        if not firstExecution:
            if not AfterOperation():
                print("!!!GOT THE WRONG RESULT!!! expected " + hex(res) + " but got " + hex(result))
                break
            else:
                print("Got the correct result, which was expected")
        SetUpOperation()
        firstExecution = False
    if hitBreakId == comparison:
        if not expect_compare:
            print("!!!DID NOT EXPECT A COMPARISON!!!")
            break
        else:
            val1 = debugger.readRegister("r0")
            val2 = debugger.readRegister("r1")
            isSame = str(val1) == str(val2)

            if isSame != compareResult:
                print("!!!COMPARE RESULT IS NOT CORRECT!!!")
                break
            else:
                print("Expected the comparison to " + ("succeed" if compareResult else "fail") + " and it did.")
        execution_service.setExecutionAddress(0x00100178) # Return to start of test


debugger.removeAllBreakpoints()