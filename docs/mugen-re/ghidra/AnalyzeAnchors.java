// Ghidra headless postScript: decompile functions around known anchors in mugen.exe.
// Usage: analyzeHeadless <projdir> <proj> -import mugen.exe -scriptPath <this dir> -postScript AnalyzeAnchors.java
// Writes decompiled C to docs/mugen-re/decompiled/ relative to CWD you launch analyzeHeadless from.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.File;
import java.io.FileWriter;
import java.util.LinkedHashSet;
import java.util.Set;

public class AnalyzeAnchors extends GhidraScript {

    // Core fight-engine seeds (see docs/mugen-re/FINDINGS.md, passes 1-3)
    private static final String[] DEFAULT_SEEDS = {
        // Expression evaluator + redirects
        "00443dd0",                     // expression operand iterator (eval core)
        "00443b80", "00443d80",         // trigger evaluator / redirect region
        "00482b9e",                     // redirect name dispatch (parent/root/helper/target/...)
        // Per-frame executor + hitpause gate
        "0044f213", "0044f288", "0044f33f",
        // Controller param readers (ctrl.time)
        "00451c20", "00454230",
        // Controller dispatch chain (representative entries span the whole table)
        "0047401e",                     // chain head (tagin/changestate)
        "00474a25",                     // hitdef entry in dispatch
        "0047628b",                     // varset/varadd cluster
        "00477045",                     // explodbindtime / modifyexplod
        "004771c1",                     // helper entry
        // HitDef field schema parse (pass 3: full ordered field list)
        "0047d865",                     // attr reader (schema head)
        "0047e2c8",                     // ground.velocity cluster
        "0047ede6",                     // guardflag/hitflag cluster
        // Trigger vocabulary chains
        "00482e34",                     // core trigger chain head (time/statetime/anim/vel/pos)
        "00483057",                     // long-name chain (roundstate/life/power/...)
        "004841da",                     // gethitvar field names
        // SuperPause / helper param blocks
        "004824b4", "00482000",
        // Trigger compiler + CNS parser
        "004859c6", "0048883e",         // 'Invalid trigger' / 'Missing trigger code'
        "0048a900", "0048ab00",         // CNS statedef section scanner
        "0048b5f0",                     // 'ctrl in statedef' validator
        "00477ce5", "00478156"          // triggerall/trigger%i readers, ignorehitpause
    };

    @Override
    public void run() throws Exception {
        String[] seeds = getScriptArgs().length > 0 ? getScriptArgs() : DEFAULT_SEEDS;

        File outdir = new File("docs/mugen-re/decompiled");
        outdir.mkdirs();
        FileWriter index = new FileWriter(new File(outdir, "INDEX.txt"));

        DecompInterface ifc = new DecompInterface();
        ifc.openProgram(currentProgram);

        Set<String> seen = new LinkedHashSet<>();
        for (String s : seeds) {
            Address a = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(s);
            Function f = getFunctionContaining(a);
            if (f == null) {
                // seed may be an uncalled function entry; try to create
                createFunction(a, null);
                f = getFunctionAt(a);
            }
            if (f == null) {
                printf("NO-FUNCTION at %s%n", s);
                index.write(String.format("NO-FUNCTION %s%n", s));
                continue;
            }
            String entry = f.getEntryPoint().toString();
            if (!seen.add(entry)) continue; // same function already decompiled

            DecompileResults r = ifc.decompileFunction(f, 120, monitor);
            String head = String.format(
                "FUNC %s  size=0x%x  params_seed=%s  decompiled=%s%n",
                entry, f.getBody().getNumAddresses(), s, r.decompileCompleted());
            printf("%s", head);
            index.write(head);

            if (r.decompileCompleted()) {
                String c = r.getDecompiledFunction().getC();
                FileWriter w = new FileWriter(new File(outdir, "fun_" + entry + ".c"));
                w.write("// entry " + entry + "  (seed " + s + ")\n");
                w.write(c);
                w.close();
            }
        }
        index.close();
        printf("AnalyzeAnchors done -> %s%n", outdir.getAbsolutePath());
    }
}
