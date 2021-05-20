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

#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/io/InStringSequence.h>
#include <lsp-plug.in/expr/Tokenizer.h>

#include <private/config.h>
#include <private/cmdline.h>

namespace far_screamer
{
    using namespace lsp;

    typedef struct option_t
    {
        const char *s_short;
        const char *s_long;
        bool        s_flag;
        const char *s_desc;
    } option_t;

    static const option_t options[] =
    {
        { "-dg",  "--dry-gain",         false,     "Dry gain (in dB) - the amount of unprocessed signal"    },
        { "-if",  "--in-file",          false,     "Input file"                                             },
        { "-ir",  "--ir-file",          false,     "Impulse response file"                                  },
        { "-m",   "--mapping",          false,     "IR convolution mapping in format: out:in:ir[:gain]"     },
        { "-mb",  "--mid-balance",      false,     "The amount of Middle part (in dB) in stereo signal"     },
        { "-of",  "--out-file",         false,     "Output file"                                            },
        { "-pd",  "--predelay",         false,     "The amount of pre-delay added to the signal (in ms)"    },
        { "-sb",  "--side-balance",     false,     "The amount of Side part (in dB) in stereo signal"       },
        { "-sr",  "--srate",            false,     "Sample rate of output file"                             },
        { "-wg",  "--wet-gain",         false,     "Wet gain (in dB) - the amount of processed signal"      },

        { NULL, NULL, false, NULL }
    };

    status_t print_usage(const char *name, bool fail)
    {
        LSPString buf, fmt;
        size_t maxlen = 0;

        // Estimate maximum parameter size
        for (const option_t *p = far_screamer::options; p->s_short != NULL; ++p)
        {
            buf.fmt_ascii("%s, %s", p->s_short, p->s_long);
            maxlen  = lsp_max(buf.length(), maxlen);
        }
        fmt.fmt_ascii("  %%-%ds    %%s\n", int(maxlen));

        // Output usage
        printf("usage: %s [arguments]\n", name);
        printf("available arguments:\n");
        for (const option_t *p = far_screamer::options; p->s_short != NULL; ++p)
        {
            buf.fmt_ascii("%s, %s", p->s_short, p->s_long);
            printf(fmt.get_native(), buf.get_native(), p->s_desc);
        }

        return (fail) ? STATUS_BAD_ARGUMENTS : STATUS_SKIP;
    }

    status_t parse_cmdline_int(ssize_t *dst, const char *val, const char *parameter)
    {
        LSPString in;
        if (!in.set_native(val))
            return STATUS_NO_MEM;

        io::InStringSequence is(&in);
        expr::Tokenizer t(&is);
        ssize_t ivalue;

        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE: ivalue = t.int_value(); break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }

        if (t.get_token(expr::TF_GET) != expr::TT_EOF)
        {
            fprintf(stderr, "Bad '%s' value\n", parameter);
            return STATUS_INVALID_VALUE;
        }

        *dst = ivalue;

        return STATUS_OK;
    }

    status_t parse_cmdline_float(float *dst, const char *val, const char *parameter)
    {
        LSPString in;
        if (!in.set_native(val))
        {
            fprintf(stderr, "Out of memory\n");
            return STATUS_NO_MEM;
        }

        io::InStringSequence is(&in);
        expr::Tokenizer t(&is);
        float fvalue;

        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE: fvalue = t.int_value(); break;
            case expr::TT_FVALUE: fvalue = t.float_value(); break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }

        if (t.get_token(expr::TF_GET) != expr::TT_EOF)
        {
            fprintf(stderr, "Bad '%s' value\n", parameter);
            return STATUS_INVALID_VALUE;
        }

        *dst = fvalue;

        return STATUS_OK;
    }

    status_t parse_cmdline_mapping(mapping_t *dst, const char *val, const char *parameter)
    {
        LSPString in;
        if (!in.set_native(val))
        {
            fprintf(stderr, "Out of memory\n");
            return STATUS_NO_MEM;
        }

        io::InStringSequence is(&in);
        expr::Tokenizer t(&is);

        // 'out'
        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE: dst->out = t.int_value(); break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }

        // 'in'
        if (t.get_token(expr::TF_GET) != expr::TT_COLON)
        {
            fprintf(stderr, "Bad '%s' value\n", parameter);
            return STATUS_INVALID_VALUE;
        }
        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE: dst->in = t.int_value(); break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }

        // 'ir'
        if (t.get_token(expr::TF_GET) != expr::TT_COLON)
        {
            fprintf(stderr, "Bad '%s' value\n", parameter);
            return STATUS_INVALID_VALUE;
        }
        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE: dst->ir = t.int_value(); break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }

        // 'gain'
        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_EOF:
                dst->gain           = 0.0f;
                return STATUS_OK;
            case expr::TT_COLON:
                break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }
        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE: dst->gain = t.int_value(); break;
            case expr::TT_FVALUE: dst->gain = t.float_value(); break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }

        return STATUS_OK;
    }

    status_t parse_cmdline(config_t *cfg, int argc, const char **argv)
    {
        status_t res;
        const char *cmd = argv[0], *val;
        lltl::pphash<char, char> options;

        // Read options to hash
        for (int i=1; i < argc; )
        {
            const char *opt = argv[i++];

            // Aliases
            for (const option_t *p = far_screamer::options; p->s_short != NULL; ++p)
                if (!strcmp(opt, p->s_short))
                {
                    opt = p->s_long;
                    break;
                }

            // Check arguments
            const char *xopt = opt;
            if (!strcmp(opt, "--help"))
                return print_usage(cmd, false);
            else if ((opt[0] != '-') || (opt[1] != '-'))
            {
                fprintf(stderr, "Invalid argument: %s\n", opt);
                return STATUS_BAD_ARGUMENTS;
            }
            else
                xopt = opt;

            // Parse options
            bool found = false;
            if ((!strcmp(xopt, "-m")) || (!strcmp(xopt, "--mapping")))
            {
                if (i >= argc)
                {
                    fprintf(stderr, "Not defined value for option: %s\n", opt);
                    return STATUS_BAD_ARGUMENTS;
                }

                val = argv[i++];

                // Add child file to list of children
                mapping_t *m = cfg->sMapping.add();
                if (m == NULL)
                {
                    fprintf(stderr, "Not enough memory\n");
                    return STATUS_NO_MEM;
                }

                // Pase mapping
                if ((res = parse_cmdline_mapping(m, val, xopt)) != STATUS_OK)
                    return res;

                found       = true;
            }
            else
            {
                for (const option_t *p = far_screamer::options; p->s_short != NULL; ++p)
                    if (!strcmp(xopt, p->s_long))
                    {
                        if (i >= argc)
                        {
                            fprintf(stderr, "Not defined value for option: %s\n", opt);
                            return STATUS_BAD_ARGUMENTS;
                        }

                        // Add option to settings map
                        val = (p->s_flag) ? NULL : argv[i++];
                        if (options.exists(xopt))
                        {
                            fprintf(stderr, "Duplicate option: %s\n", opt);
                            return STATUS_BAD_ARGUMENTS;
                        }

                        // Try to create option
                        if (!options.create(xopt, const_cast<char *>(val)))
                        {
                            fprintf(stderr, "Not enough memory\n");
                            return STATUS_NO_MEM;
                        }

                        found       = true;
                        break;
                    }
            }

            if (!found)
            {
                fprintf(stderr, "Invalid option: %s\n", opt);
                return STATUS_BAD_ARGUMENTS;
            }
        }

        // Parse other parameters
        if ((val = options.get("--srate")) != NULL)
        {
            if ((res = parse_cmdline_int(&cfg->nSampleRate, val, "sample rate")) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--in-file")) != NULL)
            cfg->sInFile.set_native(val);
        else
        {
            fprintf(stderr, "Input file name required\n");
            return STATUS_BAD_ARGUMENTS;
        }

        if ((val = options.get("--out-file")) != NULL)
            cfg->sOutFile.set_native(val);
        else
        {
            fprintf(stderr, "Output file name required\n");
            return STATUS_BAD_ARGUMENTS;
        }

        if ((val = options.get("--ir-file")) != NULL)
            cfg->sIRFile.set_native(val);
        else
        {
            fprintf(stderr, "Impulse response file name required\n");
            return STATUS_BAD_ARGUMENTS;
        }

        if ((val = options.get("--dry-gain")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fDry, val, "dry gain")) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--wet-gain")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fWet, val, "wet gain")) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--mid-balance")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fMid, val, "middle balance")) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--side-balance")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fSide, val, "side balance")) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--predelay")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fPreDelay, val, "predelay")) != STATUS_OK)
                return res;
        }


        return STATUS_OK;
    }
}



