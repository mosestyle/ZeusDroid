import org.zeusport.game.ImportRules;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.file.Files;
import java.util.List;

/** Plain-JVM tests for the Android importer rules (no Android needed). */
public class ImportRulesTest {
    static int checks = 0, failures = 0;

    static void eq(Object expected, Object actual, String what) {
        checks++;
        boolean ok = expected == null ? actual == null : expected.equals(actual);
        if (!ok) { failures++; System.out.println("  FAIL " + what + ": expected " + expected + " got " + actual); }
    }

    static void touch(File root, String rel) throws Exception {
        File f = new File(root, rel);
        f.getParentFile().mkdirs();
        try (FileOutputStream o = new FileOutputStream(f)) { o.write(1); }
    }

    public static void main(String[] a) throws Exception {
        System.out.println("normalize / traversal");
        eq(null, ImportRules.normalize("../etc/passwd"), "dotdot");
        eq(null, ImportRules.normalize("/abs/file"), "absolute");
        eq(null, ImportRules.normalize("C:\\Windows\\x"), "drive");
        eq(null, ImportRules.normalize("DATA/a/../../x"), "inner dotdot");
        eq("DATA/x.sg3", ImportRules.normalize("DATA\\x.sg3"), "backslash");
        eq("DATA/x.sg3", ImportRules.normalize("./DATA//x.sg3"), "dot and empty segments");
        eq(null, ImportRules.normalize("DATA/\u0001bad"), "control char");

        System.out.println("game folder mapping");
        eq("DATA/Zeus_Terrain.sg3", ImportRules.mapGameFolderFile("DATA/Zeus_Terrain.sg3"), "data");
        eq("DATA/Zeus_Terrain.sg3", ImportRules.mapGameFolderFile("data/Zeus_Terrain.sg3"), "data case-insensitive dir");
        eq("Audio/Music/Setup.mp3", ImportRules.mapGameFolderFile("Audio/Music/Setup.mp3"), "audio");
        eq("Adventures/The Odyssey/The Odyssey.pak", ImportRules.mapGameFolderFile("Adventures/The Odyssey/The Odyssey.pak"), "adventure");
        eq("Zeus_Text.eng", ImportRules.mapGameFolderFile("zeus_text.eng"), "eng file");
        eq(null, ImportRules.mapGameFolderFile("Zeus.exe"), "exe skipped");
        eq(null, ImportRules.mapGameFolderFile("mss32.dll"), "dll skipped");
        eq(null, ImportRules.mapGameFolderFile("Binks/intro.bik"), "movies skipped");
        eq(null, ImportRules.mapGameFolderFile("Save/Moses/game.sav"), "original saves not imported");
        eq(null, ImportRules.mapGameFolderFile("Zeus - Manual.pdf"), "manual skipped");
        eq("eZeus/interface.e", ImportRules.mapGameFolderFile("eZeus-0.8.2-beta/interface.e"), "ezeus pack inside game folder");
        eq("eZeus/Fonts/Zeus.ttf", ImportRules.mapGameFolderFile("eZeus-0.8.2-beta/Fonts/Zeus.ttf"), "ezeus font");
        eq(null, ImportRules.mapGameFolderFile("eZeus-0.8.2-beta/Bin/SDL2.dll"), "ezeus windows bin skipped");
        eq(null, ImportRules.mapGameFolderFile("eZeus-0.8.2-beta/Save/moses/x.ez"), "ezeus saves never overwritten");
        eq(null, ImportRules.mapGameFolderFile("eZeus-0.8.2-beta/settings.txt"), "settings untouched");
        eq(null, ImportRules.mapGameFolderFile("../DATA/x"), "traversal rejected");

        System.out.println("eZeus archive mapping");
        eq("eZeus/i30.e", ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta/i30.e"), "i30");
        eq("eZeus/Text/language.txt", ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta/Text/language.txt"), "language");
        eq("eZeus/i45.e", ImportRules.mapEZeusArchiveEntry("i45.e"), "loose i45");
        // ZeusDroid's own texture-pack backup (0.1.18) imports with the same rules.
        eq("eZeus/i30.e", ImportRules.mapEZeusArchiveEntry("eZeus-texture-pack/i30.e"), "backup i30");
        eq("eZeus/interface.e", ImportRules.mapEZeusArchiveEntry("eZeus-texture-pack/interface.e"), "backup interface");
        eq("eZeus/Fonts/Zeus.ttf", ImportRules.mapEZeusArchiveEntry("eZeus-texture-pack/Fonts/Zeus.ttf"), "backup font");
        eq("eZeus/Text/language.txt", ImportRules.mapEZeusArchiveEntry("eZeus-texture-pack/Text/language.txt"), "backup language");
        eq(null, ImportRules.mapEZeusArchiveEntry("readme.txt"), "loose other skipped");
        eq(null, ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta/VC_redist.x64.exe"), "vc redist skipped");
        eq(null, ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta/engconverter-0.5.exe"), "engconverter skipped");
        eq(null, ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta/../../evil"), "zip slip rejected");
        eq(null, ImportRules.mapEZeusArchiveEntry("other/thing.e"), "foreign top folder skipped");

        System.out.println("source-code zip (user report 2026-10-22)");
        eq("eZeus/Fonts/Zeus.ttf", ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta.4/fonts/Zeus.ttf"), "lowercase fonts normalised");
        eq("eZeus/Text/language.txt", ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta.4/text/language.txt"), "lowercase text normalised");
        eq(null, ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta.4/main.cpp"), "sources not imported");
        eq(null, ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta.4/CMakeLists.txt"), "cmake not imported");
        eq(null, ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta.4/widgets/egamewidget.h"), "headers not imported");
        eq(true, ImportRules.looksLikeSourceArchiveEntry("eZeus-0.8.2-beta.4/main.cpp"), "source archive detected");
        eq(false, ImportRules.looksLikeSourceArchiveEntry("eZeus-0.8.2-beta/i30.e"), "release pack not flagged");
        eq("eZeus/Adventures/The Sands of Betrayal/x.epak",
           ImportRules.mapEZeusArchiveEntry("eZeus-0.8.2-beta/Adventures/The Sands of Betrayal/x.epak"), "ezeus adventure");
        eq(true, ImportRules.isLeftoverSourceFile("egamewidget.cpp"), "leftover cpp");
        eq(false, ImportRules.isLeftoverSourceFile("language.txt"), "language kept");

        System.out.println("safeResolve");
        File tmp = Files.createTempDirectory("zp").toFile();
        boolean threw = false;
        try { ImportRules.safeResolve(tmp, "a/../../x"); } catch (java.io.IOException e) { threw = true; }
        eq(true, threw, "escape detected");
        eq(new File(tmp, "DATA/x").getCanonicalPath(), ImportRules.safeResolve(tmp, "DATA/x").getCanonicalPath(), "inside ok");

        System.out.println("validate");
        List<ImportRules.Item> v = ImportRules.validate(tmp);
        eq(false, ImportRules.allRequiredOk(v), "empty root incomplete");
        for (String f : new String[] {"DATA/Zeus_Terrain.sg3", "DATA/Zeus_Terrain.555", "DATA/SprMain.sg3",
                "DATA/Zeus_Interface.sg3", "DATA/PoseidonImps.sg3", "DATA/Poseidon_Loaded.sg3",
                "Zeus_Text.eng", "Zeus_MM.eng", "Model/Zeus eventmsg.txt", "Adventures/agora.pak",
                "eZeus/interface.e", "eZeus/i30.e", "eZeus/Fonts/Zeus.ttf", "eZeus/Text/language.txt"}) {
            touch(tmp, f);
        }
        v = ImportRules.validate(tmp);
        eq(true, ImportRules.allRequiredOk(v), "all required present");
        new File(tmp, "eZeus/i30.e").delete();
        eq(false, ImportRules.allRequiredOk(ImportRules.validate(tmp)), "missing textures detected");

        System.out.println(checks + " checks, " + failures + " failures");
        System.exit(failures == 0 ? 0 : 1);
    }
}
