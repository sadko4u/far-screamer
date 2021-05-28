/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of far-screamer
 * Created on: 28 мая 2021 г.
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

#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/lltl/parray.h>

namespace far_screamer
{
    int main(int argc, const char **argv);
}

MTEST_BEGIN("far_screamer", main)

    MTEST_MAIN
    {
        lltl::parray<char> args;
        args.add(const_cast<char *>(full_name()));

        for (ssize_t i=0; i < argc; ++i)
            args.add(const_cast<char *>(argv[i]));

        far_screamer::main(args.size(), const_cast<const char **>(args.array()));
    }

MTEST_END


