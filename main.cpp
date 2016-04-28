#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <map>


const int SOI_MARKER = 0xD8;
const int APP0_MARKER = 0xE0;
const int DQT_MARKER = 0xDB;
const int SOF_MARKER = 0xC0;
const int DHT_MARKER = 0xC4;
const int SOS_MARKER = 0xDA;
const int EOI_MARKER = 0xD9;

unsigned char quantTable[4][128];

const int DC = 0;
const int AC = 1;
std::map<std::pair<unsigned char, unsigned int>, unsigned char> huffTable[2][2];


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
    unsigned char c;
    fread(&c, 1, 1, f);
    unsigned precision = c >> 4 == 0 ? 8 : 16;
    printf("精度：%d\n", precision);
    unsigned char id = c & 0x0F;
    printf("量化表ID: %d\n", id);
    fread(quantTable[id], (precision / 8), 64, f);
    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) {
            printf("\n");
        }
        printf("%2d ", quantTable[id][i]);
    }
    printf("\n");
}
void readSOF(FILE *f) {
    unsigned int len = EnterNewSection(f, "SOF");
    fseek(f, 1, SEEK_CUR); // 精度
    unsigned int height, width;
    unsigned char v[3];
    fread(v, 1, 2, f);
    // TODO: 高度跟寬度不確定
    height = v[0] * 256 + v[1];
    fread(v, 1, 2, f);
    width = v[0] * 256 + v[1];
    printf("長*寬: %d*%d\n", height, width);
    fseek(f, 1, SEEK_CUR); // 顏色分量數，固定為3
    for (int i = 0; i < 3; i++) {
        fread(v, 1, 3, f);
        printf("顏色分量ID：%d\n", v[0]);
        printf("水平採樣因子：%d\n", v[1] >> 4);
        printf("垂直採樣因子：%d\n", v[1] & 0x0F);
        printf("量化表ID：%d\n", v[2]);
    }
}
std::pair<unsigned char, unsigned int>* createHuffCode(unsigned char *a, unsigned int number) {
    int si = sizeof(std::pair<unsigned char, unsigned int>);
    auto *ret = (std::pair<unsigned char, unsigned int>*)malloc(si * number);
    int code = 0;
    int count = 0;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < a[i]; j++) {
            ret[count++] = std::make_pair(i + 1, code);
            code += 1;
        }
        code = code << 1;
    }
    return ret;
}
void readDHT(FILE *f) {
    unsigned int len = EnterNewSection(f, "DHT");
    unsigned char v[1];
    fread(v, 1, 1, f);
    unsigned char DCorAC = v[0] >> 4;
    printf(DCorAC == 0 ? "DC\n" : "AC\n");
    unsigned char id = v[0] & 0x0F;
    printf("ID: %d\n", id);

    unsigned char a[16];
    fread(a, 1, 16, f);
    unsigned int number = 0;
    for (int i = 0; i < 16; i++) {
        printf("%d ", a[i]);
        number += a[i];
    }
    printf("\n");
    auto *huffCode = createHuffCode(a, number);
    for (int i = 0; i < number; i++) {
        unsigned char v;
        fread(&v, 1, 1, f);
        huffTable[DCorAC][id][huffCode[i]] = v;
        printf("%d %d: %d\n", huffCode[i].first, huffCode[i].second, v);
    }
    free(huffCode);
}
void readSOS(FILE *f) {
    unsigned int len = EnterNewSection(f, "SOS");

    fseek(f, 1, SEEK_CUR);   // 顏色分量數，固定為3
    for (int i = 0; i < 3; i++) {
        unsigned char v[1];
        fread(v, 1, 1, f);
        printf("顏色分量id：%d\n", v[0]);
        fread(v, 1, 1, f);
        printf("DC霍夫曼id：%d\n", v[0] >> 4);
        printf("AC霍夫曼id：%d\n", v[0] & 0x0F);
    }
    fseek(f, 3, SEEK_CUR);
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