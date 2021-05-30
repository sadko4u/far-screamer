/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of far-screamer
 * Created on: 20 мая 2021 г.
 *
 * far-screamer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * far-screamer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with far-screamer. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRIVATE_CONFIG_H_
#define PRIVATE_CONFIG_H_

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/dsp-units/filters/common.h>

namespace far_screamer
{
    using namespace lsp;

    typedef struct mapping_t
    {
        size_t      out;        // Number of the output channel
        size_t      in;         // Number of the input channel
        size_t      ir;         // Number of the IR channel
        float       gain;       // The applied gain
    } mapping_t;

    enum normalize_t
    {
        NORM_NONE,              // No normalization
        NORM_ABOVE,             // When the maximum peak is above the threshold
        NORM_BELOW,             // When the maximum peak is below the threshold
        NORM_ALWAYS             // Always normalize
    };

    /**
     * Overall configuration
     */
    struct config_t
    {
        private:
            config_t & operator = (const config_t &);

        public:
            ssize_t                                 nSampleRate;    // Sample rate
            float                                   fDry;           // Amount of dry signal
            float                                   fWet;           // Amount of wet signal
            float                                   fMid;           // Amount of the middle signal
            float                                   fSide;          // Amount of the size signal
            float                                   fPreDelay;      // Pre-delay of the signal
            float                                   fFadeIn;        // Fade-in length
            float                                   fFadeOut;       // Fade-out length
            float                                   fHeadCut;       // Head cut
            float                                   fTailCut;       // Tail cut
            ssize_t                                 nNormalize;     // Normalization method
            float                                   fNormGain;      // Normalization gain
            bool                                    bTrim;          // Trim to original file
            LSPString                               sInFile;        // Source file
            LSPString                               sOutFile;       // Destination file
            LSPString                               sIRFile;        // Impulse response file
            dspu::filter_params_t                   sLPF;           // Low-pass filter
            dspu::filter_params_t                   sHPF;           // Hi-pass filter
            lltl::darray<mapping_t>                 sMapping;       // Mapping of the IR convolution

        public:
            explicit config_t();
            ~config_t();

        public:
            void clear();
    };
}


#endif /* PRIVATE_CONFIG_H_ */
