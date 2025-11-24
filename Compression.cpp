#include "Compression.h"
#include <cstring>
#include <zlib.h>
#include <lzma.h>
#include <jpeglib.h>
#include <set>
#include <map>
#include <cassert>  //проаверки
#include <QDebug>




// ЕГО НЕ ИСПОЛЬЗУЮ (ВРУЧНУЮ НЕ БУДУ ТЕПЕРЬ, БУДУ ТОЛЬКО ЧЕРЕЗ БИБЛИОТЕКУ) ТОЛЬКО ДЛЯ ЛЗМА





// писало ошибку библиотека не видела, поэтому добавил сюда
#ifndef LZMA_PROPS_SIZE
#define LZMA_PROPS_SIZE 5
#endif

namespace Compression {

// Deflate разбивает поток на блоки, ищет повторы + применяет энтропийное кодирование
bool deflateCompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out, int level) {
    if (level < 1) level = 1;
    if (level > 9) level = 9;

    // вычисляем максимальный размер выходного буфера
    uLongf destLen = compressBound(in.size());  // заранее говорю, какой максимальный размер может получиться, чтобы мы выделили нужный буфер
    out.resize(destLen);
    int zres = ::compress2(out.data(), &destLen, in.data(), in.size(), level);
    if (zres != Z_OK) {
        qWarning() << "deflateCompress: zlib compress2 неудачно" << zres;
        out.clear();
        return false;
    }
    out.resize(destLen);    // только реально использованная часть
    return true;
}

// использует огромное окно поиска и арифметическое кодирование
bool lzmaCompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out, int preset) {
    if (preset < 1) preset = 1;
    if (preset > 9) preset = 9;
    // liblzma: lzma_easy_buffer_encode
    size_t propsSize = LZMA_PROPS_SIZE;
    (void)propsSize; // подавляем warning, если propsSize пока не используется

    // примерный размер выходного буфера
    size_t outMax = in.size() + in.size() / 3 + 128;
    out.resize(outMax);
    size_t destLen = outMax;

    // ищеm повторяющиеся шаблоны байт в большом “окне” (до мегабайтов).
    // запоминаем, где уже встречался этот блок, и заменяем его короткой ссылкой
    lzma_ret r = lzma_easy_buffer_encode(preset, LZMA_CHECK_CRC64,  
                                         nullptr,  
                                         in.data(), in.size(), 
                                         out.data(), &destLen, out.size()); 
    if (r != LZMA_OK) {
        qWarning() << "lzmaCompress: lzma_easy_buffer_encode неудачно" << int(r);
        out.clear();
        return false;
    }
    out.resize(destLen);
    return true;
}

// PackBits Если подряд идут одинаковые байты → пишем 2 байта: countByte = 257 - длина и значение.
// eсли разные — пишем «литеральный» блок: countByte = длина - 1, затем сами байты.
void packbitsCompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out) {
    size_t i = 0;
    size_t n = in.size();
    out.clear();
    while (i < n) {
        size_t runStart = i;
        size_t runLen = 1;
        // ищем подряд идущие одинаковые байты
        while (i + runLen < n && in[i] == in[i + runLen] && runLen < 128) runLen++;
        if (runLen >= 3) {

            out.push_back(static_cast<uint8_t>(257 - runLen)); 
            out.push_back(in[i]);
            i += runLen;
            continue;
        }
        // иначе литеральный блок
        size_t litStart = i;
        size_t litLen = 0;
        while (i < n) {
            size_t look = 1;
            while (i + look < n && in[i + look - 1] == in[i + look] && look < 128) look++;
            if (look >= 3 && litLen > 0) break;
            i++;
            litLen++;
            if (litLen >= 128) break;
        }

        out.push_back(static_cast<uint8_t>(litLen - 1));
        out.insert(out.end(), in.begin() + litStart, in.begin() + litStart + litLen);
    }
}

/* cоздаю словарь с кодами 0-255, потом новые комбинации с 258
    слова по мере чтения +, словарь до 4096 - сброс*/
bool lzwCompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out) {
    if (in.empty()) { out.clear(); return true; }


    const int MAX_BITS = 12;
    const int MAX_TABLE = 1 << MAX_BITS;
    int clearCode = 256;
    int eoiCode = 257;
    int nextCode = 258;
    int codeSize = 9;
    int codeMask = (1 << codeSize) - 1;

    std::map<std::vector<uint8_t>, int> dict;
    for (int i = 0; i < 256; ++i) {
        std::vector<uint8_t> v(1, static_cast<uint8_t>(i));
        dict[v] = i;
    }

    auto emitBits = [&](uint32_t bits, int nbits, std::vector<uint8_t> &buffer, int &bitPos, uint32_t &acc) {
        acc |= (bits << bitPos);
        bitPos += nbits;
        while (bitPos >= 8) {
            buffer.push_back(static_cast<uint8_t>(acc & 0xFF));
            acc >>= 8;
            bitPos -= 8;
        }
    };

    std::vector<uint8_t> outbuf;
    uint32_t acc = 0;
    int bitPos = 0;

    // запись код в поток (упаковка битов)
    emitBits(clearCode, codeSize, outbuf, bitPos, acc);

    std::vector<uint8_t> w;
    w.push_back(in[0]);
    for (size_t i = 1; i < in.size(); ++i) {
        uint8_t k = in[i];
        std::vector<uint8_t> wk = w;
        wk.push_back(k);
        if (dict.find(wk) != dict.end()) {
            w = wk;
        } else {
            int codeW = dict[w];
            emitBits(codeW, codeSize, outbuf, bitPos, acc);

            if (nextCode < MAX_TABLE) {
                dict[wk] = nextCode++;
                if (nextCode == (1 << codeSize) && codeSize < MAX_BITS) {
                    codeSize++;
                }
            } else {
                emitBits(clearCode, codeSize, outbuf, bitPos, acc);
                dict.clear();
                for (int j = 0; j < 256; ++j) {
                    std::vector<uint8_t> v(1, static_cast<uint8_t>(j));
                    dict[v] = j;
                }
                nextCode = 258;
                codeSize = 9;
            }
            w.clear();
            w.push_back(k);
        }
    }

    if (!w.empty()) {
        int codeW = dict[w];
        emitBits(codeW, codeSize, outbuf, bitPos, acc);
    }

    emitBits(eoiCode, codeSize, outbuf, bitPos, acc);

    while (bitPos > 0) {
        outbuf.push_back(static_cast<uint8_t>(acc & 0xFF));
        acc >>= 8;
        bitPos -= 8;
    }
    out.swap(outbuf);
    return true;
}

//JPEG с потерями: разбивает на блоки 8×8, применяет косинусное преобразование, и обрезает малозаметные коэффициенты
bool jpegCompress(const QImage &img, std::vector<uint8_t> &out, int quality) {
    QImage tmp = img.convertToFormat(QImage::Format_RGB888);
    int width = tmp.width();
    int height = tmp.height();
    int channels = 3;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char *mem = nullptr;
    unsigned long memSize = 0;
    jpeg_mem_dest(&cinfo, &mem, &memSize);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = channels;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];
    const uchar *data = tmp.constBits();
    int row_stride = width * channels;
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = const_cast<JSAMPLE*>(data + cinfo.next_scanline * row_stride);
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    out.assign(mem, mem + memSize);

    jpeg_destroy_compress(&cinfo);
    if (mem) {
        free(mem);
    }
    return true;
}

// преобразует изображение в линейный массив пикселей
void imageToRGBorGray(const QImage &img, std::vector<uint8_t> &out, int &samplesPerPixel, bool &isGray) {
    QImage tmp = img;
    isGray = false;
    if (tmp.format() != QImage::Format_RGB888 && tmp.format() != QImage::Format_Grayscale8) {
        if (tmp.hasAlphaChannel()) tmp = tmp.convertToFormat(QImage::Format_ARGB32);
        tmp = tmp.convertToFormat(QImage::Format_RGB888);
    }
    if (tmp.format() == QImage::Format_Grayscale8) {
        isGray = true;
        samplesPerPixel = 1;
        int w = tmp.width(), h = tmp.height();
        out.resize(w * h);
        for (int y = 0; y < h; ++y) {
            const uchar *line = tmp.constScanLine(y);
            memcpy(out.data() + y * w, line, w);
        }
    } else {
        isGray = false;
        samplesPerPixel = 3;
        int w = tmp.width(), h = tmp.height();
        out.resize(w * h * 3);
        for (int y = 0; y < h; ++y) {
            const uchar *line = tmp.constScanLine(y);
            memcpy(out.data() + y * w * 3, line, w * 3);
        }
    }
}

}
