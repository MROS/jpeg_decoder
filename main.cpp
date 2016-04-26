#include <stdio.h>
#include <stdlib.h>

const int SOI_MARKER = 0xD8;
const int APP0_MARKER = 0xE0;
const int DQT_MARKER = 0xDB;
const int SOF_MARKER = 0xC0;
const int DHT_MARKER = 0xC4;
const int SOS_MARKER = 0xDA;
const int EOI_MARKER = 0xD9;

void showSectionName(const char *s) {
    printf("************************ %s **************************\n", s);
    return;
}

unsigned int readSectionLength(FILE *f) {
    unsigned char c;
    unsigned int length;
    fread(&c, 1, 1, f);
    length = c;
    fread(&c, 1, 1, f);
    length = length * 16 + c;
    return length;
}

unsigned int EnterNewSection(FILE *f, const char *s) {
    showSectionName(s);
    unsigned int len = readSectionLength(f);
    printf("本區段長度為 %d\n", len);
    return len;
}

void readAPP(FILE *f) {
    unsigned int len = EnterNewSection(f, "APP0");
    char m[5];
    fread(m, 1, 5, f);
    printf("使用 %s\n", m);
    unsigned char v[2];
    fread(v, 1, 2, f);
    printf("版本 %d.%d\n", v[0], v[1]);
    fseek(f, 1, SEEK_CUR);
    fread(v, 1, 2, f);
    printf("x方向像素密度：%d\n", v[0] * 16 + v[1]);
    fread(v, 1, 2, f);
    printf("y方向像素密度：%d\n", v[0] * 16 + v[1]);
    fseek(f, len - 14, SEEK_CUR);
}

void readDQT(FILE *f) {
    unsigned int len = EnterNewSection(f, "DQT");
    fseek(f, len - 2, SEEK_CUR);
}
void readSOF(FILE *f) {
    unsigned int len = EnterNewSection(f, "SOF");
    fseek(f, len - 2, SEEK_CUR);
}
void readDHT(FILE *f) {
    unsigned int len = EnterNewSection(f, "DHT");
    fseek(f, len - 2, SEEK_CUR);
}
void readSOS(FILE *f) {
    unsigned int len = EnterNewSection(f, "SOS");
    fseek(f, len - 2, SEEK_CUR);
}

void readHeader(FILE *f) {
    unsigned char c;
    fread(&c, 1, 1, f);
    while (c == 0xFF) {
        fread(&c, 1, 1, f);
        switch (c) {
            case SOI_MARKER:
                printf("Start of Image\n");
                break;
            case APP0_MARKER:
                readAPP(f);
                break;
            case DQT_MARKER:
                readDQT(f);
                break;
            case SOF_MARKER:
                readSOF(f);
                break;
            case DHT_MARKER:
                readDHT(f);
                break;
            case SOS_MARKER:
                readSOS(f);
                break;
            case EOI_MARKER:
                break;
        }
        fread(&c, 1, 1, f);
    }
    if (fread(&c, 1, 1, f) != 0) {
        fprintf(stderr, "沒有吃完就結束\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法：jpeg_decoder <jpeg file>\n");
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        fprintf(stderr, "檔案開啟失敗\n");
    }
    readHeader(f);
    return 0;
}