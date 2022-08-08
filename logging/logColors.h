#pragma once

// extensions start
/*! \brief define a simple ansi console colors struct
 *
 * The contained values correspond to ansi color definitions
 * However, the colors are only work on platforms that are
 * able to interpret these ansi escape sequences, like
 * [L|U]nix
 */
struct LogC {
    enum C {
        reset = 0,
        black=30,
        red=31,
        green=32,
        yellow=33,
        blue = 34,
        magenta=35,
        cyan=36,
        white=37
    };
};

#define GENERATE_OPERATOR_CONTENT   \
        unsigned char tmp=Base::getBase();  \
        *this << ::logging::log::dec << "\033[" << static_cast<unsigned short>(l) << 'm';  \
        Base::setBase(tmp);

/*! \brief Defines the extended output type that is capable to interpret
 *         the colors and produce the correct escape sequences.
 */
template < typename Base>
struct CC : public Base {
    /*! \brief catch color type and produce the correct escape sequence
     */
    CC& operator << (const LogC::C l) {
        GENERATE_OPERATOR_CONTENT
        return *this;
    }

    /*! \brief forward all unknown types to the base output type for
     *         further processing.
     */
    template<typename T>
    CC& operator << (const T &t) {
        Base::operator<<(t);
        return *this;
    }
};
// extensions end