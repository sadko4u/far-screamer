/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of far-screamer
 * Created on: 21 мая 2021 г.
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

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/helpers.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/dsp-units/filters/common.h>

#include <private/config.h>
#include <private/cmdline.h>

UTEST_BEGIN("far_screamer", cmdline)

    void validate_config(far_screamer::config_t *cfg)
    {
        LSPString key;

        // Validate root parameters
        UTEST_ASSERT(cfg->nSampleRate == 88200);
        UTEST_ASSERT(float_equals_absolute(cfg->fDry, 1.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->fWet, 2.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->fMid, -1.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->fSide, -2.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->fPreDelay, 10.5f));
        UTEST_ASSERT(cfg->sInFile.equals_ascii("in-file.wav"));
        UTEST_ASSERT(cfg->sOutFile.equals_ascii("out-file.wav"));
        UTEST_ASSERT(cfg->sIRFile.equals_ascii("ir-file.wav"));
        UTEST_ASSERT(float_equals_absolute(cfg->fFadeIn, 10.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->fFadeOut, 20.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->fHeadCut, 30.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->fTailCut, 40.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->fNormGain, -3.0f));
        UTEST_ASSERT(cfg->nNormalize == far_screamer::NORM_ALWAYS);
        UTEST_ASSERT(cfg->bTrim == true);

        // Check channel mapping
        UTEST_ASSERT(cfg->sMapping.size() == 3);
        ssize_t idx = 0;
        far_screamer::mapping_t *m;

        UTEST_ASSERT((m = cfg->sMapping.uget(idx++)) != NULL);
        UTEST_ASSERT(m->out == 0);
        UTEST_ASSERT(m->in == 1);
        UTEST_ASSERT(m->ir == 2);
        UTEST_ASSERT(float_equals_absolute(m->gain, 3.0f));

        UTEST_ASSERT((m = cfg->sMapping.uget(idx++)) != NULL);
        UTEST_ASSERT(m->out == 4);
        UTEST_ASSERT(m->in == 5);
        UTEST_ASSERT(m->ir == 6);
        UTEST_ASSERT(float_equals_absolute(m->gain, 7.0f));

        UTEST_ASSERT((m = cfg->sMapping.uget(idx++)) != NULL);
        UTEST_ASSERT(m->out == 8);
        UTEST_ASSERT(m->in == 9);
        UTEST_ASSERT(m->ir == 10);
        UTEST_ASSERT(float_equals_absolute(m->gain, 0.0f));

        // Check filters
        UTEST_ASSERT(cfg->sLPF.nType == dspu::FLT_BT_RLC_LOPASS);
        UTEST_ASSERT(cfg->sLPF.nSlope == 4); // slope is twice greater than specified
        UTEST_ASSERT(float_equals_absolute(cfg->sLPF.fFreq, 100.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->sLPF.fFreq2, 0.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->sLPF.fQuality, 0.0f));

        UTEST_ASSERT(cfg->sHPF.nType == dspu::FLT_MT_LRX_HIPASS);
        UTEST_ASSERT(cfg->sHPF.nSlope == 3);
        UTEST_ASSERT(float_equals_absolute(cfg->sHPF.fFreq, 10000.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->sHPF.fFreq2, 0.0f));
        UTEST_ASSERT(float_equals_absolute(cfg->sHPF.fQuality, 12.0f));
    }

    void parse_cmdline(far_screamer::config_t *cfg)
    {
        static const char *ext_argv[] =
        {
            "-sr",  "88200",
            "-dg",  "1.0",
            "-wg",  "2.0",
            "-mb",  "-1.0",
            "-sb",  "-2.0",
            "-pd",  "10.5",
            "-if",  "in-file.wav",
            "-of",  "out-file.wav",
            "-ir",  "ir-file.wav",
            "-fi",  "10",
            "-fo",  "20",
            "-hc",  "30",
            "-tc",  "40",
            "-m",   "0:1:2:3",
            "-m",   "4:5:6:7.0",
            "-m",   "8:9:10",
            "-lp",  "RLC_BT:2:100.0",
            "-hp",  "LRX_MT:3:10000.0:12",
            "-tl",  "true",
            "-ng",  "-3.0",
            "-n",   "ALWAYS",

            NULL
        };

        lltl::parray<char> argv;
        UTEST_ASSERT(argv.add(const_cast<char *>(full_name())));
        for (const char **pv = ext_argv; *pv != NULL; ++pv)
        {
            UTEST_ASSERT(argv.add(const_cast<char *>(*pv)));
        }

        status_t res = far_screamer::parse_cmdline(cfg, argv.size(), const_cast<const char **>(argv.array()));
        UTEST_ASSERT(res == STATUS_OK);
    }

    UTEST_MAIN
    {
        // Parse configuration from file and cmdline
        far_screamer::config_t cfg;
        parse_cmdline(&cfg);

        // Validate the final configuration
        validate_config(&cfg);
    }

UTEST_END


