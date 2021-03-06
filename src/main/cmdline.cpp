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

    typedef struct cfg_flag_t
    {
        const char     *name;
        ssize_t         value;
    } cfg_flag_t;

    static const option_t options[] =
    {
        { "-dg",  "--dry-gain",         false,     "Dry gain (in dB) - the amount of unprocessed signal"    },
        { "-fi",  "--fade-in",          false,     "Fade in of the IR file (in milliseconds)"               },
        { "-fo",  "--fade-out",         false,     "Fade out of the IR file (in milliseconds)"              },
        { "-hc",  "--head-cut",         false,     "Head cut of the IR file (in milliseconds)"              },
        { "-hp",  "--hi-pass",          false,     "High-pass filter parameters (--help for details)"       },
        { "-if",  "--in-file",          false,     "Input file"                                             },
        { "-ir",  "--ir-file",          false,     "Impulse response file"                                  },
        { "-lp",  "--low-pass",         false,     "Low-pass filter parameters (--help for details)"        },
        { "-m",   "--mapping",          false,     "IR convolution mapping in format: out:in:ir[:gain]"     },
        { "-mb",  "--mid-balance",      false,     "The amount of Middle part (in dB) in stereo signal"     },
        { "-n",   "--normalize",        false,     "Set normalization mode"                                 },
        { "-ng",  "--norm-gain",        false,     "Set normalization peak gain (in dB)"                    },
        { "-of",  "--out-file",         false,     "Output file"                                            },
        { "-pd",  "--predelay",         false,     "The amount of pre-delay added to the signal (in ms)"    },
        { "-sb",  "--side-balance",     false,     "The amount of Side part (in dB) in stereo signal"       },
        { "-sr",  "--srate",            false,     "Sample rate of output file"                             },
        { "-tc",  "--tail-cut",         false,     "Tail cut of the IR file (in milliseconds)"              },
        { "-tl",  "--trim-length",      true,      "Trim length of output file to match the input file"     },
        { "-wg",  "--wet-gain",         false,     "Wet gain (in dB) - the amount of processed signal"      },

        { NULL, NULL, false, NULL }
    };

    const cfg_flag_t normalize_flags[] =
    {
        { "none",   NORM_NONE   },
        { "above",  NORM_ABOVE  },
        { "below",  NORM_BELOW  },
        { "always", NORM_ALWAYS },
        { NULL,     0           }
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

    const cfg_flag_t *find_config_flag(const LSPString *s, const cfg_flag_t *flags)
    {
        for (size_t i=0; (flags != NULL) && (flags->name != NULL); ++i, ++flags)
        {
            if (s->equals_ascii_nocase(flags->name))
                return flags;
        }
        return NULL;
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

    status_t parse_cmdline_bool(bool *dst, const char *val, const char *parameter)
    {
        LSPString in;
        if (!in.set_native(val))
        {
            fprintf(stderr, "Out of memory\n");
            return STATUS_NO_MEM;
        }

        io::InStringSequence is(&in);
        expr::Tokenizer t(&is);
        bool bvalue;

        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE: bvalue = t.int_value(); break;
            case expr::TT_FVALUE: bvalue = t.float_value() >= 0.5f; break;
            case expr::TT_TRUE: bvalue = true; break;
            case expr::TT_FALSE: bvalue = false; break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }

        if (t.get_token(expr::TF_GET) != expr::TT_EOF)
        {
            fprintf(stderr, "Bad '%s' value\n", parameter);
            return STATUS_INVALID_VALUE;
        }

        *dst = bvalue;

        return STATUS_OK;
    }

    status_t parse_cmdline_enum(ssize_t *dst, const char *parameter, const char *val, const cfg_flag_t *flags)
    {
        LSPString in;
        if (!in.set_native(val))
        {
            fprintf(stderr, "Out of memory\n");
            return STATUS_NO_MEM;
        }

        io::InStringSequence is(&in);
        expr::Tokenizer t(&is);
        const cfg_flag_t *flag = NULL;

        switch (t.get_token(expr::TF_GET | expr::TF_XKEYWORDS))
        {
            case expr::TT_BAREWORD:
                if ((flag = find_config_flag(t.text_value(), flags)) == NULL)
                {
                    fprintf(stderr, "Bad '%s' value\n", parameter);
                    return STATUS_BAD_FORMAT;
                }
                break;

            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_BAD_FORMAT;
        }

        if (t.get_token(expr::TF_GET) != expr::TT_EOF)
        {
            fprintf(stderr, "Bad '%s' value\n", parameter);
            return STATUS_INVALID_VALUE;
        }

        *dst = flag->value;

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

    status_t parse_filter_params(dspu::filter_params_t *fp, const char *val, const char *parameter, bool lowpass)
    {
        if ((!strcmp(val, "--help")) || (!strcmp(val, "-h")))
        {
            printf("Filter parameters format: <type>:<slope>:<freq>[:<q>]\n");
            printf("  type  - filter type, one of: RLC_BT, RLC_MT, BWC_BT, BWC_MT, LRX_BT, LRX_MT, APO_DR\n");
            printf("  slope - filter slope (1 .. 4)\n");
            printf("  freq  - filter cut-off frequency (10 .. 24000)\n");
            printf("  q     - quality factor of filter (0 .. 100)\n");
            printf("\n");

            return STATUS_SKIP;
        }

        LSPString in;
        if (!in.set_native(val))
        {
            fprintf(stderr, "Out of memory\n");
            return STATUS_NO_MEM;
        }

        io::InStringSequence is(&in);
        expr::Tokenizer t(&is);

        // 'type'
        switch (t.get_token(expr::TF_GET | expr::TF_XKEYWORDS | expr::TF_BAREWORD))
        {
            case expr::TT_BAREWORD:
            {
                const LSPString *ft = t.text_value();
                if (ft->equals_ascii("RLC_BT"))
                {
                    fp->nSlope      = 2;
                    fp->nType       = (lowpass) ? dspu::FLT_BT_RLC_LOPASS : dspu::FLT_BT_RLC_HIPASS;
                }
                else if (ft->equals_ascii("RLC_MT"))
                {
                    fp->nSlope      = 2;
                    fp->nType       = (lowpass) ? dspu::FLT_MT_RLC_LOPASS : dspu::FLT_MT_RLC_HIPASS;
                }
                else if (ft->equals_ascii("BWC_BT"))
                {
                    fp->nSlope      = 2;
                    fp->nType       = (lowpass) ? dspu::FLT_BT_BWC_LOPASS : dspu::FLT_BT_BWC_HIPASS;
                }
                else if (ft->equals_ascii("BWC_MT"))
                {
                    fp->nSlope      = 2;
                    fp->nType       = (lowpass) ? dspu::FLT_MT_BWC_LOPASS : dspu::FLT_MT_BWC_HIPASS;
                }
                else if (ft->equals_ascii("LRX_BT"))
                {
                    fp->nSlope      = 1;
                    fp->nType       = (lowpass) ? dspu::FLT_BT_LRX_LOPASS : dspu::FLT_BT_LRX_HIPASS;
                }
                else if (ft->equals_ascii("LRX_MT"))
                {
                    fp->nSlope      = 1;
                    fp->nType       = (lowpass) ? dspu::FLT_MT_LRX_LOPASS : dspu::FLT_MT_LRX_HIPASS;
                }
                else if (ft->equals_ascii("APO_DR"))
                {
                    fp->nSlope      = 1;
                    fp->nType       = (lowpass) ? dspu::FLT_DR_APO_LOPASS : dspu::FLT_DR_APO_HIPASS;
                }
                else
                {
                    fprintf(stderr, "Unknown filter type: %s\n", ft->get_native());
                    return STATUS_BAD_FORMAT;
                }

                break;
            }
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }

        // 'slope'
        if (t.get_token(expr::TF_GET) != expr::TT_COLON)
        {
            fprintf(stderr, "Bad '%s' value\n", parameter);
            return STATUS_INVALID_VALUE;
        }
        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE:
            {
                ssize_t slope = t.int_value();
                if ((slope <= 0) || (slope > 4))
                {
                    fprintf(stderr, "Invalid slope value: %ld\n", long(slope));
                    return STATUS_BAD_FORMAT;
                }
                fp->nSlope *= slope;
                break;
            }
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }

        // 'freq'
        if (t.get_token(expr::TF_GET) != expr::TT_COLON)
        {
            fprintf(stderr, "Bad '%s' value\n", parameter);
            return STATUS_INVALID_VALUE;
        }
        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE: fp->fFreq = t.int_value(); break;
            case expr::TT_FVALUE: fp->fFreq = t.float_value(); break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }
        if ((fp->fFreq < 10.0f) || (fp->fFreq > 24000.0f))
        {
            fprintf(stderr, "Invalid cut-off frequency %d for %s\n", int(fp->fFreq), parameter);
            return STATUS_INVALID_VALUE;
        }

        // 'q'
        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_EOF:
                fp->fQuality    = 0.0f;
                return STATUS_OK;
            case expr::TT_COLON:
                break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }
        switch (t.get_token(expr::TF_GET))
        {
            case expr::TT_IVALUE: fp->fQuality = t.int_value(); break;
            case expr::TT_FVALUE: fp->fQuality = t.float_value(); break;
            default:
                fprintf(stderr, "Bad '%s' value\n", parameter);
                return STATUS_INVALID_VALUE;
        }
        if ((fp->fQuality < 0.0f) || (fp->fQuality > 100.0f))
        {
            fprintf(stderr, "Invalid quality factor %f for %s\n", fp->fQuality, parameter);
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
                        if ((!p->s_flag) && (i >= argc))
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
        if ((val = options.get("--low-pass")) != NULL)
        {
            if ((res = parse_filter_params(&cfg->sLPF, val, "low-pass filter", true)) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--hi-pass")) != NULL)
        {
            if ((res = parse_filter_params(&cfg->sHPF, val, "high-pass filter", false)) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--head-cut")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fHeadCut, val, "head cut")) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--tail-cut")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fTailCut, val, "tail cut")) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--fade-in")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fFadeIn, val, "fade in")) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--fade-out")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fFadeOut, val, "fade out")) != STATUS_OK)
                return res;
        }
        if (options.contains("--trim-length"))
            cfg->bTrim  = true;
        if ((val = options.get("--norm-gain")) != NULL)
        {
            if ((res = parse_cmdline_float(&cfg->fNormGain, val, "norm-gain")) != STATUS_OK)
                return res;
        }
        if ((val = options.get("--normalize")) != NULL)
        {
            if ((res = parse_cmdline_enum(&cfg->nNormalize, "normalize", val, normalize_flags)) != STATUS_OK)
                return res;
        }


        // Mandatory parameters
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

        return STATUS_OK;
    }
}



