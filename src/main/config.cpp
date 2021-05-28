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

#include <private/config.h>

namespace far_screamer
{
    config_t::config_t()
    {
        nSampleRate         = -1;
        fDry                = -1000.0f;
        fWet                = 0.0f;
        fMid                = 0.0f;
        fSide               = 0.0f;
        fPreDelay           = 0;
        fFadeIn             = 0.0f;
        fFadeOut            = 0.0f;
        fHeadCut            = 0.0f;
        fTailCut            = 0.0f;

        sLPF.nType          = dspu::FLT_NONE;
        sLPF.fFreq          = 0;
        sLPF.fFreq2         = 0;
        sLPF.fGain          = 1.0f;
        sLPF.nSlope         = 1;
        sLPF.fQuality       = 0.0f;

        sHPF.nType          = dspu::FLT_NONE;
        sHPF.fFreq          = 0;
        sHPF.fFreq2         = 0;
        sHPF.fGain          = 1.0f;
        sHPF.nSlope         = 1;
        sHPF.fQuality       = 0.0f;
    }

    config_t::~config_t()
    {
        clear();
    }

    void config_t::clear()
    {
        nSampleRate         = -1;
        fDry                = -1000.0f;
        fWet                = 0.0f;
        fMid                = 0.0f;
        fSide               = 0.0f;
        fPreDelay           = 0;
        fFadeIn             = 0.0f;
        fFadeOut            = 0.0f;
        fHeadCut            = 0.0f;
        fTailCut            = 0.0f;

        sLPF.nType          = dspu::FLT_NONE;
        sLPF.fFreq          = 0;
        sLPF.fFreq2         = 0;
        sLPF.fGain          = 1.0f;
        sLPF.nSlope         = 1;
        sLPF.fQuality       = 0.0f;

        sHPF.nType          = dspu::FLT_NONE;
        sHPF.fFreq          = 0;
        sHPF.fFreq2         = 0;
        sHPF.fGain          = 1.0f;
        sHPF.nSlope         = 1;
        sHPF.fQuality       = 0.0f;

        sInFile.clear();
        sOutFile.clear();
        sIRFile.clear();
        sMapping.flush();
    }
}



