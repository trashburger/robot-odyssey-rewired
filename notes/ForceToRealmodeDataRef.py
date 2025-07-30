# Convert all selected 16-bit scalars to data segment memory references
#@author Micah
#@category RealMode
#@keybinding
#@menupath Tools/ForceToRealmodeDataRef
#@toolbar

from ghidra.program.model.mem import MemoryAccessException
from ghidra.program.model.symbol import FlowType
from ghidra.program.model.listing import FlowOverride
from ghidra.program.model.scalar import Scalar
from ghidra.program.model.lang import OperandType, Register
from ghidra.program.model.symbol import RefTypeFactory
from ghidra.program.model.listing import Instruction

cs = currentProgram.getLanguage().getRegister("cs")
ds = currentProgram.getLanguage().getRegister("ds")
es = currentProgram.getLanguage().getRegister("es")
segments = [cs, ds, es]

for unit in currentProgram.getListing().getCodeUnits(currentSelection, True):
    if not isinstance(unit, Instruction):
        continue
    selected_seg = unit.getRegisterValue(ds)

    for operand in range(unit.getNumOperands()):
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
