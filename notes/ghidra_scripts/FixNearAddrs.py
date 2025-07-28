# Fix near call/jmp in 16-bit real mode
#@author Micah E Scott
#@category RealMode
#@keybinding 
#@menupath 
#@toolbar 

from ghidra.program.model.mem import MemoryAccessException
from ghidra.program.model.symbol import FlowType
from ghidra.program.model.listing import FlowOverride

for unit in currentProgram.getListing().getCodeUnits(currentSelection, True):
    try:
        if unit.getLength() == 3 and unit.getBytes()[0]&0xFF in [0xe8, 0xe9]:
            addr = unit.getMinAddress()
            segment = addr.getSegment()
            next = addr.getSegmentOffset() + unit.getLength()
            target_offset = (next + ((unit.getBytes()[2]&0xFF)<<8) + (unit.getBytes()[1]&0xFF))&0xFFFF
            target_addr = addr.getAddressSpace().getAddress(segment, target_offset)

            for old_ref in unit.getReferencesFrom():
                print(old_ref)
                if True:
                    removeReference(old_ref)

            print(unit, target_addr)
            if True:
                createMemoryReference(unit, 0, target_addr, {
                    0xe8: FlowType.UNCONDITIONAL_CALL,
                    0xe9: FlowType.UNCONDITIONAL_JUMP
                }[unit.getBytes()[0]&0xFF])
                unit.setFlowOverride(FlowOverride.NONE)

    except MemoryAccessException:
        pass
