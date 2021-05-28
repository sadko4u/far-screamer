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

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/stdlib/stdio.h>

#include <private/tool.h>
#include <private/config.h>
#include <private/cmdline.h>
#include <private/audio.h>

#define MIN_SAMPLE_RATE         8000
#define MAX_SAMPLE_RATE         192000
#define MIN_GAIN                -200.0f

namespace far_screamer
{
    using namespace lsp;

    status_t apply_equalizer(size_t *latency, dspu::Sample *dst, const config_t *cfg)
    {
        // Configure the equalizer
        dspu::Equalizer eq;

        if (!eq.init(2, 0))
        {
            fprintf(stderr, "Not enough memory to initialize equalizer\n");
            return STATUS_NO_MEM;
        }

        eq.set_sample_rate(cfg->nSampleRate);
        eq.set_mode(dspu::EQM_IIR);
        eq.reset();

        // Setup filters
        size_t index = 0;
        if (cfg->sLPF.nType != dspu::FLT_NONE)
            eq.set_params(index++, &cfg->sLPF);
        if (cfg->sHPF.nType != dspu::FLT_NONE)
            eq.set_params(index++, &cfg->sHPF);
        if (index <= 0)
            return STATUS_OK;

        // Perform audio processing of the IR file
        filter_sample(dst, &eq);
        *latency = eq.get_latency();

        return STATUS_OK;
    }

    bool contains_mapping(const config_t *cfg, size_t oc, size_t ic)
    {
        for (size_t i=0, n=cfg->sMapping.size(); i<n; ++i)
        {
            const mapping_t *m = cfg->sMapping.uget(i);
            if ((m->in == ic) && (m->out == oc))
                return true;
        }
        return false;
    }

    status_t convolve_data(dspu::Sample *out, const dspu::Sample *in, const dspu::Sample *ir, config_t *cfg, size_t latency)
    {
        // Flags that indicate that dry signal has been emitted to the specified output track
        size_t predelay = dspu::millis_to_samples(cfg->nSampleRate, cfg->fPreDelay);
        size_t out_length = in->length() + ir->length() + latency + predelay;
        float g_dry = (cfg->fDry >= MIN_GAIN) ? dspu::db_to_gain(cfg->fDry) : 0.0f;

        if (cfg->sMapping.is_empty())
        {
            mapping_t *xm;

            // Check typical configurations
            if ((in->channels() == 2) && (ir->channels() == 2))
            {
                // Stereo convolution
                printf("Applying Stereo IR to stereo file\n");
                if (!(xm = cfg->sMapping.add_n(2)))
                {
                    fprintf(stderr, "Not enough memory for generating mapping data\n");
                    return STATUS_NO_MEM;
                }

                // Simple mapping
                for (size_t i=0; i<2; ++i)
                {
                    xm[i].in    = i;
                    xm[i].out   = i;
                    xm[i].ir    = i;
                    xm[i].gain  = 1.0f;
                }
            }
            else if ((in->channels() == 2) && (ir->channels() == 4))
            {
                // True reverb convolution
                printf("  applying TrueReverb convolution schema\n");
                if (!(xm = cfg->sMapping.add_n(4)))
                {
                    fprintf(stderr, "Not enough memory for generating mapping data\n");
                    return STATUS_NO_MEM;
                }

                // Left channel convolved with channels 1 and 2 of the IR
                // Right channel convolved with channels 3 and 4 of the IR
                for (size_t i=0; i<4; ++i)
                {
                    xm[i].in    = i >> 1;
                    xm[i].out   = i >> 1;
                    xm[i].ir    = i;
                    xm[i].gain  = 1.0f;
                }
            }
            else if (in->channels() == 1)
            {
                printf("  applying IR to mono file\n");

                if (!(xm = cfg->sMapping.add_n(ir->channels())))
                {
                    fprintf(stderr, "Not enough memory for generating mapping data\n");
                    return STATUS_NO_MEM;
                }

                // Simple 1:n mapping
                for (size_t i=0; i<ir->channels(); ++i)
                {
                    xm[i].in    = 0;
                    xm[i].out   = i;
                    xm[i].ir    = i;
                    xm[i].gain  = 1.0f;
                }
            }
            else if (ir->channels() == 1)
            {
                printf("  applying mono IR to file\n");

                if (!(xm = cfg->sMapping.add_n(in->channels())))
                {
                    fprintf(stderr, "Not enough memory for generating mapping data\n");
                    return STATUS_NO_MEM;
                }

                // Simple n:1 mapping
                for (size_t i=0; i<in->channels(); ++i)
                {
                    xm[i].in    = i;
                    xm[i].out   = 0;
                    xm[i].ir    = 0;
                    xm[i].gain  = 1.0f;
                }
            }
            else
            {
                fprintf(stderr, "Untypical configuration of input and IR file, need output mapping to be explicitly specified\n");
                return STATUS_BAD_ARGUMENTS;
            }
        }
        else
            printf("  applying mapping-defined convolution\n");

        // Estimate number of output channels
        size_t out_channels = 0;
        for (size_t i=0, n=cfg->sMapping.size(); i<n; ++i)
        {
            const mapping_t *m = cfg->sMapping.uget(i);
            if (out_channels <= m->out)
                out_channels    = m->out + 1;
        }

        // Resize the output sample
        if (!out->resize(out_channels, out_length, out_length))
        {
            fprintf(stderr, "Not enough memory for output data\n");
            return STATUS_NO_MEM;
        }

        // Form the 'Dry' sound according to the mapping settings
        for (size_t oc=0; oc<out_channels; ++oc)
        {
            float *dptr = out->channel(oc);

            for (size_t ic=0; ic<in->channels(); ++ic)
            {
                if (!contains_mapping(cfg, oc, ic))
                    continue;

                // Copy 'dry' sound with adjusted gain
                const float *sptr = in->channel(ic);
                dsp::fmadd_k3(&dptr[latency], sptr, g_dry, in->length());
            }
        }

        // Now apply mapping function
        for (size_t i=0, n=cfg->sMapping.size(); i<n; ++i)
        {
            const mapping_t *m = cfg->sMapping.uget(i);

            // Output information
            printf("  convolving IN channel %d with IR channel %d to OUT channel %d at %.2f dB\n",
                int(m->in), int(m->ir), int(m->out), dspu::gain_to_db(m->gain)
            );

            // Perform convolution
            float gain = m->gain + cfg->fWet;
            if (gain >= MIN_GAIN)
                convolve(out, in, ir, m->out, m->in, m->ir, predelay, dspu::db_to_gain(gain));
        }

        return STATUS_OK;
    }

    status_t apply_fades(dspu::Sample *s, const config_t *cfg)
    {
        ssize_t head_cut = dspu::millis_to_samples(s->sample_rate(), cfg->fHeadCut);
        if (head_cut < 0)
        {
            fprintf(stderr, "Negative head cut value, can not proceed\n");
            return STATUS_BAD_ARGUMENTS;
        }

        ssize_t tail_cut = dspu::millis_to_samples(s->sample_rate(), cfg->fTailCut);
        if (tail_cut < 0)
        {
            fprintf(stderr, "Negative tail cut value, can not proceed\n");
            return STATUS_BAD_ARGUMENTS;
        }

        ssize_t fade_in = dspu::millis_to_samples(s->sample_rate(), cfg->fFadeIn);
        if (fade_in < 0)
        {
            fprintf(stderr, "Negative fade in value, can not proceed\n");
            return STATUS_BAD_ARGUMENTS;
        }

        ssize_t fade_out = dspu::millis_to_samples(s->sample_rate(), cfg->fFadeOut);
        if (fade_in < 0)
        {
            fprintf(stderr, "Negative fade out value, can not proceed\n");
            return STATUS_BAD_ARGUMENTS;
        }

        return cut_sample(s, head_cut, tail_cut, fade_in, fade_out);
    }

    int main(int argc, const char **argv)
    {
        config_t cfg;
        status_t res;
        size_t latency;
        dspu::Sample in, out, ir;

        // Parse configuration
        if ((res = parse_cmdline(&cfg, argc, argv)) != STATUS_OK)
            return (res == STATUS_SKIP) ? STATUS_OK : res;

        // Load audio file
        if ((res = load_audio_file(&in, cfg.nSampleRate, &cfg.sInFile)) != STATUS_OK)
            return res;
        cfg.nSampleRate = in.sample_rate();

        // Load IR file
        if ((res = load_audio_file(&ir, cfg.nSampleRate, &cfg.sIRFile)) != STATUS_OK)
            return res;

        // Apply fades to the IR file
        LSPString path;
        path.set_ascii("tmp/ir-no-fades.wav");
        save_audio_file(&ir, &path);
        if ((res = apply_fades(&ir, &cfg)) != STATUS_OK)
            return res;
        path.set_ascii("tmp/ir-with-fades.wav");
        save_audio_file(&ir, &path);

        // Apply filters to the IR
        printf("  applying IR filters\n");
        if ((res = apply_equalizer(&latency, &ir, &cfg)) != STATUS_OK)
            return res;

        // Convolve the input file with the IR and store to output file
        if ((res = convolve_data(&out, &in, &ir, &cfg, latency)) != STATUS_OK)
            return res;

        // Apply mid/side balance
        float mid_g  = (cfg.fMid  >= MIN_GAIN) ? dspu::db_to_gain(cfg.fMid)  : 0.0f;
        float side_g = (cfg.fSide >= MIN_GAIN) ? dspu::db_to_gain(cfg.fSide) : 0.0f;
        apply_mid_side(&out, mid_g, side_g);

        // Export the processed audio file
        out.set_sample_rate(in.sample_rate());
        if ((res = save_audio_file(&out, &cfg.sOutFile)) != STATUS_OK)
            return res;

        return STATUS_OK;
    }
}

