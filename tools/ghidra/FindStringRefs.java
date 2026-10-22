// Ghidra headless script (project-owned). Lists defined strings matching a
// regular expression and the functions that reference them.
// Usage (see tools/ghidra/run_headless.sh):
//   analyzeHeadless <proj_dir> <proj_name> -process Zeus.exe -noanalysis \
//     -scriptPath tools/ghidra -postScript FindStringRefs.java "<regex>" <out.txt>
// Output contains addresses, string text and function names/entry points only
// (no decompiled code), so it can be kept in docs/research/.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;

import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.regex.Pattern;

public class FindStringRefs extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        Pattern p = Pattern.compile(args.length > 0 ? args[0] : ".*", Pattern.CASE_INSENSITIVE);
        PrintWriter out = new PrintWriter(new FileWriter(args.length > 1 ? args[1] : "/dev/stdout"));
        int nStrings = 0, nRefs = 0;
        DataIterator it = currentProgram.getListing().getDefinedData(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Data d = it.next();
            if (!d.hasStringValue()) continue;
            Object v = d.getValue();
            if (v == null) continue;
            String s = v.toString();
            if (!p.matcher(s).find()) continue;
            nStrings++;
            out.printf("STRING %s \"%s\"%n", d.getAddress(), s.replace("\n", "\\n"));
            for (Reference r : getReferencesTo(d.getAddress())) {
                Address from = r.getFromAddress();
                Function f = getFunctionContaining(from);
                out.printf("    ref %s in %s%n", from,
                    f == null ? "<no function>" : f.getName() + "@" + f.getEntryPoint());
                nRefs++;
            }
        }
        out.printf("# %d strings, %d references, %d functions in program%n", nStrings, nRefs,
            currentProgram.getFunctionManager().getFunctionCount());
        out.close();
    }
}
