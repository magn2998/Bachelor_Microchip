import sys,os
import lan966x
import random

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
r1 = 0
r2 = 0
imm = 0
res = 0
result = 0 # This is what is read, and res is what is expected

# Setup where to put the values in memory
operandAddr = 0x0010E030
rdAddr = 0x0010E031 #rd is always r5 [7]
r1Addr = 0x0010E032
r2Addr = 0x0010E033
immAddr = 0x0010E034

PcAddr = 0x0010E02E

# Where their values are stored in memory
r1ValAddr = 0x0010E04C
r2ValAddr = 0x0010E050
rdValAddr = 0x0010E05C

#Variable used to indicate if you expect a compare command and one that indicate if the compare should be true or not
expect_compare = False
compareResult = False

# Instruction Addresses
InstrAddrSwitch = 0x0010214E
CMPAddr         = 0x0010234E

def TestMOV():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_MOV
    r1 = random.randint(0x0, 0xFFFFFFFF)
    res = r1
    print("Testing MOV - Moves " + hex(r1) + " into rd")
    return False

def TestMOVI():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_MOVI
    imm = random.randint(0x0, 0xFFFF)
    res = imm
    print("Testing MOVI - Moves imm " + hex(imm) + " into rd")
    return False

def TestMOVETOP():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_MOVETOP
    imm = random.randint(0x0, 0xFFFF)
    currentValue = debugger.readMemoryValue(rdValAddr, 32)
    res = ((currentValue & 0xFFFF) | (imm << 16)) & 0xFFFFFFFF
    print("Testing MOVETOP - Move " + hex(imm) + " Into top 16 bits of rd with current value of " + hex(currentValue))
    return False

def TestADD():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_ADD
    r1 = random.randint(0x0, 0xFFFFFFFF)
    r2 = random.randint(0x0, 0xFFFFFFFF)
    res = (r1+r2) & 0xFFFFFFFF
    print("Testing ADD. " + str(r1) + " + " + str(r2) + " = " + str(res))
    return False

def TestADDI():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_ADDI
    r1 = random.randint(0x0, 0xFFFFFFFF)
    imm = random.randint(0x0, 0xFFFF)
    res = (r1+imm) & 0xFFFFFFFF
    print("Testing ADDI. " + str(r1) + " + " + str(imm) + " = " + str(res))
    return False

def TestSUB():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_SUB
    r1 = random.randint(0x0, 0xFFFFFFFF)
    r2 = random.randint(0x0, 0xFFFFFFFF)
    res = (r1-r2) & 0xFFFFFFFF
    print("Testing SUB. " + str(r1) + " - " + str(r2) + " = " + str(res))
    return False

def TestSUBI():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_SUBI
    r1 = random.randint(0x0, 0xFFFFFFFF)
    imm = random.randint(0x0, 0xFFFF)
    res = (r1-imm) & 0xFFFFFFFF
    print("Testing SUBI. " + str(r1) + " - " + str(imm) + " = " + str(res))
    return False

def TestAND():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_AND
    r1 = random.randint(0x0, 0xFFFFFFFF)
    r2 = random.randint(0x0, 0xFFFFFFFF)
    res = r1 & r2
    print("Testing AND. " + hex(r1) + " & " + hex(r2) + " = " + hex(res))
    return False

def TestANDI():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_ANDI
    r1 = random.randint(0x0, 0xFFFFFFFF)
    imm = random.randint(0x0, 0xFFFF)
    res = r1 & imm
    print("Testing ANDI. " + hex(r1) + " & " + hex(imm) + " = " + hex(res))
    return False

def TestOR():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_OR
    r1 = random.randint(0x0, 0xFFFFFFFF)
    r2 = random.randint(0x0, 0xFFFFFFFF)
    res = r1 | r2
    print("Testing OR. " + hex(r1) + " | " + hex(r2) + " = " + hex(res))
    return False

def TestORI():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_ORI
    r1 = random.randint(0x0, 0xFFFFFFFF)
    imm = random.randint(0x0, 0xFFFF)
    res = r1 | imm
    print("Testing ORI. " + hex(r1) + " | " + hex(imm) + " = " + hex(res))
    return False

def TestXOR():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_XOR
    r1 = random.randint(0x0, 0xFFFFFFFF)
    r2 = random.randint(0x0, 0xFFFFFFFF)
    res = r1 ^ r2
    print("Testing XOR. " + hex(r1) + " ^ " + hex(r2) + " = " + hex(res))
    return False

def TestXORI():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_XORI
    r1 = random.randint(0x0, 0xFFFFFFFF)
    imm = random.randint(0x0, 0xFFFF)
    res = r1 ^ imm
    print("Testing XORI. " + hex(r1) + " ^ " + hex(imm) + " = " + hex(res))
    return False

def TestLSL():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_LSL
    r1 = random.randint(0x0, 0xFFFFFFFF)
    r2 = random.randint(0x0, 0x28)
    res = (r1 << r2) & 0xFFFFFFFF
    print("Testing LSL. " + hex(r1) + " << " + str(r2) + " = " + hex(res))
    return False

def TestLSLI():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_LSLI
    r1 = random.randint(0x0, 0xFFFFFFFF)
    imm = random.randint(0x0, 0x28)
    res = (r1 << imm) & 0xFFFFFFFF
    print("Testing LSLI. " + hex(r1) + " << " + str(imm) + " = " + hex(res))
    return False

def TestLSR():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_LSR
    r1 = random.randint(0x0, 0xFFFFFFFF)
    r2 = random.randint(0x0, 0x28)
    res = (r1 >> r2) & 0xFFFFFFFF
    print("Testing LSR. " + hex(r1) + " >> " + str(r2) + " = " + hex(res))
    return False

def TestLSRI():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_LSRI
    r1 = random.randint(0x0, 0xFFFFFFFF)
    imm = random.randint(0x0, 0x28)
    res = (r1 >> imm) & 0xFFFFFFFF
    print("Testing LSRI. " + hex(r1) + " >> " + str(imm) + " = " + hex(res))
    return False

def TestMUL():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_MUL
    r1 = random.randint(0x0, 0xFFFFFFFF)
    r2 = random.randint(0x0, 0xFFFFFFFF)
    res = (r1 * r2) & 0xFFFFFFFF
    print("Testing MUL. " + str(r1) + " * " + str(r2) + " = " + hex(res))
    return False

def TestMULI():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_MULI
    r1 = random.randint(0x0, 0xFFFFFFFF)
    imm = random.randint(0x0, 0xFFFFFFFF)
    res = (r1 * imm) & 0xFFFFFFFF
    print("Testing MULI. " + str(r1) + " * " + str(imm) + " = " + hex(res))
    return False

def TestNEGATE():
    global operand, r1, r2, imm, res
    operand = BOOTSTRAP_INTERP_NEGATE
    r1 = random.randint(0x0, 0xFFFFFFFF)
    res = (~r1) & 0xFFFFFFFF
    print("Testing NEGATE. ~" + hex(r1) + " = " + hex(res))
    return False

def TestCompare():
    global operand, r1, r2, imm, res, compareResult
    operand = BOOTSTRAP_INTERP_CMP
    r1 = random.randint(0, 0xFFFFFFFF)
    compareResult = random.choice([True, False]) 
    temp = 0
    if compareResult: # Same
        temp = r1
    else: # Different
        temp = r1 ^ (1 << (random.randint(0, 31)))
    
    debugger.writeMemoryValue(rdValAddr, temp, 32) #Since compare is CMP RD and R1, this is required

    print("Testing Compare! Compares " + hex(r1) + " against " + hex(temp))
    return True

operations = [TestMOV, TestMOVI, TestMOVETOP, TestADD, TestADDI, TestSUB, TestSUBI, TestAND, TestANDI, TestOR, TestORI, TestXOR, TestXORI, TestLSL, TestLSLI, TestLSR, TestLSRI, TestMUL, TestMULI, TestNEGATE, TestCompare];

def SetUpOperation():
    global expect_compare
    rnd_operation = random.choice(operations)
    expect_compare = rnd_operation()
    # Set The Correct Values
    debugger.writeMemoryValue(operandAddr, operand, 8)
    debugger.writeMemoryValue(rdAddr, 7, 8)
    debugger.writeMemoryValue(r1Addr, 3, 8)
    debugger.writeMemoryValue(r2Addr, 4, 8)
    debugger.writeMemoryValue(immAddr, imm, 32)
    # Set correct value in memory
    debugger.writeMemoryValue(r1ValAddr, r1, 32)
    debugger.writeMemoryValue(r2ValAddr, r2, 32)
    # Set PC value to 0, to ensure it doesn't time out
    debugger.writeMemoryValue(PcAddr, 0x0, 8)
    

def AfterOperation():
    global result
    # Read Value of result
    if expect_compare:
        return True
    result = debugger.readMemoryValue(rdValAddr, 32)
    print("Result: Expected " + hex(res) + " and got " + hex(result))
    return res == result




StartOfTest = debugger.setHardwareAddressBreakpoint(InstrAddrSwitch).getId()
comparison = debugger.setHardwareAddressBreakpoint(CMPAddr).getId()

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
        execution_service.setExecutionAddress(InstrAddrSwitch-2) # Return to start of test


debugger.removeAllBreakpoints()