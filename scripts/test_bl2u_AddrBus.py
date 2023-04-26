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

startInstr = GetLabelAddress("ADDRBUSLOOP1",  buildSrc + "/bl2u/bl2u.dump") - 18
populateInstr = GetLabelAddress("ADDRBUSLOOP2",  buildSrc + "/bl2u/bl2u.dump")
didntFailInstr = GetLabelAddress("SKIPADDRCHECK",  buildSrc + "/bl2u/bl2u.dump") + 12
endInstr = GetLabelAddress("ENDADDRTEST",  buildSrc + "/bl2u/bl2u.dump")
memoryReg = "r0"
statusFlagReg = "r12"

print(hex(startInstr))
print(hex(populateInstr))
print(hex(endInstr))

EndOfTest = debugger.setHardwareAddressBreakpoint(endInstr).getId()
EndOfPopulation = debugger.setHardwareAddressBreakpoint(populateInstr).getId()
DidntFail = debugger.setHardwareAddressBreakpoint(didntFailInstr).getId()


def EndOfTestEvent(expectedAddr, shouldFail):
    # Code to be executed when the breakpoint is hit
    hasFoundError = debugger.readRegister(statusFlagReg)

    if (hasFoundError == 1 and not shouldFail):
        print("Found No Error as expected.")
        return True
    elif not shouldFail:
        print("!!! Expected no Error, but error was found!!!")
    elif hasFoundError == 1:
        print("!!! Expected Error but no error was detected!!!")
    else:
        print("Found correct error.")
        return True
    return False



# Called when the Random Test is done populating the memory - now write to random memory address
def PopulatingDoneEvent():
    if random.random() < 0.05: #Much lower probability of failing - runs very fast so this is fine
        readValue = debugger.readRegister(memoryReg)
        
        bitPos = random.randint(0, 27)
        writeAddr = 0x60000000 ^ (1 << bitPos)
        
        if writeAddr == readValue: # If they by chance are the same
            print("Leaving memory intact")
            return 0, False
            
        
        debugger.writeMemoryValue(writeAddr, 0xe3, 8) 
        print("Putting incorrect value at addr" + hex(writeAddr & 0xffffffff) + " at address to related register")
        #value_as_array = value.to_bytes(4, byteorder='little')
        #debugger.writeMemory(0x60000300, value_as_array)
        return 0, True
    else:
        print("Leaving memory intact")
        return 0, False


def DidntFailEvent(shouldFail):
    if shouldFail:
        print("Expected Error, but no error was reported")
        return False
    else:
        print("Found No Error as Expected")
        return True

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
    if hitBreakId == DidntFail:
        if not DidntFailEvent(expectError):
            break;

#execution_service.resume()
    