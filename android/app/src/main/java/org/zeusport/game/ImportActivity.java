package org.zeusport.game;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Color;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.StatFs;
import android.provider.DocumentsContract;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * First-run screen: explains that user-owned game files are required, imports
 * them through the Storage Access Framework into app-owned storage (no broad
 * storage permission), validates the result and launches the native game.
 */
public class ImportActivity extends Activity {
    private static final String TAG = "ZeusImport";
    private static final int REQ_GAME_FOLDER = 1;
    private static final int REQ_EZEUS_ARCHIVE = 2;
    private static final long SPACE_MARGIN = 64L * 1024 * 1024;

    private final Handler mUi = new Handler(Looper.getMainLooper());
    private final AtomicBoolean mCancel = new AtomicBoolean(false);
    private volatile boolean mBusy = false;

    private LinearLayout mStatusList;
    private TextView mProgressText;
    private ProgressBar mProgress;
    private Button mStart, mPickFolder, mPickZip, mCancelButton;

    private File root() {
        File ext = getExternalFilesDir(null);
        if (ext == null) ext = getFilesDir();
        return new File(ext, ImportRules.ROOT);
    }

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        if (ImportRules.allRequiredOk(ImportRules.validate(root()))
                && !getIntent().getBooleanExtra("forceImportScreen", false)) {
            launchGame();
            return;
        }
        buildUi();
        refreshStatus();
    }

    private int dp(float v) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v,
            getResources().getDisplayMetrics());
    }

    private Button button(String text, View.OnClickListener l) {
        Button bt = new Button(this);
        bt.setText(text);
        bt.setAllCaps(false);
        bt.setMinHeight(dp(52));
        bt.setOnClickListener(l);
        return bt;
    }

    private void buildUi() {
        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Color.rgb(0x16, 0x22, 0x30));
        LinearLayout col = new LinearLayout(this);
        col.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(24);
        col.setPadding(pad, pad, pad, pad);
        scroll.addView(col);

        TextView title = new TextView(this);
        title.setText("Zeus + Poseidon — native Android port (eZeus engine)");
        title.setTextColor(Color.rgb(0xF2, 0xE6, 0xC8));
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        col.addView(title);

        TextView body = new TextView(this);
        body.setTextColor(Color.rgb(0xDD, 0xDD, 0xDD));
        body.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        body.setPadding(0, dp(12), 0, dp(12));
        body.setText(
            "This app contains only the open-source engine. To play you need your own copy of "
            + "Zeus: Master of Olympus with the Poseidon expansion (for example the GOG edition) "
            + "and the free eZeus data pack (eZeus-0.8.2-beta.zip from the eZeus GitHub releases).\n\n"
            + "1. Copy your \"Zeus and Poseidon\" install folder from your PC to this device "
            + "(USB, SD card or cloud) and pick it below. If you already unpacked eZeus into that "
            + "folder (eZeus-0.8.2-beta/), it is imported too.\n"
            + "2. Otherwise pick the eZeus zip separately. Optional i45.zip / i60.zip add closer zoom levels.\n\n"
            + "Files are copied into this app's private storage. Your saves are kept when you re-import. "
            + "Nothing is uploaded anywhere.");
        col.addView(body);

        mPickFolder = button("Import game folder…", v -> pickFolder());
        mPickZip = button("Import eZeus data pack (.zip)…", v -> pickZip());
        mCancelButton = button("Cancel import", v -> mCancel.set(true));
        mCancelButton.setVisibility(View.GONE);
        mStart = button("Start game", v -> launchGame());
        col.addView(mPickFolder);
        col.addView(mPickZip);

        mProgress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        mProgress.setMax(1000);
        mProgress.setVisibility(View.GONE);
        col.addView(mProgress, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(24)));
        mProgressText = new TextView(this);
        mProgressText.setTextColor(Color.rgb(0xCC, 0xCC, 0xCC));
        col.addView(mProgressText);
        col.addView(mCancelButton);

        TextView st = new TextView(this);
        st.setText("Data status");
        st.setTextColor(Color.rgb(0xF2, 0xE6, 0xC8));
        st.setTypeface(Typeface.DEFAULT_BOLD);
        st.setPadding(0, dp(16), 0, dp(4));
        col.addView(st);
        mStatusList = new LinearLayout(this);
        mStatusList.setOrientation(LinearLayout.VERTICAL);
        col.addView(mStatusList);
        col.addView(mStart);

        setContentView(scroll);
    }

    private void refreshStatus() {
        List<ImportRules.Item> items = ImportRules.validate(root());
        mStatusList.removeAllViews();
        for (ImportRules.Item i : items) {
            TextView t = new TextView(this);
            String mark = i.ok ? "✔ " : (i.required ? "✖ " : "○ ");
            t.setText(mark + i.label + (i.required ? "" : "  (optional)"));
            t.setTextColor(i.ok ? Color.rgb(0x9C, 0xE0, 0x9C)
                : (i.required ? Color.rgb(0xF0, 0x8A, 0x80) : Color.rgb(0xAA, 0xAA, 0xAA)));
            t.setPadding(0, dp(2), 0, dp(2));
            mStatusList.addView(t);
        }
        boolean ok = ImportRules.allRequiredOk(items);
        mStart.setEnabled(ok && !mBusy);
        mStart.setText(ok ? "Start game" : "Start game (missing required files)");
        mPickFolder.setEnabled(!mBusy);
        mPickZip.setEnabled(!mBusy);
        mCancelButton.setVisibility(mBusy ? View.VISIBLE : View.GONE);
        mProgress.setVisibility(mBusy ? View.VISIBLE : View.GONE);
    }

    private void pickFolder() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        startActivityForResult(i, REQ_GAME_FOLDER);
    }

    private void pickZip() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");
        i.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {
            "application/zip", "application/x-zip-compressed", "application/octet-stream"});
        i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
        i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        startActivityForResult(i, REQ_EZEUS_ARCHIVE);
    }

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        super.onActivityResult(req, res, data);
        if (res != RESULT_OK || data == null) return;
        if (req == REQ_GAME_FOLDER && data.getData() != null) {
            startWork(() -> importTree(data.getData()));
        } else if (req == REQ_EZEUS_ARCHIVE) {
            final List<Uri> uris = new ArrayList<>();
            if (data.getClipData() != null) {
                for (int i = 0; i < data.getClipData().getItemCount(); i++) {
                    uris.add(data.getClipData().getItemAt(i).getUri());
                }
            } else if (data.getData() != null) {
                uris.add(data.getData());
            }
            startWork(() -> {
                for (Uri u : uris) {
                    if (mCancel.get()) break;
                    importZip(u);
                }
            });
        }
    }

    private interface Work { void run() throws IOException; }

    private void startWork(Work w) {
        if (mBusy) return;
        mBusy = true;
        mCancel.set(false);
        refreshStatus();
        new Thread(() -> {
            String msg;
            try {
                w.run();
                msg = mCancel.get() ? "Import cancelled. Files copied so far are kept; run it again to resume."
                                    : "Import finished.";
            } catch (Exception e) {
                Log.e(TAG, "import failed", e);
                msg = "Import failed: " + e.getMessage()
                    + "\nNothing was deleted. Check free space and that you picked the right folder, then try again.";
            }
            final String m = msg;
            mUi.post(() -> {
                mBusy = false;
                mProgressText.setText(m);
                refreshStatus();
            });
        }, "zeus-import").start();
    }

    private void progress(final long done, final long total, final String what) {
        mUi.post(() -> {
            mProgress.setProgress(total > 0 ? (int) (done * 1000 / total) : 0);
            mProgressText.setText(String.format(Locale.ROOT, "%s\n%.1f / %.1f MB",
                what, done / 1048576.0, total / 1048576.0));
        });
    }

    // ------------------------------------------------------------ tree import

    private static final class Entry {
        final Uri uri; final String rel; final long size; final long mtime;
        Entry(Uri u, String r, long s, long m) { uri = u; rel = r; size = s; mtime = m; }
    }

    private List<Entry> listTree(Uri tree) {
        ContentResolver cr = getContentResolver();
        List<Entry> out = new ArrayList<>();
        ArrayDeque<String[]> stack = new ArrayDeque<>();
        stack.push(new String[] {DocumentsContract.getTreeDocumentId(tree), ""});
        String[] cols = {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE,
            DocumentsContract.Document.COLUMN_SIZE,
            DocumentsContract.Document.COLUMN_LAST_MODIFIED};
        while (!stack.isEmpty() && !mCancel.get()) {
            String[] cur = stack.pop();
            Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, cur[0]);
            try (Cursor c = cr.query(children, cols, null, null, null)) {
                if (c == null) continue;
                while (c.moveToNext()) {
                    String id = c.getString(0);
                    String name = c.getString(1);
                    String mime = c.getString(2);
                    long size = c.isNull(3) ? 0 : c.getLong(3);
                    long mtime = c.isNull(4) ? 0 : c.getLong(4);
                    String rel = cur[1].isEmpty() ? name : cur[1] + "/" + name;
                    if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)) {
                        stack.push(new String[] {id, rel});
                    } else {
                        out.add(new Entry(DocumentsContract.buildDocumentUriUsingTree(tree, id),
                            rel, size, mtime));
                    }
                }
            }
        }
        return out;
    }

    private void importTree(Uri tree) throws IOException {
        progress(0, 0, "Scanning selected folder…");
        List<Entry> all = listTree(tree);
        // The user may have picked the parent of the install folder; detect a
        // single top-level folder containing DATA/ and strip it.
        String prefix = detectGameRootPrefix(all);
        List<Entry> todo = new ArrayList<>();
        List<String> dests = new ArrayList<>();
        long total = 0;
        for (Entry e : all) {
            String rel = e.rel;
            if (!prefix.isEmpty()) {
                if (!rel.startsWith(prefix)) continue;
                rel = rel.substring(prefix.length());
            }
            String dest = ImportRules.mapGameFolderFile(rel);
            if (dest == null) continue;
            if (e.size > ImportRules.MAX_FILE_BYTES) throw new IOException("file too large: " + rel);
            todo.add(e);
            dests.add(dest);
            total += e.size;
        }
        if (todo.isEmpty()) {
            throw new IOException("no Zeus files found (expected a DATA folder). Pick the \"Zeus and Poseidon\" folder.");
        }
        File root = root();
        if (!root.isDirectory() && !root.mkdirs()) throw new IOException("cannot create " + root);
        checkSpace(root, total);
        long done = 0;
        byte[] buf = new byte[1 << 16];
        for (int i = 0; i < todo.size() && !mCancel.get(); i++) {
            Entry e = todo.get(i);
            File dst = ImportRules.safeResolve(root, dests.get(i));
            if (dst.isFile() && dst.length() == e.size) {
                done += e.size;                 // unchanged: skip
                continue;
            }
            progress(done, total, "Copying " + dests.get(i));
            try (InputStream in = getContentResolver().openInputStream(e.uri)) {
                if (in == null) throw new IOException("cannot open " + e.rel);
                copyAtomic(in, dst, buf, e.size);
            }
            done += e.size;
        }
        progress(done, total, "Copied " + todo.size() + " files");
    }

    static String detectGameRootPrefix(List<Entry> all) {
        for (Entry e : all) {
            if (e.rel.regionMatches(true, 0, "DATA/", 0, 5)) return "";
        }
        for (Entry e : all) {
            int i = e.rel.indexOf('/');
            if (i > 0 && e.rel.regionMatches(true, i + 1, "DATA/", 0, 5)) {
                return e.rel.substring(0, i + 1);
            }
        }
        return "";
    }

    // ------------------------------------------------------------- zip import

    private void importZip(Uri zip) throws IOException {
        File root = root();
        if (!root.isDirectory() && !root.mkdirs()) throw new IOException("cannot create " + root);
        long total = querySize(zip);
        checkSpace(root, total > 0 ? total * 2 : 0);
        long done = 0;
        long expanded = 0;
        int files = 0;
        byte[] buf = new byte[1 << 16];
        try (InputStream raw = getContentResolver().openInputStream(zip);
             ZipInputStream zin = new ZipInputStream(raw)) {
            ZipEntry ze;
            while ((ze = zin.getNextEntry()) != null && !mCancel.get()) {
                if (ze.isDirectory()) continue;
                String dest = ImportRules.mapEZeusArchiveEntry(ze.getName());
                if (dest == null) continue;
                File dst = ImportRules.safeResolve(root, dest);
                long declared = ze.getSize();
                if (declared > ImportRules.MAX_FILE_BYTES) throw new IOException("entry too large: " + ze.getName());
                progress(done, total, "Extracting " + dest);
                long n = copyAtomic(zin, dst, buf, declared);
                expanded += n;
                if (expanded > ImportRules.MAX_ARCHIVE_BYTES) throw new IOException("archive expands too much");
                done += ze.getCompressedSize() > 0 ? ze.getCompressedSize() : n;
                files++;
            }
        }
        if (files == 0) throw new IOException("the archive did not contain eZeus data (expected eZeus-*/interface.e)");
        progress(total, total, "Extracted " + files + " files");
    }

    private long querySize(Uri u) {
        try (Cursor c = getContentResolver().query(u,
                new String[] {android.provider.OpenableColumns.SIZE}, null, null, null)) {
            if (c != null && c.moveToFirst() && !c.isNull(0)) return c.getLong(0);
        } catch (Exception ignored) { }
        return 0;
    }

    private void checkSpace(File dir, long need) throws IOException {
        if (need <= 0) return;
        long avail = new StatFs(dir.getAbsolutePath()).getAvailableBytes();
        if (avail < need + SPACE_MARGIN) {
            throw new IOException(String.format(Locale.ROOT,
                "not enough free space: need %.0f MB, have %.0f MB", need / 1048576.0, avail / 1048576.0));
        }
    }

    /** Writes to "name.part" then renames, so a crash never leaves a truncated file in place. */
    private long copyAtomic(InputStream in, File dst, byte[] buf, long expected) throws IOException {
        File dir = dst.getParentFile();
        if (dir != null && !dir.isDirectory() && !dir.mkdirs()) throw new IOException("cannot create " + dir);
        File tmp = new File(dst.getPath() + ".part");
        long n = 0;
        try (OutputStream out = new FileOutputStream(tmp)) {
            int r;
            while ((r = in.read(buf)) > 0) {
                if (mCancel.get()) throw new IOException("cancelled");
                out.write(buf, 0, r);
                n += r;
                if (n > ImportRules.MAX_FILE_BYTES) throw new IOException("file too large: " + dst.getName());
            }
        } catch (IOException e) {
            //noinspection ResultOfMethodCallIgnored
            tmp.delete();
            if (mCancel.get()) return n;
            throw e;
        }
        if (expected > 0 && n != expected) {
            //noinspection ResultOfMethodCallIgnored
            tmp.delete();
            throw new IOException("size mismatch for " + dst.getName() + " (" + n + " != " + expected + ")");
        }
        if (dst.exists() && !dst.delete()) throw new IOException("cannot replace " + dst);
        if (!tmp.renameTo(dst)) throw new IOException("cannot finalize " + dst);
        return n;
    }

    private void launchGame() {
        Intent i = new Intent(this, ZeusActivity.class);
        startActivity(i);
        finish();
    }

    @Override
    public void onBackPressed() {
        if (mBusy) {
            mCancel.set(true);
            return;
        }
        super.onBackPressed();
    }
}
