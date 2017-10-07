#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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
    int value = 0;
    unsigned char cur_byte;

    cur_byte = fgetc(fp);
    (*chunk_left)--;

    if (!(cur_byte & 0x80))
        return cur_byte & 0x7F;

    value = cur_byte & 0x7F;
    do
    {
        cur_byte = fgetc(fp);
        (*chunk_left)--;
        value = (value << 7) + (cur_byte & 0x7F);
    } while ( cur_byte & 0x80);

    return value;
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

    double multiplier = 960. / h_info.ppqn;
    char chunk_header[8] = {0};
    chunk_info cur_chunk;

    for (int i = 0; i < h_info.num_mtrk_chunks; i++)
    {
        printf("\nCHUNK%d\n", i);
        size_t numread = fread(chunk_header, 1 /* byte */, 8 /* num x */, fp); // 4 bytes for type, 4 for size
        if (numread != 8) { printf("Didn't read what i expected\n"); }
        cur_chunk = parse_chunk_header(chunk_header);

        if (cur_chunk.type != MTRK)
        {
            printf("Skipping %lu bytes as chunk type is unknown\n", cur_chunk.len);
            fseek(fp, cur_chunk.len, SEEK_CUR);
        }
        printf("ChunkType: %s len:%lu\n\n", s_type_id_names[cur_chunk.type], cur_chunk.len);

        int delta_time = 0;
        int chunk_left = cur_chunk.len;
        unsigned char status_running = 0;
        unsigned char cur_status = 0;
        unsigned char cur_note = 0;
        unsigned char cur_velocity = 0;
        bool is_note_event;
        while (chunk_left > 0)
        {
            is_note_event = false;

            // 1. get delta-time
            delta_time += read_var_len_variable(fp, &chunk_left);

            unsigned char cur_byte = 0;
            cur_byte = fgetc(fp);
            chunk_left--;

            if (cur_byte == 0xff) // Meta Event
            {
                // Meta-event type
                numread = fread(&cur_byte, 1, 1, fp);
                chunk_left -= numread;

                int len = read_var_len_variable(fp, &chunk_left);
                chunk_left -= len;
                fseek(fp, len, SEEK_CUR);

                if (cur_byte == 0x2f)
                {
                    printf("META EVENT! END OF TRACK\n");
                }
                else
                    printf("META event! Type:%d %x - (len: %d) skipping!\n", cur_byte, cur_byte, len);

                // reset status_running;
                status_running = 0;

            }
            else if ((cur_byte == 0xf0) || // sysex event
                     (cur_byte == 0xf7))
            {
                int len = read_var_len_variable(fp, &chunk_left);
                printf("SYSEX EVENT %d %x (len:%d) - skipping!\n", cur_byte, cur_byte, len);
                chunk_left -= len;
                fseek(fp, len, SEEK_CUR);
                // reset status_running;
                status_running = 0;
            }
            else 
            {
                unsigned char left_nib = (cur_byte | 0x0f) - 0x0f;
                unsigned char right_nib = (cur_byte | 0xf0) - 0xf0;

                if (left_nib == 0x80)
                {
                    unsigned char key;
                    unsigned char velocity;
                    numread = fread(&key, 1, 1, fp);
                    chunk_left -= numread;
                    numread = fread(&velocity, 1, 1, fp);
                    chunk_left -= numread;

                    cur_note = key;
                    cur_velocity = velocity;
                    cur_status = 128;
                    is_note_event = true;
                    status_running = cur_byte;
                }
                else if (left_nib == 0x90)
                {
                    unsigned char key;
                    unsigned char velocity;
                    numread = fread(&key, 1, 1, fp);
                    chunk_left -= numread;
                    numread = fread(&velocity, 1, 1, fp);
                    chunk_left -= numread;

                    cur_note = key;
                    cur_velocity = velocity;
                    cur_status = 144;
                    is_note_event = true;
                    status_running = cur_byte;
                }
                else if (left_nib == 0xa0)
                {
                    unsigned char key;
                    unsigned char val; // pressure
                    numread = fread(&key, 1, 1, fp);
                    chunk_left -= numread;
                    numread = fread(&val, 1, 1, fp);
                    chunk_left -= numread;
                    printf("MIDI Polyphonic Key PRESS on Channel:%d Key:%d Velocity:%d\n", right_nib, key, val);
                    status_running = cur_byte;
                }
                else if (left_nib == 0xb0)
                {
                    unsigned char controller_number;
                    unsigned char val;
                    numread = fread(&controller_number, 1, 1, fp);
                    chunk_left -= numread;
                    numread = fread(&val, 1, 1, fp);
                    chunk_left -= numread;
                    printf("MIDI Control Change! Channel:%d controller_number:%d Velocity:%d\n", right_nib, controller_number, val);
                    status_running = cur_byte;
                }
                else if (left_nib == 0xc0)
                {
                    numread = fread(&cur_byte, 1, 1, fp);
                    chunk_left -= numread;
                    status_running = cur_byte;
                    printf("MIDI Program Change: Channel :%d\n", right_nib);
                }
                else if (left_nib == 0xd0)
                {
                    numread = fread(&cur_byte, 1, 1, fp);
                    chunk_left -= numread;
                    status_running = cur_byte;
                    printf("MIDI Channel PRessur:%d\n", right_nib);
                }
                else if (left_nib == 0xe0)
                {
                    unsigned char controller_number;
                    unsigned char val;
                    numread = fread(&controller_number, 1, 1, fp);
                    chunk_left -= numread;
                    numread = fread(&val, 1, 1, fp);
                    chunk_left -= numread;
                    printf("MIDI Pitch Wheel Change :%d controller_number:%d Velocity:%d\n", right_nib, controller_number, val);
                    status_running = cur_byte;
                }
                else
                {
                    // no status byte seen, use STATUS RUNNING
                    left_nib = (status_running | 0x0f) - 0x0f;
                    right_nib = (status_running | 0xf0) - 0xf0;

                    if (left_nib == 0x80)
                    {
                        unsigned char key = cur_byte;
                        unsigned char velocity;
                        numread = fread(&velocity, 1, 1, fp);
                        chunk_left -= numread;
                        // printf("MIDI NOTE OFF on Channel:%d Key:%d Velocity:%d\n", right_nib, key, velocity);
                        cur_note = key;
                        cur_velocity = velocity;
                        cur_status = 128;
                        is_note_event = true;

                    }
                    else if (left_nib == 0x90)
                    {
                        unsigned char key = cur_byte;
                        unsigned char velocity;
                        numread = fread(&velocity, 1, 1, fp);
                        chunk_left -= numread;


                        cur_note = key;
                        cur_velocity = velocity;
                        is_note_event = true;

                        if (velocity == 0)
                            cur_status = 128;
                        else
                            cur_status = 144;

                    }
                    else if (left_nib == 0xa0)
                    {
                        unsigned char key = cur_byte;
                        unsigned char val; // pressure
                        numread = fread(&val, 1, 1, fp);
                        chunk_left -= numread;
                        printf("MID Polyphonic Key PRESS on Channel:%d Key:%d Velocity:%d\n", right_nib, key, val);
                    }
                    else if (left_nib == 0xb0)
                    {
                        unsigned char controller_number = cur_byte;
                        unsigned char val;
                        numread = fread(&val, 1, 1, fp);
                        chunk_left -= numread;
                        printf("MIDI Control Change! Channel:%d controller_number:%d Velocity:%d\n", right_nib, controller_number, val);
                    }
                    else if (left_nib == 0xc0)
                    {
                        printf("MIDI Program Change: Channel :%d\n", right_nib);
                    }
                    else if (left_nib == 0xd0)
                    {
                        printf("MIDI Channel PRessur:%d\n", right_nib);
                    }
                    else if (left_nib == 0xe0)
                    {
                        unsigned char controller_number = cur_byte;
                        unsigned char val;
                        numread = fread(&val, 1, 1, fp);
                        chunk_left -= numread;
                        printf("MIDI Pitch Wheel Change :%d controller_number:%d Velocity:%d\n", right_nib, controller_number, val);
                    }
                }
            }
            if (is_note_event)
            {
                double delta = delta_time * multiplier;
                int scaled_delta = (delta - floor(delta) > 0.5) ? ceil(delta) : floor(delta);
                printf("%d::%d::%d::%d\n", scaled_delta, cur_status, cur_note, cur_velocity);
            }
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

