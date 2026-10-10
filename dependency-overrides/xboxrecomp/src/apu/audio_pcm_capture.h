#ifndef UCRR_AUDIO_PCM_CAPTURE_H
#define UCRR_AUDIO_PCM_CAPTURE_H
/* Diagnostic-only accepted-XAudio2 PCM, NOT device loopback. Audio owner only.
 * Exclusive creation; bounded 10 minutes; buffered synchronous I/O can perturb
 * timing. Never use a capture run alone for performance acceptance. Complete
 * records survive abrupt exit; decoder must disclose a truncated final record. */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
static FILE *audio_pcm_file;
static uint64_t audio_pcm_frames;
static unsigned audio_pcm_records;
static char audio_pcm_buffer[65536];
static void audio_pcm_capture_close(void) {
    if (!audio_pcm_file) return;
    int failed = fclose(audio_pcm_file);
    audio_pcm_file = NULL;
    fprintf(stderr,"[AUDIO-PCM] closed frames=%llu records=%u close_error=%d\n",
            (unsigned long long)audio_pcm_frames,audio_pcm_records,failed!=0);
}
static void audio_pcm_capture_init(void) {
    const char *path=getenv("UCRR_AUDIO_PCM_CAPTURE");
    int fd=-1;
    if(audio_pcm_file || !path || !*path)return;
    audio_pcm_frames=0;audio_pcm_records=0;
    if(_sopen_s(&fd,path,_O_WRONLY|_O_CREAT|_O_EXCL|_O_BINARY,_SH_DENYWR,
                _S_IREAD|_S_IWRITE)!=0){
        fprintf(stderr,"[AUDIO-PCM] create failed (existing files never overwritten)\n");return;
    }
    audio_pcm_file=_fdopen(fd,"wb");
    if(!audio_pcm_file){_close(fd);return;}
    setvbuf(audio_pcm_file,audio_pcm_buffer,_IOFBF,sizeof(audio_pcm_buffer));
    {const char magic[8]={'U','C','A','P','C','M','0','1'};
     const uint32_t fmt[4]={48000,2,16,24};
     if(fwrite(magic,1,8,audio_pcm_file)!=8 || fwrite(fmt,1,16,audio_pcm_file)!=16){
         audio_pcm_capture_close();return;
     }}
    fprintf(stderr,"[AUDIO-PCM] enabled rate=48000 channels=2 cap_seconds=600 scope=accepted-source-not-loopback\n");
}
static void audio_pcm_capture_record(const int16_t *pcm,unsigned frames,
                                     uint64_t us,uint64_t played,unsigned queued) {
    uint64_t timing[2]={us,played};uint32_t fields[2]={frames,queued};
    if(!audio_pcm_file || !pcm || !frames || frames>1024)return;
    if(audio_pcm_frames+frames>48000ull*600){audio_pcm_capture_close();return;}
    if(fwrite(timing,1,16,audio_pcm_file)!=16 || fwrite(fields,1,8,audio_pcm_file)!=8 ||
       fwrite(pcm,4,frames,audio_pcm_file)!=frames){
        fprintf(stderr,"[AUDIO-PCM] write failed; capture disabled\n");
        audio_pcm_capture_close();return;
    }
    audio_pcm_frames+=frames;++audio_pcm_records;
    /* Keep at most ~one second of accepted PCM in userspace after a crash. */
    if(audio_pcm_records%188==0 && fflush(audio_pcm_file)!=0){
        fprintf(stderr,"[AUDIO-PCM] flush failed; capture disabled\n");
        audio_pcm_capture_close();
    }
}
#endif
