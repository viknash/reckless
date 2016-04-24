#include <reckless/template_formatter.hpp>
#include <reckless/ntoa.hpp>

#include <cstdio>
#include <cstring>
#include <cstdint>

namespace reckless {
namespace {

    unsigned atou(char const*& s)
    {
        unsigned v = *s - '0';
        ++s;
        if(!isdigit(*s))
            return v;
        v = 10*v + *s - '0';
        ++s;
        while(isdigit(*s)) {
            v = 10*v + *s - '0';
            ++s;
        }
        return v;
    }

    bool isdigit(char c)
    {
        return c >= '0' && c <= '9';
    }
    
    // TODO for people writing custom format functions, it would be nice to
    // have access to this. Also being able to just skip past the conversion
    // specification so they can check the format character.
    char const* parse_conversion_specification(conversion_specification* pspec, char const* pformat)
    {
        bool left_justify = false;
        bool alternative_form = false;
        bool show_plus_sign = false;
        bool blank_sign = false;
        bool pad_with_zeroes = false;
        while(true) {
            char flag = *pformat;
            if(flag == '-')
                left_justify = true;
            else if(flag == '+')
                show_plus_sign = true;
            else if (flag == ' ')
                blank_sign = true;
            else if(flag == '#')
                alternative_form = true;
            else if(flag == '0')
                pad_with_zeroes = true;
            else
                break;
            ++pformat;
        }
        
        pspec->minimum_field_width = isdigit(*pformat)? atou(pformat) : 0;
        if(*pformat == '.') {
            ++pformat;
            pspec->precision = isdigit(*pformat)? atou(pformat) : UNSPECIFIED_PRECISION;
        } else {
            pspec->precision = UNSPECIFIED_PRECISION;
        }
        if(show_plus_sign)
            pspec->plus_sign = '+';
        else if(blank_sign)
            pspec->plus_sign = ' ';
        else
            pspec->plus_sign = 0;
        
        pspec->left_justify = left_justify;
        pspec->alternative_form = alternative_form;
        pspec->pad_with_zeroes = pad_with_zeroes;
        pspec->uppercase = false;
        
        return pformat;
    }
        
    template <typename T>
    char const* generic_format_int(output_buffer* pbuffer, char const* pformat, T v)
    {
        conversion_specification spec;
        pformat = parse_conversion_specification(&spec, pformat);
        char f = *pformat;
        if(f == 'd') {
            itoa_base10(pbuffer, v, spec);
            return pformat + 1;
        } else if(f == 'x') {
            spec.uppercase = false;
            itoa_base16(pbuffer, v, spec);
            return pformat + 1;
        } else if(f == 'X') {
            spec.uppercase = true;
            itoa_base16(pbuffer, v, spec);
            return pformat + 1;
        } else if(f == 'b') {
            // FIXME
            return nullptr;
        } else {
            return nullptr;
        }
    }

    template <typename T>
    char const* generic_format_float(output_buffer* pbuffer, char const* pformat, T v)
    {
        conversion_specification cs;
        pformat = parse_conversion_specification(&cs, pformat);
        char f = *pformat;
        if(f != 'f')
            return nullptr;
        
        ftoa_base10_f(pbuffer, v, cs);
        return pformat + 1;
    }

    template <typename T>
    char const* generic_format_char(output_buffer* pbuffer, char const* pformat, T v)
    {
        char f = *pformat;
        if(f == 's') {
            char* p = pbuffer->reserve(1);
            *p = static_cast<char>(v);
            pbuffer->commit(1);
            return pformat + 1;
        } else {
            return generic_format_int(pbuffer, pformat, static_cast<int>(v));
        }
    }

}   // anonymous namespace

char const* format(output_buffer* pbuffer, char const* pformat, char v)
{
    return generic_format_char(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, signed char v)
{
    return generic_format_char(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, unsigned char v)
{
    return generic_format_char(pbuffer, pformat, v);
}

//char const* format(output_buffer* pbuffer, char const* pformat, wchar_t v);
//char const* format(output_buffer* pbuffer, char const* pformat, char16_t v);
//char const* format(output_buffer* pbuffer, char const* pformat, char32_t v);

char const* format(output_buffer* pbuffer, char const* pformat, short v)
{
    return generic_format_int(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, unsigned short v)
{
    return generic_format_int(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, int v)
{
    return generic_format_int(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, unsigned int v)
{
    return generic_format_int(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, long v)
{
    return generic_format_int(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, unsigned long v)
{
    return generic_format_int(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, long long v)
{
    return generic_format_int(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, unsigned long long v)
{
    return generic_format_int(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, float v)
{
    return generic_format_float(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, double v)
{
    return generic_format_float(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, long double v)
{
    return generic_format_float(pbuffer, pformat, v);
}

char const* format(output_buffer* pbuffer, char const* pformat, char const* v)
{
    char c = *pformat;
    if(c =='s') {
        auto len = std::strlen(v);
        char* p = pbuffer->reserve(len);
        std::memcpy(p, v, len);
        pbuffer->commit(len);
    } else if(c == 'p') {
        conversion_specification cs;
        cs.minimum_field_width = 0;
        cs.precision = 1;
        cs.plus_sign = 0;
        cs.left_justify = false;
        cs.alternative_form = true;
        cs.pad_with_zeroes = false;
        cs.uppercase = false;
        itoa_base16(pbuffer, reinterpret_cast<std::uintptr_t>(v), cs);
    } else {
        return nullptr;
    }

    return pformat + 1;
}

char const* format(output_buffer* pbuffer, char const* pformat, std::string const& v)
{
    if(*pformat != 's')
        return nullptr;
    auto len = v.size();
    char* p = pbuffer->reserve(len);
    std::memcpy(p, v.data(), len);
    pbuffer->commit(len);
    return pformat + 1;
}

char const* format(output_buffer* pbuffer, char const* pformat, void const* p)
{
    char c = *pformat;
    if(c != 'p' && c !='s')
        return nullptr;
    
    conversion_specification cs;
    cs.minimum_field_width = 0;
    cs.precision = 1;
    cs.plus_sign = 0;
    cs.left_justify = false;
    cs.alternative_form = true;
    cs.pad_with_zeroes = false;
    cs.uppercase = false;
    
    itoa_base16(pbuffer, reinterpret_cast<std::uintptr_t>(p), cs);
    return pformat+1;
}

void template_formatter::append_percent(output_buffer* pbuffer)
{
    auto p = pbuffer->reserve(1u);
    *p = '%';
    pbuffer->commit(1u);
}

//http://www.scs.stanford.edu/histar/src/pkg/uclibc/libc/string/generic/strchrnul.c
/* Find the first occurrence of C in S or the final NUL byte.  */
char *strchrnul (const char *s, int c_in)
{
  const unsigned char *char_ptr;
  const unsigned long int *longword_ptr;
  unsigned long int longword, magic_bits, charmask;
  unsigned char c;

  c = (unsigned char) c_in;

  /* Handle the first few characters by reading one character at a time.
     Do this until CHAR_PTR is aligned on a longword boundary.  */
  for (char_ptr = (const unsigned char *) s;
       ((size_t) char_ptr & (sizeof (longword) - 1)) != 0;
       ++char_ptr)
    if (*char_ptr == c || *char_ptr == '\0')
      return (char *) char_ptr;

  /* All these elucidatory comments refer to 4-byte longwords,
     but the theory applies equally well to 8-byte longwords.  */

  longword_ptr = (unsigned long int *) char_ptr;

  /* Bits 31, 24, 16, and 8 of this number are zero.  Call these bits
     the "holes."  Note that there is a hole just to the left of
     each byte, with an extra at the end:

     bits:  01111110 11111110 11111110 11111111
     bytes: AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD

     The 1-bits make sure that carries propagate to the next 0-bit.
     The 0-bits provide holes for carries to fall into.  */
  switch (sizeof (longword))
    {
    case 4: magic_bits = 0x7efefeffL; break;
    case 8: magic_bits = ((0x7efefefeL << 16) << 16) | 0xfefefeffL; break;
    default:
      abort ();
    }

  /* Set up a longword, each of whose bytes is C.  */
  charmask = c | (c << 8);
  charmask |= charmask << 16;
  if (sizeof (longword) > 4)
    /* Do the shift in two steps to avoid a warning if long has 32 bits.  */
    charmask |= (charmask << 16) << 16;
  if (sizeof (longword) > 8)
    abort ();

  /* Instead of the traditional loop which tests each character,
     we will test a longword at a time.  The tricky part is testing
     if *any of the four* bytes in the longword in question are zero.  */
  for (;;)
    {
      /* We tentatively exit the loop if adding MAGIC_BITS to
	 LONGWORD fails to change any of the hole bits of LONGWORD.

	 1) Is this safe?  Will it catch all the zero bytes?
	 Suppose there is a byte with all zeros.  Any carry bits
	 propagating from its left will fall into the hole at its
	 least significant bit and stop.  Since there will be no
	 carry from its most significant bit, the LSB of the
	 byte to the left will be unchanged, and the zero will be
	 detected.

	 2) Is this worthwhile?  Will it ignore everything except
	 zero bytes?  Suppose every byte of LONGWORD has a bit set
	 somewhere.  There will be a carry into bit 8.  If bit 8
	 is set, this will carry into bit 16.  If bit 8 is clear,
	 one of bits 9-15 must be set, so there will be a carry
	 into bit 16.  Similarly, there will be a carry into bit
	 24.  If one of bits 24-30 is set, there will be a carry
	 into bit 31, so all of the hole bits will be changed.

	 The one misfire occurs when bits 24-30 are clear and bit
	 31 is set; in this case, the hole at bit 31 is not
	 changed.  If we had access to the processor carry flag,
	 we could close this loophole by putting the fourth hole
	 at bit 32!

	 So it ignores everything except 128's, when they're aligned
	 properly.

	 3) But wait!  Aren't we looking for C as well as zero?
	 Good point.  So what we do is XOR LONGWORD with a longword,
	 each of whose bytes is C.  This turns each byte that is C
	 into a zero.  */

      longword = *longword_ptr++;

      /* Add MAGIC_BITS to LONGWORD.  */
      if ((((longword + magic_bits)

	    /* Set those bits that were unchanged by the addition.  */
	    ^ ~longword)

	   /* Look at only the hole bits.  If any of the hole bits
	      are unchanged, most likely one of the bytes was a
	      zero.  */
	   & ~magic_bits) != 0 ||

	  /* That caught zeroes.  Now test for C.  */
	  ((((longword ^ charmask) + magic_bits) ^ ~(longword ^ charmask))
	   & ~magic_bits) != 0)
	{
	  /* Which of the bytes was C or zero?
	     If none of them were, it was a misfire; continue the search.  */

	  const unsigned char *cp = (const unsigned char *) (longword_ptr - 1);

	  if (*cp == c || *cp == '\0')
	    return (char *) cp;
	  if (*++cp == c || *cp == '\0')
	    return (char *) cp;
	  if (*++cp == c || *cp == '\0')
	    return (char *) cp;
	  if (*++cp == c || *cp == '\0')
	    return (char *) cp;
	  if (sizeof (longword) > 4)
	    {
	      if (*++cp == c || *cp == '\0')
		return (char *) cp;
	      if (*++cp == c || *cp == '\0')
		return (char *) cp;
	      if (*++cp == c || *cp == '\0')
		return (char *) cp;
	      if (*++cp == c || *cp == '\0')
		return (char *) cp;
	    }
	}
    }

  /* This should never happen.  */
  return NULL;
}

char const* template_formatter::next_specifier(output_buffer* pbuffer,
        char const* pformat)
{
//#ifdef _GNU_SOURCE
    while(true) {
        char const* pspecifier = strchrnul(pformat, '%');
        
        auto len = pspecifier - pformat;
        auto p = pbuffer->reserve(len);
        std::memcpy(p, pformat, len);
        pbuffer->commit(len);
        if(*pspecifier == '\0')
            return nullptr;

        pformat = pspecifier + 1;

        if(*pformat != '%')
            return pformat;

        // Found "%%". Add a single '%' and continue.
        ++pformat;
        append_percent(pbuffer);
    }
//#else
//    static_assert(false, "need replacement for strchrnul");
//endif
}

void template_formatter::format(output_buffer* pbuffer, char const* pformat)
{
    // There are no remaining arguments to format, so we will ignore additional
    // format specifications that might occur in the format string. However, we
    // still need to treat "%%" as "%". We'll iterate over on next_specifier
    // and when it finds a format specifier, we append a '%' and move on.
    while((pformat = next_specifier(pbuffer, pformat)) != nullptr)
        append_percent(pbuffer);
}

}   // namespace reckless
