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

endOfRndTest = debugger.setHardwareAddressBreakpoint(0x00102AAA).getId()
endOfRndPopulate = debugger.setHardwareAddressBreakpoint(0x00102A44).getId()


def EndOfRandomTestCallBack(expectedAddr, shouldFail):
    # Code to be executed when the breakpoint is hit
    hasFoundError = debugger.readRegister("r7")
    failedMemAddress = debugger.readRegister("r0") - 4
    
    if (hasFoundError == 2 and not shouldFail):
        print("Found No Error as expected.")
        return True
    elif not shouldFail:
        print("!!! Expected no Error, but error was found!!!")
    elif hasFoundError == 2:
        print("!!! Expected Error but no error was detected!!!")
    elif failedMemAddress != expectedAddr:
        print("!!! Found Error But Not At Correct Address!!!")
    else:
        print("Found correct error.")
        return True
    return False
    

# Called when the Random Test is done populating the memory - now write to random memory address
def RandonTestPopulationDone():
    if random.random() < 0.75:
        addr = random.randint(0x60000000 >> 2, 0x7ffffffc >> 2) << 2
        readValue = debugger.readMemoryValue(addr, 32)
        
        bitPos = random.randint(0, 31)
        writeValue = readValue ^ (1 << bitPos)
        
        debugger.writeMemoryValue(addr, writeValue, 32) # Write negated value to induce error
        print("Putting incorrect value of " + hex(writeValue & 0xffffffff) + " at address " + hex(addr))
        #value_as_array = value.to_bytes(4, byteorder='little')
        #debugger.writeMemory(0x60000300, value_as_array)
        return addr, True
    else:
        print("Leaving memory intact")
        return 0, False

#Begin Execution and await breakpoint hit
execution_service.resume()

# Which address to fail at
failAtAddress = 0
expectError = True

while(True):
    execution_service.waitForStop(0)
    hitBreakPoint = breakpointService.getHitBreakpoint()
    
    if hitBreakPoint == None:
        execution_service.resume()
        continue
    
    hitBreakId = hitBreakPoint.getId()
    if hitBreakId == endOfRndTest:
        if not EndOfRandomTestCallBack(failAtAddress, expectError):
            break 
        debugger.writeRegister("r0", 0x60000000) # Reset Memory Address
        debugger.writeRegister("r6", random.randint(0x000000, 0xffffff)) # Choose new seed
        debugger.writeRegister("r7", 0) # Reset Status Flag
        execution_service.setExecutionAddress(0x00102A20) # Return to first address
    if hitBreakId == endOfRndPopulate:
        failAtAddress, expectError = RandonTestPopulationDone()

#execution_service.resume()
    