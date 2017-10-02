#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// MIDI file format into from:
// http://valentin.dasdeck.com/midi/midifile.htm

enum chunk_type_id { MTHD, MTRK, UNKNOWN };
static char *s_type_id_names[] = {"MThd", "MTrk", "Unknown"};

typedef struct chunk_info
{
    unsigned int type;
    size_t len;
} chunk_info;

void pack_int(char *from, char *dest, int len)
{
    for (int i = 0; i < len; i++)
        dest[i] = from[len - 1 - i];
}

chunk_info parse_chunk_header(char chunk_header[8])
{
    char chunk_type[5] = {0};
    strncpy(chunk_type, chunk_header, 4);

    int chunk_len = 0;
    char *packed_len = (char*)&chunk_len;
    for (int i = 0; i < 4; i++)
        packed_len[i] = chunk_header[7-i];

    chunk_info info;

    if (strcmp("MThd", chunk_type) == 0)
        info.type = MTHD;
    else if (strcmp("MTrk", chunk_type) == 0)
        info.type = MTRK;
    else
        info.type = UNKNOWN;

    info.len = chunk_len;

    return info;
}


void parse_midi(char *midifile)
{
    printf("Yar! Opening %s\n", midifile);
    FILE *fp = fopen(midifile, "rb");
    if (fp == NULL)
    {
        printf("Couldnae open yer file\n");
        return;
    }

    char chunk_header[8] = {0};
    size_t numread;

    numread = fread(chunk_header, 1 /* byte */, 8 /* num x */, fp);
    if (numread != 8) { printf("Didn't read what i expected\n"); }
    chunk_info cur_chunk = parse_chunk_header(chunk_header);

    if (cur_chunk.type == UNKNOWN)
        return;

    int format_type = 0;
    int num_mtrk_chunks = 0;
    int ppqn = 0;
    char scratch[2] = {0};
    char *packed_scratch;

    if (cur_chunk.type == MTHD)
    {
        // format type first
        numread = fread(scratch, 1, 2, fp);
        packed_scratch = (char*)&format_type;
        pack_int(scratch, packed_scratch, 2);

        // num of MTrk chunks
        numread = fread(scratch, 1, 2, fp);
        packed_scratch = (char*)&num_mtrk_chunks;
        pack_int(scratch, packed_scratch, 2);

        // Pulses Per Quart Note
        numread = fread(scratch, 1, 2, fp);
        packed_scratch = (char*)&ppqn;
        pack_int(scratch, packed_scratch, 2);
    }

    
    printf("Type: %s len:%lu\n", s_type_id_names[cur_chunk.type], cur_chunk.len);
    printf("Format Type:%d Num MTrk Chunks:%d PPQN:%d\n", format_type, num_mtrk_chunks, ppqn);

}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Gimme a file name, ya doofus\n");
        return -1;
    }
    parse_midi(argv[1]);

}

