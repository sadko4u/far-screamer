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

#include <private/audio.h>
#include <private/config.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/expr/Expression.h>
#include <lsp-plug.in/dsp-units/misc/windows.h>
#include <lsp-plug.in/dsp-units/misc/fade.h>
#include <lsp-plug.in/dsp-units/util/Convolver.h>

namespace far_screamer
{
    using namespace lsp;

    typedef struct duration_t
    {
        size_t h;
        size_t m;
        size_t s;
        size_t ms;
    } duration_t;

    void calc_duration(duration_t *d, const dspu::Sample *sample)
    {
        uint64_t duration = (uint64_t(sample->samples()) * 1000) / sample->sample_rate();
        d->ms = duration % 1000;
        duration /= 1000;
        d->s = duration % 60;
        duration /= 60;
        d->m = duration % 60;
        d->h = duration / 60;
    }

    status_t load_audio_file(dspu::Sample *sample, ssize_t srate, const LSPString *name)
    {
        status_t res;
        io::Path path;

        // Generate file name
        if ((res = path.set(name)) != STATUS_OK)
        {
            fprintf(stderr, "  could not read file '%s', error code: %d\n", name->get_native(), int(res));
            return res;
        }

        // Load sample from file
        if ((res = sample->load(&path)) != STATUS_OK)
        {
            fprintf(stderr, "  could not read file '%s', error code: %d\n", path.as_native(), int(res));
            return res;
        }

        duration_t d;
        calc_duration(&d, sample);
        fprintf(stdout, "  loaded file: '%s', channels: %d, samples: %d, sample rate: %d, duration: %02d:%02d:%02d.%03d\n",
                path.as_native(),
                int(sample->channels()), int(sample->length()), int(sample->sample_rate()),
                int(d.h), int(d.m), int(d.s), int(d.ms)
        );

        // Resample audio data
        if (srate > 0)
        {
            if ((res = sample->resample(srate)) != STATUS_OK)
            {
                fprintf(stderr, "  could not resample file '%s' to sample rate %d, error code: %d\n",
                        path.as_native(), int(srate), int(res)
                );
                return res;
            }
        }

        return STATUS_OK;
    }

    status_t save_audio_file(dspu::Sample *sample, const LSPString *fname)
    {
        status_t res;
        expr::Expression x;
        io::Path path, dir;

        // Generate file name
        if ((res = path.set(fname)) != STATUS_OK)
        {
            fprintf(stderr, "  could not write file '%s', error code: %d\n", fname->get_native(), int(res));
            return res;
        }

        // Create parent directory recursively
        res = path.get_parent(&dir);
        if (res == STATUS_OK)
        {
            if ((res = dir.mkdir(true)) != STATUS_OK)
            {
                fprintf(stderr, "  could not create directory '%s', error code: %d\n", dir.as_native(), int(res));
                return res;
            }
        }
        else if (res != STATUS_NOT_FOUND)
        {
            fprintf(stderr, "  could not obtain parent directory for file '%s', error code: %d\n", fname->get_native(), int(res));
            return res;
        }

        // Load sample from file
        if ((res = sample->save(&path)) < 0)
        {
            fprintf(stderr, "  could not write file '%s', error code: %d\n", path.as_native(), int(-res));
            return -res;
        }

        duration_t d;
        calc_duration(&d, sample);
        fprintf(stdout, "  saved file: '%s', channels: %d, samples: %d, sample rate: %d, duration: %02d:%02d:%02d.%03d\n",
                path.as_native(),
                int(sample->channels()), int(sample->length()), int(sample->sample_rate()),
                int(d.h), int(d.m), int(d.s), int(d.ms)
        );

        return STATUS_OK;
    }

    status_t convolve(
        dspu::Sample *dst, const dspu::Sample *src, const dspu::Sample *ir,
        size_t dst_ch, size_t src_ch, size_t ir_ch,
        size_t predelay, float gain
    )
    {
        dspu::Sample out;
        dspu::Convolver cv;

        // Check channel numbers
        if (src_ch >= src->channels())
        {
            fprintf(stderr, "Invalid channel number for input file: %d\n", int(src_ch));
            return STATUS_BAD_ARGUMENTS;
        }
        if (ir_ch >= ir->channels())
        {
            fprintf(stderr, "Invalid channel number for impulse response file: %d\n", int(ir_ch));
            return STATUS_BAD_ARGUMENTS;
        }

        // Allocate necessary buffers
        size_t dry_length   = src->length();
        size_t wet_length   = dry_length + ir->length(); // The length of wet (processed) signal
        size_t length       = lsp_max(dry_length, wet_length);

        // Allocate the necessary space for the output sample
        size_t num_ch       = lsp_max(dst_ch + 1, dst->channels());
        if ((num_ch != dst->channels()) || (length > dst->length()))
        {
            status_t res = dst->resize(num_ch, length, length);
            if (res != STATUS_OK)
            {
                fprintf(stderr, "Not enough memory to resize the output audio data\n");
                return res;
            }
        }

        // Allocate buffer for convolution tail
        uint8_t *ptr;
        float *buf          = alloc_aligned<float>(ptr, length);
        if (buf == NULL)
        {
            fprintf(stderr, "Not enough memory to allocate temporary buffer\n");
            return STATUS_NO_MEM;
        }

        // Perform convolution
        if (!cv.init(ir->channel(ir_ch), ir->length(), 16, 0))
        {
            free_aligned(ptr);
            fprintf(stderr, "Not enough memory to initialize convolver\n");
            return STATUS_NO_MEM;
        }
        dsp::fill_zero(buf, length); // Fill head of buffer with zeros
        cv.process(&buf[0], src->channel(src_ch), dry_length); // The main convolution
        cv.process(&buf[dry_length], &buf[dry_length], ir->length()); // The tail of convolution

        // Apply convolution to the output sample
        float *dptr = dst->channel(dst_ch);
        dsp::fmadd_k3(&dptr[predelay], buf, gain, length);

        // Free temporary buffer
        free_aligned(ptr);

        return STATUS_OK;
    }

    status_t adjust_latency_gain(dspu::Sample *dst, const dspu::Sample *src, size_t latency, float gain)
    {
        size_t channels     = src->channels();
        size_t length       = src->length();
        size_t new_length   = latency + length;

        if (channels <= 0)
            return STATUS_OK;

        // Resize sample to the new size with added amount of latency
        if ((dst->length() != new_length) || (dst->channels() != channels))
        {
            status_t res = dst->resize(channels, new_length, new_length);
            if (res != STATUS_OK)
            {
                fprintf(stderr, "Could not resize audio sample to %d channels, %d samples\n",
                        int(channels), int(new_length));
                return res;
            }
        }

        // Apply latency and gain to the sample
        for (size_t i=0; i<channels; ++i)
        {
            const float *sbuf   = src->channel(i);
            float *dbuf         = dst->channel(i);
            dsp::mul_k3(&dbuf[latency], sbuf, gain, length);
            dsp::fill_zero(&dbuf[0], latency);          // Pad with zeros
        }

        return STATUS_OK;
    }

    status_t filter_sample(dspu::Sample *dst, dspu::Equalizer *eq)
    {
        size_t channels     = dst->channels();
        size_t length       = dst->length();

        if ((channels <= 0) || (eq->mode() == dspu::EQM_BYPASS))
            return STATUS_OK;

        // Resize sample to the new size with added amount of latency and IR length
        size_t new_length   = length + eq->get_latency() + eq->ir_size();
        if (length != new_length)
        {
            status_t res = dst->resize(channels, new_length, new_length);
            if (res != STATUS_OK)
            {
                fprintf(stderr, "Could not resize audio sample to %d channels, %d samples\n",
                        int(channels), int(new_length));
                return res;
            }
        }

        // Perform audio filtering
        for (size_t i=0; i<channels; ++i)
        {
            float *buf  = dst->channel(i);
            eq->reset();                        // Reset the state of equalizer
            eq->process(buf, buf, new_length);  // Process the channel by the equalizer
        }

        return STATUS_OK;
    }

    void apply_mid_side(dspu::Sample *dst, float mid, float side)
    {
        if (dst->channels() == 1)
        {
            printf("  mono output has no side part, adjusting only mono part\n");
            dsp::mul_k2(dst->channel(0), mid, dst->length());
        }
        else if (dst->channels() == 2)
        {
            printf("  adjusting Mid/Side balance for stereo output signal\n");

            float *a = dst->channel(0);
            float *b = dst->channel(1);
            size_t length = dst->length();
            dsp::lr_to_ms(a, b, a, b, length);
            dsp::mul_k2(a, mid, length);
            dsp::mul_k2(b, side, length);
            dsp::ms_to_lr(a, b, a, b, length);
        }
        else
        {
            printf("  unsupported mid/side balancing for %d output channels, skipping", int(dst->channels()));
        }
    }

    status_t cut_sample(dspu::Sample *dst, size_t head_cut, size_t tail_cut, size_t fade_in, size_t fade_out)
    {
        ssize_t new_length = dst->length() - (head_cut + tail_cut);
        if (new_length <= 0)
        {
            printf("  empty output sample after cutting head and tail, can not proceed\n");
            return STATUS_UNDERFLOW;
        }

        // Cut the body
        size_t channels = dst->channels();
        for (size_t i=0; i<channels; ++i)
        {
            float *channel  = dst->channel(i);
            // Apply cut
            dsp::move(channel, &channel[head_cut], new_length);
            // Apply fades
            dspu::fade_in(channel, channel, fade_in, new_length);
            dspu::fade_out(channel, channel, fade_out, new_length);
        }

        // Resize the sample and exit
        dst->set_length(new_length);
        return STATUS_OK;
    }

    status_t normalize(dspu::Sample *dst, float gain, size_t mode)
    {
        if (mode == NORM_NONE)
            return STATUS_OK;

        float peak  = 0.0f;
        for (size_t i=0, n=dst->channels(); i<n; ++i)
        {
            float cpeak = dsp::abs_max(dst->channel(i), dst->length());
            peak        = lsp_max(peak, cpeak);
        }

        // No peak detected?
        if (peak < 1e-6)
            return STATUS_OK;

        switch (mode)
        {
            case NORM_BELOW:
                if (peak >= gain)
                    return STATUS_OK;
                break;
            case NORM_ABOVE:
                if (peak <= gain)
                    return STATUS_OK;
                break;
            default:
                break;
        }

        // Adjust gain
        float k = gain / peak;
        for (size_t i=0, n=dst->channels(); i<n; ++i)
            dsp::mul_k2(dst->channel(i), k, dst->length());

        return STATUS_OK;
    }
}



