#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <map>
#include <math.h>
#include "qdbmp.h"


const int SOI_MARKER = 0xD8;
const int APP0_MARKER = 0xE0;
const int DQT_MARKER = 0xDB;
const int SOF_MARKER = 0xC0;
const int DHT_MARKER = 0xC4;
const int SOS_MARKER = 0xDA;
const int EOI_MARKER = 0xD9;
const int COM_MARKER = 0xFE;

struct {
    int height;
    int width;
} image;

struct {
    unsigned char id;
    unsigned char width;
    unsigned char height;
    unsigned char quant;
} subVector[4];

unsigned char maxWidth, maxHeight;

struct acCode {
    unsigned char len;
    unsigned char zeros;
    int value;
};

struct RGB {
    unsigned char R, G, B;
};

typedef double BLOCK[8][8];

int quantTable[4][128];

const int DC = 0;
const int AC = 1;
std::map<std::pair<unsigned char, unsigned int>, unsigned char> huffTable[2][2];

double cos_cache[200];

void init_cos_cache() {
    for (int i = 0; i < 200; i++) {
        cos_cache[i] = cos(i * M_PI / 16.0);
    }
}

class MCU {
public:
    BLOCK mcu[4][2][2];
    // 除錯用
    void show() {
        printf("*************** mcu show ***********************\n");
        for (int id = 1; id <= 3; id++) {
            for (int h = 0; h < subVector[id].height; h++) {
                for (int w = 0; w < subVector[id].width; w++) {
                    printf("mcu id: %d, %d %d\n", id, h, w);
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            printf("%lf ", mcu[id][h][w][i][j]);
                        }
                        printf("\n");
                    }
                }
            }
        }
    };
    void quantify() {
        for (int id = 1; id <= 3; id++) {
            for (int h = 0; h < subVector[id].height; h++) {
                for (int w = 0; w < subVector[id].width; w++) {
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            mcu[id][h][w][i][j] *= quantTable[subVector[id].quant][i*8 + j];
                        }
                    }
                }
            }
        }
    };
    void zigzag() {
        for (int id = 1; id <= 3; id++) {
            for (int h = 0; h < subVector[id].height; h++) {
                for (int w = 0; w < subVector[id].width; w++) {
                    int zz[8][8] = {
                            { 0,  1,  5,  6, 14, 15, 27, 28},
                            { 2,  4,  7, 13, 16, 26, 29, 42},
                            { 3,  8, 12, 17, 25, 30, 41, 43},
                            { 9, 11, 18, 24, 31, 40, 44, 53},
                            {10, 19, 23, 32, 39, 45, 52, 54},
                            {20, 22, 33, 38, 46, 51, 55, 60},
                            {21, 34, 37, 47, 50, 56, 59, 61},
                            {35, 36, 48, 49, 57, 58, 62, 63}
                    };
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            zz[i][j] = mcu[id][h][w][zz[i][j] / 8][zz[i][j] % 8];
                        }
                    }
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            mcu[id][h][w][i][j] = zz[i][j];
                        }
                    }
                }
            }
        }
    };
    void idct() {
        for (int id = 1; id <= 3; id++) {
            for (int h = 0; h < subVector[id].height; h++) {
                for (int w = 0; w < subVector[id].width; w++) {
                    double tmp[8][8] = {0};
//					 照定義展開，效能低下
//                     for (int i = 0; i < 8; i++) {
//                         for (int j = 0; j < 8; j++) {
//                             for (int x = 0; x < 8; x++) {
//                                 for (int y = 0; y < 8; y++) {
//                                     tmp[i][j] += (cc(x, y) * mcu[id][h][w][x][y] * cos((2*i+1)*M_PI/16.0*x) * cos((2*j+1)*M_PI/16.0*y));
//                                 }
//                             }
//                             tmp[i][j] /= 4.0;
//                         }
//                     }
					// 計算兩次一維idct去計算二維idct
                    double s[8][8] = {};
                    for (int j = 0; j < 8; j++) {
                        for (int x = 0; x < 8; x++) {
                            for (int y = 0; y < 8; y++) {
                                s[j][x] += c (y) * mcu[id][h][w][x][y] * cos_cache[(j + j + 1) * y];
                            }
                            s[j][x] = s[j][x] / 2.0;
                        }
                    }
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            for (int x = 0; x < 8; x++) {
                                tmp[i][j] += c(x) * s[j][x] * cos_cache[(i + i + 1) * x];
                            }
                            tmp[i][j] = tmp[i][j] / 2.0;
                        }
                    }
                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            mcu[id][h][w][i][j] = tmp[i][j];
                        }
                    }
                }
            }
        }
    }
    void decode() {
        this->quantify();
        this->zigzag();
        this->idct();
    }
    RGB **toRGB() {
        RGB **ret = (RGB **)malloc(sizeof(RGB **) * maxHeight * 8);
        for (int i = 0; i < maxHeight * 8; i++) {
            ret[i] = (RGB *)malloc(sizeof(RGB *) * maxWidth * 8);
        }
        for (int i = 0; i < maxHeight * 8; i++) {
            for (int j = 0; j < maxWidth * 8; j++) {
                double Y = trans(1, i, j);
                double Cb = trans(2, i, j);
                double Cr = trans(3, i, j);
                ret[i][j].R = chomp(Y + 1.402*Cr + 128);
                ret[i][j].G = chomp(Y - 0.34414*Cb - 0.71414*Cr + 128);
                ret[i][j].B = chomp(Y + 1.772*Cb + 128);
            }
        }
        return ret;
    }
private:
    double cc(int i, int j) {
        if (i == 0 && j == 0) {
            return 1.0/2.0;
        } else if (i == 0 || j == 0) {
            return 1.0/sqrt(2.0);
        } else {
            return 1.0;
        }
    }
    double c(int i) {
        static double x = 1.0/sqrt(2.0);
        if (i == 0) {
            return x;
        } else {
            return 1.0;
        }
    }
    unsigned char chomp(double x) {
        if (x > 255.0) {
            return 255;
        } else if (x < 0) {
            return 0;
        } else {
            return (unsigned char) x;
        }
    }
    double trans(int id, int h, int w) {
        int vh = h * subVector[id].height / maxHeight;
        int vw = w * subVector[id].width / maxWidth;
        return mcu[id][vh / 8][vw / 8][vh % 8][vw % 8];
    }
};


// 讀取 Section 用之輔助函式

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
    length = length * 256 + c;
    return length;
}

unsigned int EnterNewSection(FILE *f, const char *s) {
    showSectionName(s);
    unsigned int len = readSectionLength(f);
    printf("本區段長度為 %d\n", len);
    return len;
}

// 讀取各 Section 之函式

void readCOM(FILE *f) {
    unsigned int len = EnterNewSection(f, "COM");
    unsigned char c;
    for (int i = 0; i < len - 2; i++) {
        fread(&c, 1, 1, f);
        printf("%c", c);
    }
    printf("\n");
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
    len -= 2;
    while (len > 0) {
        unsigned char c;
        fread(&c, 1, 1, f);
        len--;
        unsigned precision = c >> 4 == 0 ? 8 : 16;
        printf("精度：%d\n", precision);
        precision /= 8;
        unsigned char id = c & 0x0F;
        printf("量化表ID: %d\n", id);
        for (int i = 0; i < 64; i++) {
            unsigned char t = 0;
            for (int p = 0; p < precision; p++) {
                unsigned char s;
                fread(&s, 1, 1, f);
                t == t << 8;
                t += s;
            }
            quantTable[id][i] = t;
        }
        for (int i = 0; i < 64; i++) {
            if (i % 8 == 0) {
                printf("\n");
            }
            printf("%2d ", quantTable[id][i]);
        }
        printf("\n");
        len -= (precision*64);
    }
}
void readSOF(FILE *f) {
    unsigned int len = EnterNewSection(f, "SOF");
    fseek(f, 1, SEEK_CUR); // 精度
    unsigned char v[3];
    fread(v, 1, 2, f);
    // TODO: 高度跟寬度不確定
    image.height = v[0] * 256 + v[1];
    fread(v, 1, 2, f);
    image.width = v[0] * 256 + v[1];
    printf("高*寬: %d*%d\n", image.height, image.width);
    fseek(f, 1, SEEK_CUR); // 顏色分量數，固定為3
    for (int i = 0; i < 3; i++) {
        fread(v, 1, 3, f);
        printf("顏色分量ID：%d\n", v[0]);
        printf("水平採樣因子：%d\n", v[1] >> 4);
        printf("垂直採樣因子：%d\n", v[1] & 0x0F);
        printf("量化表ID：%d\n", v[2]);
        subVector[v[0]].id = v[0];
        subVector[v[0]].width = v[1] >> 4;
        subVector[v[0]].height = v[1] & 0x0F;
        subVector[v[0]].quant = v[2];
        maxHeight = (maxHeight > subVector[v[0]].height ? maxHeight : subVector[v[0]].height);
        maxWidth = (maxWidth > subVector[v[0]].width ? maxWidth : subVector[v[0]].width);
    }
}

std::pair<unsigned char, unsigned int>* createHuffCode(unsigned char *a, unsigned int number) {
    int si = sizeof(std::pair<unsigned char, unsigned int>);
    auto ret = (std::pair<unsigned char, unsigned int>*)malloc(si * number);
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
    len -= 2;
    while (len > 0) {
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
        auto huffCode = createHuffCode(a, number);
        for (int i = 0; i < number; i++) {
            unsigned char v;
            fread(&v, 1, 1, f);
            huffTable[DCorAC][id][huffCode[i]] = v;
            printf("%d %d: %d\n", huffCode[i].first, huffCode[i].second, v);
        }
        free(huffCode);

        len -= (1 + 16 + number);
    }
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

// 必須連續呼叫getBit，中間被fread斷掉就會出問題
bool getBit(FILE *f) {
    static unsigned char buf;
    static unsigned char count = 0;
    if (count == 0) {
        fread(&buf, 1, 1, f);
        if (buf == 0xFF) {
            unsigned char check;
            fread(&check, 1, 1, f);
            if (check != 0x00) {
                fprintf(stderr, "data 段有不是 0xFF00 的數據");
            }
        }
    }
    bool ret = buf & (1 << (7 - count));
    count = (count == 7 ? 0 : count + 1);
    return ret;
}

unsigned char matchHuff(FILE *f, unsigned char number, unsigned char ACorDC) {
    unsigned int len = 0;
    unsigned char codeLen;
    for (int count = 1; ; count++) {
        len = len << 1;
        len += (unsigned int)getBit(f);
        if (huffTable[ACorDC][number].find(std::make_pair(count, len)) != huffTable[ACorDC][number].end()) {
            codeLen = huffTable[ACorDC][number][std::make_pair(count, len)];
            return codeLen;
        }
        if (count > 16) {fprintf(stderr, "key not found\n"); count = 1; len = 0;}
    }
}

int readDC(FILE *f, unsigned char number) {
    unsigned char codeLen = matchHuff(f, number, DC);
    if (codeLen == 0) { return 0; }
    unsigned char first = getBit(f);
    int ret = 1;
    for (int i = 1; i < codeLen; i++) {
        unsigned char b = getBit(f);
        ret = ret << 1;
        ret += first ? b : !b;
    }
    ret = first ? ret : -ret;
//    printf("read DC: len %d, value %d\n", codeLen, ret);
    return ret;
}

// 計算ZRL
acCode readAC(FILE *f, unsigned char number) {
    unsigned char x = matchHuff(f, number, AC);
    unsigned char zeros = x >> 4;
    unsigned char codeLen = x & 0x0F;
    if (x == 0) {
        return acCode{0,0,0};
    } else if (x == 0xF0) {
        return acCode{0, 16, 0};
    }
    unsigned  char first = getBit(f);
    int code = 1;
    for (int i = 1; i < codeLen; i++) {
        unsigned char b = getBit(f);
        code = code << 1;
        code += first ? b : !b;
    }
    code = first ? code : -code;
//    printf("read AC: %d %d %d\n", codeLen, zeros, code);
    return acCode{codeLen, zeros, code};
}

MCU readMCU(FILE *f) {
    static int dc[4] = {0, 0, 0, 0};
    auto mcu = MCU();
    for (int i = 1; i <= 3; i++) {
        for (int h = 0; h < subVector[i].height; h++) {
            for (int w = 0; w < subVector[i].width; w++) {
                dc[i] = readDC(f, i/2) + dc[i];
                mcu.mcu[i][h][w][0][0] = dc[i];
                unsigned int count = 1;
                while (count < 64) {
                    acCode ac = readAC(f, i/2);
                    if (ac.len == 0 && ac.zeros == 16) {
                        for (int j = 0; j < ac.zeros; j++) {
                            mcu.mcu[i][h][w][count/8][count%8] = 0;
                            count++;
                        }
                    } else if (ac.len == 0) {
                        break;
                    } else {
                        for (int j = 0; j < ac.zeros; j++) {
                            mcu.mcu[i][h][w][count/8][count%8] = 0;
                            count++;
                        }
                        mcu.mcu[i][h][w][count/8][count%8] = ac.value;
                        count++;
                    }
                }
                while (count < 64) {
                    mcu.mcu[i][h][w][count/8][count%8] = 0;
                    count++;
                }
            }
        }
    }
    return mcu;
}

void readData(FILE *f) {
    printf("************************* test read data **********************************\n");
    int w = (image.width - 1) / (8*maxWidth) + 1;
    int h = (image.height - 1) / (8*maxHeight) + 1;
    BMP *bmp = BMP_Create(maxWidth * 8 * w, maxHeight * 8 * h, 24);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            MCU mcu = readMCU(f);
            mcu.decode();
            RGB **b = mcu.toRGB();
            for (int y = i*8*maxHeight; y < (i+1)*8*maxHeight; y++) {
                for (int x = j*8*maxWidth; x < (j+1)*8*maxWidth; x++) {
                    int by = y - i*8*maxHeight;
                    int bx = x - j*8*maxWidth;
                    BMP_SetPixelRGB(bmp, x, y, b[by][bx].R, b[by][bx].G, b[by][bx].B);
                }
            }
        }
    }
    BMP_WriteFile(bmp, "out.bmp");
}

void readStream(FILE *f) {
    unsigned char c;
    fread(&c, 1, 1, f);
    while (c == 0xFF) {
        fread(&c, 1, 1, f);
        switch (c) {
            case SOI_MARKER:
                printf("Start of Image\n");
                break;
            case APP0_MARKER:
            case 0xE1:
            case 0xE2:
            case 0xE3:
            case 0xE4:
            case 0xE5:
            case 0xE6:
            case 0xE7:
            case 0xE8:
            case 0xE9:
            case 0xEA:
            case 0xEB:
            case 0xEC:
            case 0xED:
            case 0xEE:
            case 0xEF:
                readAPP(f);
                break;
            case COM_MARKER:
                readCOM(f);
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
                readData(f);
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
    init_cos_cache();
    readStream(f);
    return 0;
}
