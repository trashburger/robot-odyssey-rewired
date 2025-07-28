# Ghidra command to apply hacky real-mode fixups to the current selection.
#@author Micah 
#@category RealMode
#@keybinding
#@menupath Tools/RoRealmodeFixups
#@toolbar 

# Fix near call/jmp in 16-bit real mode

from ghidra.program.model.mem import MemoryAccessException
from ghidra.program.model.symbol import FlowType
from ghidra.program.model.listing import FlowOverride
from ghidra.program.model.scalar import Scalar
from ghidra.program.model.lang import OperandType, Register
from ghidra.program.model.symbol import RefTypeFactory
from ghidra.program.model.listing import Instruction

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

# Fix segment addressing in 16-bit real mode

cs = currentProgram.getLanguage().getRegister("cs")
ds = currentProgram.getLanguage().getRegister("ds")
es = currentProgram.getLanguage().getRegister("es")
segments = [cs, ds, es]

for unit in currentProgram.getListing().getCodeUnits(currentSelection, True):
    if not isinstance(unit, Instruction):
        continue
    selected_seg = unit.getRegisterValue(ds)

    for operand in range(unit.getNumOperands()):

        for ref in unit.getOperandReferences(operand):
            if ref.isMemoryReference() and ref.getReferenceType().isData() and True:
                removeReference(ref)

        if OperandType.isDynamic(unit.getOperandType(operand)):
            for obj in unit.getOpObjects(operand):
                if isinstance(obj, Register):
                    for segment in segments:
                        if 0 == segment.compareTo(obj):
                            selected_seg = unit.getRegisterValue(segment)
                if isinstance(obj, Scalar) and obj.bitLength() == 16 and selected_seg:
                    off_val = obj.getUnsignedValue()
                    seg_val = selected_seg.getUnsignedValue()
                    target = unit.getMinAddress().getAddressSpace().getAddress(seg_val, off_val)
                    reftype = RefTypeFactory().getDefaultMemoryRefType(unit, operand, target, True)
                    print(unit, reftype, target)
                    if True:
                       createMemoryReference(unit, operand, target, reftype)
