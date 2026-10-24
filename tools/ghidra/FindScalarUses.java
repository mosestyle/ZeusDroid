// Ghidra headless script (project-owned). Lists instructions whose operands
// contain one of the given constants (e.g. table base + field offset) and the
// containing function. Output: addresses and function names only.
// Usage: -postScript FindScalarUses.java <out.txt> <hex> [<hex> ...]
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;

import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.HashSet;
import java.util.Set;

public class FindScalarUses extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter out = new PrintWriter(new FileWriter(a[0]));
        Set<Long> want = new HashSet<>();
        for (int i = 1; i < a.length; i++) want.add(Long.parseLong(a[i], 16));
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            for (int op = 0; op < ins.getNumOperands(); op++) {
                for (Object o : ins.getOpObjects(op)) {
                    if (o instanceof Scalar) {
                        long v = ((Scalar) o).getUnsignedValue();
                        if (want.contains(v)) {
                            Function f = getFunctionContaining(ins.getAddress());
                            out.printf("%s %x %s %s%n", ins.getAddress(), v,
                                f == null ? "<none>" : f.getName() + "@" + f.getEntryPoint(), ins);
                        }
                    }
                }
            }
        }
        out.close();
    }
}
