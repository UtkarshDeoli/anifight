// List function boundaries overlapping a range passed as script args (hex lo hi).
// Used by RE pass 9 to recover real function entries for the engine-object routines
// that angr's CFGFast splits/merges. No decompiler needed (native decompiler has no
// mac_arm_64 build).
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.address.Address;
import java.util.*;

public class ListFuncs extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        long lo = Long.parseLong(args[0].replaceFirst("^0[xX]", ""), 16);
        long hi = Long.parseLong(args[1].replaceFirst("^0[xX]", ""), 16);
        Address a = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(lo);
        Address b = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(hi);
        List<Function> fs = new ArrayList<>();
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            long s = f.getEntryPoint().getOffset();
            if (s > hi) break;
            if (s >= lo) fs.add(f);
        }
        println("COUNT " + currentProgram.getFunctionManager().getFunctionCount());
        for (Function f : fs) {
            long s = f.getEntryPoint().getOffset();
            long e = f.getBody().getMaxAddress().getOffset();
            println(String.format("FUNC 0x%06x size=0x%x name=%s", s, (e - s + 1), f.getName()));
        }
    }
}
