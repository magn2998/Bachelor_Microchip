import sys,os
import lan966x
import random
import re

from arm_ds.debugger_v1 import Debugger
from arm_ds.debugger_v1 import DebugException

def GetLabelAddress(label_name, file_path):
    with open(file_path, 'r') as f:
        for line in f:
            match = re.search(r'^([0-9a-fA-F]+)\s+<' + label_name + '>:', line)
            if match:
                addr = match.group(1)
                addr_int = int(addr, 16)
                return addr_int
    return None



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

startInstr = GetLabelAddress("HAMMERPOPULATE",  buildSrc + "/bl2u/bl2u.dump") - 10
populateInstr = GetLabelAddress("HAMMERCHECK",  buildSrc + "/bl2u/bl2u.dump") - 2
endInstr = GetLabelAddress("ENDHAMMERTEST",  buildSrc + "/bl2u/bl2u.dump")
memoryReg = "r6"
statusFlagReg = "r5"

print(hex(startInstr))
print(hex(populateInstr))
print(hex(endInstr))

EndOfTest = debugger.setHardwareAddressBreakpoint(endInstr).getId()
EndOfPopulation = debugger.setHardwareAddressBreakpoint(populateInstr).getId()


def EndOfTestEvent(expectedAddr, shouldFail):
    # Code to be executed when the breakpoint is hit
    hasFoundError = debugger.readRegister(statusFlagReg)
    failedMemAddress = debugger.readRegister(memoryReg) - 4
    
    if (hasFoundError == 1 and not shouldFail):
        print("Found No Error as expected.")
        return True
    elif not shouldFail:
        print("!!! Expected no Error, but error was found!!!")
    elif hasFoundError == 1:
        print("!!! Expected Error but no error was detected!!!")
    elif failedMemAddress != expectedAddr:
        print("!!! Found Error But Not At Correct Address!!!")
    else:
        print("Found correct error.")
        return True
    return False



# Called when the Random Test is done populating the memory - now write to random memory address
def PopulatingDoneEvent():
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
    if hitBreakId == EndOfTest:
        if not EndOfTestEvent(failAtAddress, expectError):
            break 
        debugger.writeRegister(memoryReg, 0x60000000) # Reset Memory Address
        debugger.writeRegister(statusFlagReg, 0) # Reset Status Flag
        execution_service.setExecutionAddress(startInstr) # Return to first address
    if hitBreakId == EndOfPopulation:
        failAtAddress, expectError = PopulatingDoneEvent()

#execution_service.resume()
    