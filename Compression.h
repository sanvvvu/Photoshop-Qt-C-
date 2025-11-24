#ifndef COMPRESSION_H
#define COMPRESSION_H





// ЕГО НЕ ИСПОЛЬЗУЮ (ВРУЧНУЮ НЕ БУДУ ТЕПЕРЬ, БУДУ ТОЛЬКО ЧЕРЕЗ БИБЛИОТЕКУ) ТОЛЬКО ДЛЯ ЛЗМА






#include <vector>
#include <cstdint>
#include <QString>
#include <QImage>

//каждый метод возвращает буфер с уже закодированными (сжатими) байтами,
//которые можно записать в TIFF (в виде зашифрованных полос/страниц).

namespace Compression {
// Deflate: использую zlib, сжимаю входной массив с уровнями 1-9
bool deflateCompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out, int level);

// LZMA: использую liblzma, степень сжатия 1..9
bool lzmaCompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out, int preset);

// PackBits - RLE (run-length encoding)
void packbitsCompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out);

// LZW Lempel–Ziv–Welch вручную без libtiff (чтобы не тянуть лишние зависимости)
bool lzwCompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out);

// JPEG работает с цветовыми данными
bool jpegCompress(const QImage &img, std::vector<uint8_t> &out, int quality);

// утилита: преобразует QImage в буфер RGB или Grayscale (в сырой массив байтов)
void imageToRGBorGray(const QImage &img, std::vector<uint8_t> &out, int &samplesPerPixel, bool &isGray);
}

#endif
