/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of far-screamer
 * Created on: 22 мая 2021 г.
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

#ifndef PRIVATE_AUDIO_H_
#define PRIVATE_AUDIO_H_

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/filters/Equalizer.h>
#include <lsp-plug.in/expr/Resolver.h>

namespace far_screamer
{
    using namespace lsp;

    /**
     * Load audio file
     *
     * @param sample sample to store audio data
     * @param srate desired sample rate
     * @param name name of the file
     * @return status of operation
     */
    status_t load_audio_file(dspu::Sample *sample, ssize_t srate, const LSPString *name);

    /**
     * Save audio file
     *
     * @param sample sample to save
     * @param fname output file name
     * @return status of operation
     */
    status_t save_audio_file(dspu::Sample *sample, const LSPString *fname);

    /**
     * Convolve the specified channel of input audio file with specified channel of the impulse response
     * and add result to specified channel of the output file
     *
     * @param dst destination sample to add convolution data
     * @param src source sample to use for convolution
     * @param ir impulse response to use for convolution
     * @param dst_ch the number of destination channel
     * @param src_ch the number of source channel
     * @param ir the number of channel in the impulse response
     * @param predelay the predelay in samples of the convolved data
     * @param gain the overall gain of convolution (1.0f = 0 dB)
     * @return status of operation
     */
    status_t convolve(
        dspu::Sample *dst, const dspu::Sample *src, const dspu::Sample *ir,
        size_t dst_ch, size_t src_ch, size_t ir_ch,
        size_t predelay, float gain
    );


    /**
     * Add latency to the sample
     * @param dst destination sample to apply latency
     * @param src source sample to apply latency
     * @param latency latency (in samples) to add
     * @param gain the gain to adjust (1.0f = 0 dB)
     * @return status of operation
     */
    status_t adjust_latency_gain(dspu::Sample *dst, const dspu::Sample *src, size_t latency, float gain);

    /**
     * Apply filters to the audio sample
     * @param dst destination sample to apply filters
     * @param eq filters to apply
     * @return status of operation
     */
    status_t filter_sample(dspu::Sample *dst, dspu::Equalizer *eq);

    /**
     * Apply Mid/Side balance
     * @param dst destination sample to apply balance
     * @param mid middle control
     * @param side Side control
     */
    void apply_mid_side(dspu::Sample *dst, float mid, float side);

}



#endif /* PRIVATE_AUDIO_H_ */
