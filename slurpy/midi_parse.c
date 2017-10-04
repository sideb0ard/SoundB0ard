#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// MIDI file format into from:
// http://valentin.dasdeck.com/midi/midifile.htm
// https://www.csie.ntu.edu.tw/~r92092/ref/midi/

enum chunk_type_id { MTHD, MTRK, UNKNOWN };
static char *s_type_id_names[] = {"MThd", "MTrk", "Unknown"};

typedef struct chunk_info
{
    unsigned int type;
    size_t len;
} chunk_info;

void print_binary_string(unsigned char byte)
{
    printf("BYTE: %d\n", byte);
    byte & 128 ? printf("1") : printf("0");
    byte & 64 ? printf("1") : printf("0");
    byte & 32 ? printf("1") : printf("0");
    byte & 16 ? printf("1") : printf("0");
    byte & 8 ? printf("1") : printf("0");
    byte & 4 ? printf("1") : printf("0");
    byte & 2 ? printf("1") : printf("0");
    byte & 1 ? printf("1") : printf("0");
    printf("\n");
}

typedef struct header_info
{
    int format_type;
    int num_mtrk_chunks;
    int ppqn;
} header_info;

int read_var_len_variable(FILE *fp, int *chunk_left)
{
    //int value = 0;
    char cur_byte = fgetc(fp);
    //value = cur_byte & 0x7f;
    printf("ENTRY BYTE IS %d\n", cur_byte);
    print_binary_string(cur_byte);
    do
    {
        //value = (value << 7) + ((cur_byte = fgetc(fp)) & 0x7F);
        cur_byte = fgetc(fp);
        (*chunk_left)--;
        printf("ONGOING BYTE IS %d\n", cur_byte);
        print_binary_string(cur_byte);
    } while ( cur_byte & 0x80);

    return 1;
}

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

header_info parse_header_chunk(FILE *fp)
{
    header_info h_info = {0};
    char scratch[2] = {0};
    char *packed_scratch;

    // format type first
    fread(scratch, 1, 2, fp);
    packed_scratch = (char*)&h_info.format_type;
    pack_int(scratch, packed_scratch, 2);

    // num of MTrk chunks
    fread(scratch, 1, 2, fp);
    packed_scratch = (char*)&h_info.num_mtrk_chunks;
    pack_int(scratch, packed_scratch, 2);

    // Pulses Per Quart Note
    fread(scratch, 1, 2, fp);
    packed_scratch = (char*)&h_info.ppqn;
    pack_int(scratch, packed_scratch, 2);
    if (h_info.ppqn & 32768)
    {
        printf("Whoa, this file doesn't use PPQN - its' on some SMTPE frame shit - lazy Thor hasn't implemented this - bailing!\n");
        exit(-1);
    }

    return h_info;
}

bool is_midi_file(FILE *fp)
{
    char chunk_header[8] = {0};
    int numread = fread(chunk_header, 1 /* byte */, 8 /* num x */, fp);
    if (numread != 8) { printf("Didn't read what i expected\n"); return false;}

    chunk_info cur_chunk = parse_chunk_header(chunk_header);
    if (cur_chunk.type == MTHD)
        return true;

    return false;
}

bool parse_midi_file(char *midifile)
{
    printf("Yar! Opening %s\n", midifile);

    FILE *fp = fopen(midifile, "rb");
    if (fp == NULL)
    {
        printf("Couldnae open yer file\n");
        return false;
    }

    if (!is_midi_file(fp))
    {
        printf("Didn't find a MIDI header chunk - bailin'!\n");
        return false;
    }
    header_info h_info = parse_header_chunk(fp);
    printf("Format Type:%d Num MTrk Chunks:%d PPQN:%d\n", h_info.format_type, h_info.num_mtrk_chunks, h_info.ppqn);

    size_t numread;
    char chunk_header[8] = {0};
    chunk_info cur_chunk;
    int delta_time = 0;

    //for (int i = 0; i < h_info.num_mtrk_chunks; i++)
    for (int i = 0; i < 1; i++)
    {
        numread = fread(chunk_header, 1 /* byte */, 8 /* num x */, fp); // 4 bytes for type, 4 for size
        if (numread != 8) { printf("Didn't read what i expected\n"); }
        cur_chunk = parse_chunk_header(chunk_header);

        if (cur_chunk.type != MTRK)
        {
            printf("Skipping %lu bytes as chunk type is unknown\n", cur_chunk.len);
            fseek(fp, cur_chunk.len, SEEK_CUR);
        }
        printf("Type: %s len:%lu\n", s_type_id_names[cur_chunk.type], cur_chunk.len);

        int chunk_left = cur_chunk.len;
        while (chunk_left > 0)
        {
            unsigned char cur_byte = 0;
            numread = fread(&cur_byte, 1, 1, fp);
            chunk_left -= numread;

            if (cur_byte & 255) // Meta Event
            {
                //numread = fread(&cur_byte, 1, 1, fp);
                //chunk_left -= numread;
                printf("Ooh! META event! Type:%d %x\n", cur_byte, cur_byte);
                //int len = read_var_len_variable(fp, &chunk_left);
                //printf("Len is %d\n", len);
            }
            else if (cur_byte & 240) // sysex event
            {
                printf("Ooh! Sysex event %d %x!\n", cur_byte, cur_byte);
            }
            else
            {
                printf("Normy:%d %x!\n",cur_byte, cur_byte);
            }
            print_binary_string(cur_byte);
        }

        //fseek(fp, cur_chunk.len, SEEK_CUR);

        //if (i != (num_mtrk_chunks-1))
        //    fseek(fp, cur_chunk.len, SEEK_CUR);
    }

    return true;

}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Gimme a file name, ya doofus\n");
        return -1;
    }

    if (!parse_midi_file(argv[1]))
        return -1;

}

