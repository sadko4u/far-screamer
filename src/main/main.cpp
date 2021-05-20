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

#include <lsp-plug.in/dsp/dsp.h>

#include <private/tool.h>

#ifndef LSP_IDE_DEBUG
    int main(int argc, const char **argv)
    {
        using namespace lsp;

        int res;

        // Perform data processing
        dsp::context_t ctx;
        dsp::init();
        dsp::start(&ctx);
        res = far_screamer::main(argc, argv);
        dsp::finish(&ctx);

        return res;
    }
#endif /* LSP_IDE_DEBUG */



