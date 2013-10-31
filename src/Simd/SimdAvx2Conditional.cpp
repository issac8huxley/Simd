/*
* Simd Library.
*
* Copyright (c) 2011-2013 Yermalayeu Ihar.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy 
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
* copies of the Software, and to permit persons to whom the Software is 
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in 
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "Simd/SimdMemory.h"
#include "Simd/SimdLoad.h"
#include "Simd/SimdStore.h"
#include "Simd/SimdExtract.h"
#include "Simd/SimdConst.h"
#include "Simd/SimdMath.h"
#include "Simd/SimdSet.h"
#include "Simd/SimdCompare.h"
#include "Simd/SimdAvx2.h"

namespace Simd
{
#ifdef SIMD_AVX2_ENABLE    
	namespace Avx2
	{
        template <bool align, SimdCompareType compareType> 
        void ConditionalCount(const uchar * src, size_t stride, size_t width, size_t height, uchar value, uint * count)
        {
            assert(width >= A);
            if(align)
                assert(Aligned(src) && Aligned(stride));

            size_t alignedWidth = Simd::AlignLo(width, A);
            __m256i tailMask = SetMask<uchar>(0, A - width + alignedWidth, 0xFF);

            __m256i _value = _mm256_set1_epi8(value);
            __m256i _count = _mm256_setzero_si256();
            for(size_t row = 0; row < height; ++row)
            {
                for(size_t col = 0; col < alignedWidth; col += A)
                {
                    const __m256i mask = Compare<compareType>(Load<align>((__m256i*)(src + col)), _value);
                    _count = _mm256_add_epi64(_count, _mm256_sad_epu8(_mm256_and_si256(mask, K8_01), K_ZERO));
                }
                if(alignedWidth != width)
                {
                    const __m256i mask = _mm256_and_si256(Compare<compareType>(Load<false>((__m256i*)(src + width - A)), _value), tailMask);
                    _count = _mm256_add_epi64(_count, _mm256_sad_epu8(_mm256_and_si256(mask, K8_01), K_ZERO));
                }
                src += stride;
            }
            *count = ExtractSum<uint32_t>(_count);
        }

        template <SimdCompareType compareType> 
        void ConditionalCount(const uchar * src, size_t stride, size_t width, size_t height, uchar value, uint * count)
        {
            if(Aligned(src) && Aligned(stride))
                ConditionalCount<true, compareType>(src, stride, width, height, value, count);
            else
                ConditionalCount<false, compareType>(src, stride, width, height, value, count);
        }

        void ConditionalCount(const uchar * src, size_t stride, size_t width, size_t height, 
            uchar value, SimdCompareType compareType, uint * count)
        {
            switch(compareType)
            {
            case SimdCompareEqual: 
                return ConditionalCount<SimdCompareEqual>(src, stride, width, height, value, count);
            case SimdCompareNotEqual: 
                return ConditionalCount<SimdCompareNotEqual>(src, stride, width, height, value, count);
            case SimdCompareGreater: 
                return ConditionalCount<SimdCompareGreater>(src, stride, width, height, value, count);
            case SimdCompareGreaterOrEqual: 
                return ConditionalCount<SimdCompareGreaterOrEqual>(src, stride, width, height, value, count);
            case SimdCompareLesser: 
                return ConditionalCount<SimdCompareLesser>(src, stride, width, height, value, count);
            case SimdCompareLesserOrEqual: 
                return ConditionalCount<SimdCompareLesserOrEqual>(src, stride, width, height, value, count);
            default: 
                assert(0);
            }
        }

        template <bool align, SimdCompareType compareType> 
        void ConditionalSum(const uchar * src, size_t srcStride, size_t width, size_t height, 
            const uchar * mask, size_t maskStride, uchar value, uint64_t * sum)
        {
            assert(width >= A);
            if(align)
                assert(Aligned(src) && Aligned(srcStride) && Aligned(mask) && Aligned(maskStride));

            size_t alignedWidth = Simd::AlignLo(width, A);
            __m256i tailMask = SetMask<uchar>(0, A - width + alignedWidth, 0xFF);

            __m256i _value = _mm256_set1_epi8(value);
            __m256i _sum = _mm256_setzero_si256();
            for(size_t row = 0; row < height; ++row)
            {
                for(size_t col = 0; col < alignedWidth; col += A)
                {
                    const __m256i _src = Load<align>((__m256i*)(src + col));
                    const __m256i _mask = Compare<compareType>(Load<align>((__m256i*)(mask + col)), _value);
                    _sum = _mm256_add_epi64(_sum, _mm256_sad_epu8(_mm256_and_si256(_mask, _src), K_ZERO));
                }
                if(alignedWidth != width)
                {
                    const __m256i _src = Load<false>((__m256i*)(src + width - A));
                    const __m256i _mask = _mm256_and_si256(Compare<compareType>(Load<false>((__m256i*)(mask + width - A)), _value), tailMask);
                    _sum = _mm256_add_epi64(_sum, _mm256_sad_epu8(_mm256_and_si256(_mask, _src), K_ZERO));
                }
                src += srcStride;
                mask += maskStride;
            }
            *sum = ExtractSum<uint64_t>(_sum);
        }

        template <SimdCompareType compareType> 
        void ConditionalSum(const uchar * src, size_t srcStride, size_t width, size_t height, 
            const uchar * mask, size_t maskStride, uchar value, uint64_t * sum)
        {
            if(Aligned(src) && Aligned(srcStride) && Aligned(mask) && Aligned(maskStride))
                ConditionalSum<true, compareType>(src, srcStride, width, height, mask, maskStride, value, sum);
            else
                ConditionalSum<false, compareType>(src, srcStride, width, height, mask, maskStride, value, sum);
        }

        void ConditionalSum(const uchar * src, size_t srcStride, size_t width, size_t height, 
            const uchar * mask, size_t maskStride, uchar value, SimdCompareType compareType, uint64_t * sum)
        {
            switch(compareType)
            {
            case SimdCompareEqual: 
                return ConditionalSum<SimdCompareEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareNotEqual: 
                return ConditionalSum<SimdCompareNotEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareGreater: 
                return ConditionalSum<SimdCompareGreater>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareGreaterOrEqual: 
                return ConditionalSum<SimdCompareGreaterOrEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareLesser: 
                return ConditionalSum<SimdCompareLesser>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareLesserOrEqual: 
                return ConditionalSum<SimdCompareLesserOrEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            default: 
                assert(0);
            }
        }

        SIMD_INLINE __m256i Square(__m256i value)
        {
            const __m256i lo = _mm256_unpacklo_epi8(value, _mm256_setzero_si256());
            const __m256i hi = _mm256_unpackhi_epi8(value, _mm256_setzero_si256());
            return _mm256_add_epi32(_mm256_madd_epi16(lo, lo), _mm256_madd_epi16(hi, hi));
        }

        template <bool align, SimdCompareType compareType> 
        void ConditionalSquareSum(const uchar * src, size_t srcStride, size_t width, size_t height, 
            const uchar * mask, size_t maskStride, uchar value, uint64_t * sum)
        {
            assert(width >= A);
            if(align)
                assert(Aligned(src) && Aligned(srcStride) && Aligned(mask) && Aligned(maskStride));

            size_t alignedWidth = Simd::AlignLo(width, A);
            __m256i tailMask = SetMask<uchar>(0, A - width + alignedWidth, 0xFF);

            __m256i _value = _mm256_set1_epi8(value);
            __m256i _sum = _mm256_setzero_si256();
            for(size_t row = 0; row < height; ++row)
            {
                __m256i rowSum = _mm256_setzero_si256();
                for(size_t col = 0; col < alignedWidth; col += A)
                {
                    const __m256i _src = Load<align>((__m256i*)(src + col));
                    const __m256i _mask = Compare<compareType>(Load<align>((__m256i*)(mask + col)), _value);
                    rowSum = _mm256_add_epi32(rowSum, Square(_mm256_and_si256(_mask, _src)));
                }
                if(alignedWidth != width)
                {
                    const __m256i _src = Load<false>((__m256i*)(src + width - A));
                    const __m256i _mask = _mm256_and_si256(Compare<compareType>(Load<false>((__m256i*)(mask + width - A)), _value), tailMask);
                    rowSum = _mm256_add_epi32(rowSum, Square(_mm256_and_si256(_mask, _src)));
                }
                _sum = _mm256_add_epi64(_sum, HorizontalSum32(rowSum));
                src += srcStride;
                mask += maskStride;
            }
            *sum = ExtractSum<uint64_t>(_sum);
        }

        template <SimdCompareType compareType> 
        void ConditionalSquareSum(const uchar * src, size_t srcStride, size_t width, size_t height, 
            const uchar * mask, size_t maskStride, uchar value, uint64_t * sum)
        {
            if(Aligned(src) && Aligned(srcStride) && Aligned(mask) && Aligned(maskStride))
                ConditionalSquareSum<true, compareType>(src, srcStride, width, height, mask, maskStride, value, sum);
            else
                ConditionalSquareSum<false, compareType>(src, srcStride, width, height, mask, maskStride, value, sum);
        }

        void ConditionalSquareSum(const uchar * src, size_t srcStride, size_t width, size_t height, 
            const uchar * mask, size_t maskStride, uchar value, SimdCompareType compareType, uint64_t * sum)
        {
            switch(compareType)
            {
            case SimdCompareEqual: 
                return ConditionalSquareSum<SimdCompareEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareNotEqual: 
                return ConditionalSquareSum<SimdCompareNotEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareGreater: 
                return ConditionalSquareSum<SimdCompareGreater>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareGreaterOrEqual: 
                return ConditionalSquareSum<SimdCompareGreaterOrEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareLesser: 
                return ConditionalSquareSum<SimdCompareLesser>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareLesserOrEqual: 
                return ConditionalSquareSum<SimdCompareLesserOrEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            default: 
                assert(0);
            }
        }

        template <bool align>
        SIMD_INLINE __m256i SquaredDifference(const uchar * src, ptrdiff_t step, __m256i mask)
        {
            const __m256i a = _mm256_and_si256(Load<align>((__m256i*)(src - step)), mask);
            const __m256i b = _mm256_and_si256(Load<align>((__m256i*)(src + step)), mask);
            const __m256i lo = _mm256_sub_epi16(_mm256_unpacklo_epi8(a, _mm256_setzero_si256()), _mm256_unpacklo_epi8(b, _mm256_setzero_si256()));
            const __m256i hi = _mm256_sub_epi16(_mm256_unpackhi_epi8(a, _mm256_setzero_si256()), _mm256_unpackhi_epi8(b, _mm256_setzero_si256()));
            return _mm256_add_epi32(_mm256_madd_epi16(lo, lo), _mm256_madd_epi16(hi, hi));
        }

        template <bool align, SimdCompareType compareType> 
        void ConditionalSquareGradientSum(const uchar * src, size_t srcStride, size_t width, size_t height, 
            const uchar * mask, size_t maskStride, uchar value, uint64_t * sum)
        {
            assert(width >= A + 3 && height >= A + 3);
            if(align)
                assert(Aligned(src) && Aligned(srcStride) && Aligned(mask) && Aligned(maskStride));

            src += srcStride;
            mask += maskStride;
            height -= 2;

            size_t alignedWidth = Simd::AlignLo(width - 1, A);
            __m256i noseMask = SetMask<uchar>(0xFF, A - 1, 0);
            __m256i tailMask = SetMask<uchar>(0, A - width + 1 + alignedWidth, 0xFF);

            __m256i _value = _mm256_set1_epi8(value);
            __m256i _sum = _mm256_setzero_si256();
            for(size_t row = 0; row < height; ++row)
            {
                __m256i rowSum = _mm256_setzero_si256();
                {
                    const __m256i _mask = _mm256_and_si256(Compare<compareType>(Load<false>((__m256i*)(mask + 1)), _value), noseMask);
                    rowSum = _mm256_add_epi32(rowSum, SquaredDifference<false>(src + 1, 1, _mask));
                    rowSum = _mm256_add_epi32(rowSum, SquaredDifference<false>(src + 1, srcStride, _mask));
                }
                for(size_t col = A; col < alignedWidth; col += A)
                {
                    const __m256i _mask = Compare<compareType>(Load<align>((__m256i*)(mask + col)), _value);
                    rowSum = _mm256_add_epi32(rowSum, SquaredDifference<false>(src + col, 1, _mask));
                    rowSum = _mm256_add_epi32(rowSum, SquaredDifference<align>(src + col, srcStride, _mask));
                }
                if(alignedWidth != width - 1)
                {
                    size_t offset = width - A - 1;
                    const __m256i _mask = _mm256_and_si256(Compare<compareType>(Load<false>((__m256i*)(mask + offset)), _value), tailMask);
                    rowSum = _mm256_add_epi32(rowSum, SquaredDifference<false>(src + offset, 1, _mask));
                    rowSum = _mm256_add_epi32(rowSum, SquaredDifference<false>(src + offset, srcStride, _mask));
                }
                _sum = _mm256_add_epi64(_sum, HorizontalSum32(rowSum));
                src += srcStride;
                mask += maskStride;
            }
            *sum = ExtractSum<uint64_t>(_sum);
        }

        template <SimdCompareType compareType> 
        void ConditionalSquareGradientSum(const uchar * src, size_t srcStride, size_t width, size_t height, 
            const uchar * mask, size_t maskStride, uchar value, uint64_t * sum)
        {
            if(Aligned(src) && Aligned(srcStride) && Aligned(mask) && Aligned(maskStride))
                ConditionalSquareGradientSum<true, compareType>(src, srcStride, width, height, mask, maskStride, value, sum);
            else
                ConditionalSquareGradientSum<false, compareType>(src, srcStride, width, height, mask, maskStride, value, sum);
        }

        void ConditionalSquareGradientSum(const uchar * src, size_t srcStride, size_t width, size_t height, 
            const uchar * mask, size_t maskStride, uchar value, SimdCompareType compareType, uint64_t * sum)
        {
            switch(compareType)
            {
            case SimdCompareEqual: 
                return ConditionalSquareGradientSum<SimdCompareEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareNotEqual: 
                return ConditionalSquareGradientSum<SimdCompareNotEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareGreater: 
                return ConditionalSquareGradientSum<SimdCompareGreater>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareGreaterOrEqual: 
                return ConditionalSquareGradientSum<SimdCompareGreaterOrEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareLesser: 
                return ConditionalSquareGradientSum<SimdCompareLesser>(src, srcStride, width, height, mask, maskStride, value, sum);
            case SimdCompareLesserOrEqual: 
                return ConditionalSquareGradientSum<SimdCompareLesserOrEqual>(src, srcStride, width, height, mask, maskStride, value, sum);
            default: 
                assert(0);
            }
        }
	}
#endif// SIMD_AVX2_ENABLE
}