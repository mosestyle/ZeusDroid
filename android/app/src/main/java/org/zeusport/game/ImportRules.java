package org.zeusport.game;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Pure-Java rules describing which user-owned files are imported and where
 * they go. No Android dependencies so it can be unit-tested on the JVM
 * (tools/tests/run_java_tests.sh).
 *
 * Target layout under the app's external files dir (mirrors the desktop
 * eZeus layout "<game>/eZeus-x/Bin" so the engine needs no special casing):
 *
 *   ZeusPoseidon/                 original game data (read-only after import)
 *     DATA/ Audio/ Model/ Adventures/ Zeus_Text.eng Zeus_MM.eng zeus.ico
 *     eZeus/                      eZeus data pack + engine-writable files
 *       interface.e i15.e i30.e [i45.e i60.e] Fonts/ Text/ Adventures/
 *       Save/ settings.txt        (never touched by the importer)
 */
public final class ImportRules {
    private ImportRules() {}

    public static final String ROOT = "ZeusPoseidon";
    public static final String EZEUS = "eZeus";

    /** Max size of any single imported file (largest known is ~150 MB). */
    public static final long MAX_FILE_BYTES = 600L * 1024 * 1024;
    /** Max total bytes accepted from one archive (zip-bomb guard). */
    public static final long MAX_ARCHIVE_BYTES = 2L * 1024 * 1024 * 1024;

    private static final String[] GAME_DIRS = {"DATA/", "Audio/", "Model/", "Adventures/"};
    private static final String[] GAME_FILES = {"Zeus_Text.eng", "Zeus_MM.eng", "zeus.ico"};
    private static final String[] SKIP_EXT = {".exe", ".dll", ".asi", ".m3d", ".lnk", ".pdf", ".bik"};

    /**
     * Normalises an archive/tree relative path. Returns null for anything
     * unsafe (absolute, drive letters, "..", empty segments, control chars).
     */
    public static String normalize(String path) {
        if (path == null) return null;
        String p = path.replace('\\', '/');
        if (p.startsWith("/") || p.matches("^[A-Za-z]:.*")) return null;
        List<String> out = new ArrayList<>();
        for (String seg : p.split("/", -1)) {
            if (seg.isEmpty() || seg.equals(".")) continue;
            if (seg.equals("..")) return null;
            for (int i = 0; i < seg.length(); i++) {
                if (seg.charAt(i) < 0x20) return null;
            }
            out.add(seg);
        }
        if (out.isEmpty()) return null;
        return String.join("/", out);
    }

    private static boolean hasSkippedExtension(String p) {
        String l = p.toLowerCase(Locale.ROOT);
        for (String e : SKIP_EXT) if (l.endsWith(e)) return true;
        return false;
    }

    private static boolean isEZeusDirName(String seg) {
        return seg.toLowerCase(Locale.ROOT).startsWith("ezeus");
    }

    /** Maps an eZeus data-pack relative path (after its top folder). */
    static String mapEZeusInner(String inner) {
        if (inner == null) return null;
        String l = inner.toLowerCase(Locale.ROOT);
        if (l.startsWith("bin/")) return null;           // Windows binaries
        if (l.startsWith("save/")) return null;          // never overwrite saves
        if (l.equals("settings.txt")) return null;
        if (l.startsWith("vc_redist") || l.startsWith("engconverter")) return null;
        if (hasSkippedExtension(inner)) return null;
        return EZEUS + "/" + inner;
    }

    /**
     * Maps a path relative to the folder the user picked (expected to be a
     * copy of the PC "Zeus and Poseidon" install folder) to a destination
     * relative to ROOT, or null when the file is not needed.
     */
    public static String mapGameFolderFile(String rel) {
        String p = normalize(rel);
        if (p == null) return null;
        String first = p.contains("/") ? p.substring(0, p.indexOf('/')) : p;
        if (p.contains("/") && isEZeusDirName(first)) {
            return mapEZeusInner(p.substring(p.indexOf('/') + 1));
        }
        if (hasSkippedExtension(p)) return null;
        for (String d : GAME_DIRS) {
            if (p.regionMatches(true, 0, d, 0, d.length())) {
                // Keep original spelling of the top folder as the engine expects it.
                return d + p.substring(d.length());
            }
        }
        for (String f : GAME_FILES) {
            if (p.equalsIgnoreCase(f)) return f;
        }
        return null;
    }

    /**
     * Maps an entry of an eZeus release archive (eZeus-0.8.2-beta.zip, or the
     * optional i45.zip / i60.zip) to a destination relative to ROOT.
     */
    public static String mapEZeusArchiveEntry(String entryName) {
        String p = normalize(entryName);
        if (p == null) return null;
        if (!p.contains("/")) {
            // Loose files at archive root (i45.zip/i60.zip contain i45.e / i60.e).
            return p.toLowerCase(Locale.ROOT).endsWith(".e") ? EZEUS + "/" + p : null;
        }
        String first = p.substring(0, p.indexOf('/'));
        String inner = p.substring(p.indexOf('/') + 1);
        if (isEZeusDirName(first)) return mapEZeusInner(inner);
        return null;
    }

    /** Resolves dest under root and verifies it cannot escape it. */
    public static File safeResolve(File root, String destRel) throws java.io.IOException {
        File f = new File(root, destRel);
        String r = root.getCanonicalPath() + File.separator;
        String c = f.getCanonicalPath();
        if (!c.startsWith(r)) throw new java.io.IOException("path escapes import root: " + destRel);
        return f;
    }

    // ------------------------------------------------------------ validation

    public static final class Item {
        public final String label;
        public final boolean required;
        public final boolean ok;
        Item(String label, boolean required, boolean ok) {
            this.label = label; this.required = required; this.ok = ok;
        }
    }

    private static boolean exists(File root, String rel) {
        return new File(root, rel).isFile();
    }

    private static boolean dirHasFiles(File root, String rel, String suffix) {
        File[] fs = new File(root, rel).listFiles();
        if (fs == null) return false;
        for (File f : fs) {
            if (f.isFile() && (suffix == null || f.getName().toLowerCase(Locale.ROOT).endsWith(suffix))) return true;
        }
        return false;
    }

    /** Status of the imported data under {@code root} (= .../ZeusPoseidon). */
    public static List<Item> validate(File root) {
        List<Item> r = new ArrayList<>();
        r.add(new Item("Zeus graphics (DATA/Zeus_Terrain, SprMain, Interface)", true,
            exists(root, "DATA/Zeus_Terrain.sg3") && exists(root, "DATA/Zeus_Terrain.555")
                && exists(root, "DATA/SprMain.sg3") && exists(root, "DATA/Zeus_Interface.sg3")));
        r.add(new Item("Poseidon expansion graphics (DATA/Poseidon_*)", true,
            exists(root, "DATA/PoseidonImps.sg3") && exists(root, "DATA/Poseidon_Loaded.sg3")));
        r.add(new Item("Game texts (Zeus_Text.eng, Zeus_MM.eng)", true,
            (exists(root, "Zeus_Text.eng") || exists(root, EZEUS + "/Zeus_Text.xml"))
                && (exists(root, "Zeus_MM.eng") || exists(root, EZEUS + "/Zeus_MM.xml"))));
        r.add(new Item("Game model data (Model/)", true, exists(root, "Model/Zeus eventmsg.txt")));
        r.add(new Item("Adventures (Adventures/*.pak)", true, dirHasFiles(root, "Adventures", ".pak")));
        r.add(new Item("eZeus interface pack (interface.e)", true, exists(root, EZEUS + "/interface.e")));
        r.add(new Item("eZeus textures (i15.e or i30.e)", true,
            exists(root, EZEUS + "/i15.e") || exists(root, EZEUS + "/i30.e")));
        r.add(new Item("eZeus font and language (Fonts/Zeus.ttf, Text/language.txt)", true,
            exists(root, EZEUS + "/Fonts/Zeus.ttf") && exists(root, EZEUS + "/Text/language.txt")));
        r.add(new Item("Music (Audio/Music)", false, dirHasFiles(root, "Audio/Music", null)));
        r.add(new Item("Sound effects and voices (Audio/Wavs, Audio/Voice)", false,
            new File(root, "Audio/Wavs").isDirectory() && new File(root, "Audio/Voice").isDirectory()));
        r.add(new Item("High zoom textures (i45.e / i60.e)", false,
            exists(root, EZEUS + "/i45.e") || exists(root, EZEUS + "/i60.e")));
        return r;
    }

    public static boolean allRequiredOk(List<Item> items) {
        for (Item i : items) if (i.required && !i.ok) return false;
        return true;
    }
}
