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
        fDry                = 0.0f;
        fWet                = 0.0f;
        fMid                = 0.0f;
        fSide               = 0.0f;
        fPreDelay           = 0;
    }

    config_t::~config_t()
    {
        clear();
    }

    void config_t::clear()
    {
        nSampleRate         = -1;
        fDry                = 0.0f;
        fWet                = 0.0f;
        fMid                = 0.0f;
        fSide               = 0.0f;
        fPreDelay           = 0;
        sInFile.clear();
        sOutFile.clear();
        sIRFile.clear();
        sMapping.flush();
    }
}



