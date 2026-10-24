// Ghidra headless script (project-owned). Decompiles the functions containing
// the given addresses, or the functions referencing strings matching
// "str:<regex>", and writes C to the output file. Output contains decompiled
// code of the user's game and must stay local (build/, git-ignored).
// Usage: -postScript DecompileAt.java <out.c> <addr|str:regex|xref:addr> ...
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;

import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.regex.Pattern;

public class DecompileAt extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        PrintWriter out = new PrintWriter(new FileWriter(args[0]));
        Set<Function> fs = new LinkedHashSet<>();
        for (int i = 1; i < args.length; i++) {
            String a = args[i];
            if (a.startsWith("str:")) {
                Pattern p = Pattern.compile(a.substring(4), Pattern.CASE_INSENSITIVE);
                DataIterator it = currentProgram.getListing().getDefinedData(true);
                while (it.hasNext()) {
                    Data d = it.next();
                    if (!d.hasStringValue() || d.getValue() == null) continue;
                    if (!p.matcher(d.getValue().toString()).find()) continue;
                    for (Reference r : getReferencesTo(d.getAddress())) {
                        Function f = getFunctionContaining(r.getFromAddress());
                        if (f != null) fs.add(f);
                    }
                }
            } else if (a.startsWith("xref:")) {
                Address t = toAddr(a.substring(5));
                for (Reference r : getReferencesTo(t)) {
                    Function f = getFunctionContaining(r.getFromAddress());
                    if (f != null) fs.add(f);
                    out.printf("// xref %s from %s%n", t, r.getFromAddress());
                }
            } else {
                Function f = getFunctionContaining(toAddr(a));
                if (f != null) fs.add(f);
            }
        }
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        for (Function f : fs) {
            DecompileResults r = di.decompileFunction(f, 120, monitor);
            out.printf("// ===== %s @ %s%n", f.getName(), f.getEntryPoint());
            out.println(r.getDecompiledFunction() == null ? "// (failed)" : r.getDecompiledFunction().getC());
        }
        out.close();
    }
}
