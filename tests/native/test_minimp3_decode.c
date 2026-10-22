// Decodes MP3 files with the exact minimp3 sources SDL_mixer 2.8.2 uses on
// Android, to check the user's music decodes on ARM64. Prints frames/rate/ms.
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    int bad = 0;
    for (int a = 1; a < argc; a++) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { printf("cannot open %s\n", argv[a]); bad++; continue; }
        fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
        unsigned char* buf = malloc(n);
        if (fread(buf, 1, n, f) != (size_t)n) { bad++; fclose(f); continue; }
        fclose(f);
        mp3dec_t dec; mp3dec_init(&dec);
        mp3dec_frame_info_t info; short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        long pos = 0, samples = 0; int frames = 0, hz = 0, ch = 0;
        while (pos < n) {
            int s = mp3dec_decode_frame(&dec, buf + pos, (int)(n - pos), pcm, &info);
            if (info.frame_bytes == 0) break;
            pos += info.frame_bytes;
            if (s > 0) { frames++; samples += s; hz = info.hz; ch = info.channels; }
        }
        double ms = hz ? 1000.0 * samples / hz : 0;
        printf("%s: frames=%d rate=%d ch=%d duration=%.1fs\n", argv[a], frames, hz, ch, ms / 1000);
        if (frames == 0) bad++;
        free(buf);
    }
    return bad ? 1 : 0;
}
