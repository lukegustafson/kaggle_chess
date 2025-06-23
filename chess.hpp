/*
MIT License

Copyright (c) 2023 disservin, 2024-2025 Luke Gustafson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

THIS FILE IS AUTO GENERATED DO NOT CHANGE MANUALLY.

Original Version by disservin: 
https://github.com/Disservin/chess-library
VERSION: 0.6.77
*/

#ifndef CHESS_HPP
#define CHESS_HPP

#include <functional>
#include <utility>
#include <cstdint>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <string>
#include <string_view>

namespace chess {

class Color {
   public:
    enum class underlying : std::int8_t { WHITE = 0, BLACK = 1, NONE = -1 };

    constexpr Color() : color(underlying::NONE) {}
    constexpr Color(underlying c) : color(c) {
        assert(c == underlying::WHITE || c == underlying::BLACK || c == underlying::NONE);
    }
    constexpr Color(int c) : color(static_cast<underlying>(c)) { assert(c == 0 || c == 1 || c == -1); }

    constexpr Color operator~() const noexcept { return static_cast<Color>(static_cast<uint8_t>(color) ^ 1); }
    constexpr bool operator==(const Color& rhs) const noexcept { return color == rhs.color; }
    constexpr bool operator!=(const Color& rhs) const noexcept { return color != rhs.color; }

    constexpr operator int() const noexcept { return static_cast<int>(color); }

    [[nodiscard]] constexpr underlying internal() const noexcept { return color; }

    static constexpr underlying WHITE = underlying::WHITE;
    static constexpr underlying BLACK = underlying::BLACK;
    static constexpr underlying NONE  = underlying::NONE;

   private:
    underlying color;
};  // namespace chess

constexpr Color::underlying operator~(Color::underlying color) {
    switch (color) {
        case Color::underlying::WHITE:
            return Color::underlying::BLACK;
        case Color::underlying::BLACK:
            return Color::underlying::WHITE;
        default:
            return Color::underlying::NONE;
    }
}

}  // namespace chess

namespace chess {

class File {
   public:
    enum class underlying : std::uint8_t { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, NO_FILE };

    constexpr File() : file(underlying::NO_FILE) {}
    constexpr File(underlying file) : file(file) {}
    constexpr File(int file) : file(static_cast<underlying>(file)) {}

    [[nodiscard]] constexpr underlying internal() const noexcept { return file; }

    constexpr bool operator==(const File& rhs) const noexcept { return file == rhs.file; }
    constexpr bool operator!=(const File& rhs) const noexcept { return file != rhs.file; }

    constexpr bool operator==(const underlying& rhs) const noexcept { return file == rhs; }
    constexpr bool operator!=(const underlying& rhs) const noexcept { return file != rhs; }

    constexpr bool operator>=(const File& rhs) const noexcept {
        return static_cast<int>(file) >= static_cast<int>(rhs.file);
    }
    constexpr bool operator<=(const File& rhs) const noexcept {
        return static_cast<int>(file) <= static_cast<int>(rhs.file);
    }

    constexpr bool operator>(const File& rhs) const noexcept {
        return static_cast<int>(file) > static_cast<int>(rhs.file);
    }

    constexpr bool operator<(const File& rhs) const noexcept {
        return static_cast<int>(file) < static_cast<int>(rhs.file);
    }

    constexpr operator int() const noexcept { return static_cast<int>(file); }

    static constexpr underlying FILE_A  = underlying::FILE_A;
    static constexpr underlying FILE_B  = underlying::FILE_B;
    static constexpr underlying FILE_C  = underlying::FILE_C;
    static constexpr underlying FILE_D  = underlying::FILE_D;
    static constexpr underlying FILE_E  = underlying::FILE_E;
    static constexpr underlying FILE_F  = underlying::FILE_F;
    static constexpr underlying FILE_G  = underlying::FILE_G;
    static constexpr underlying FILE_H  = underlying::FILE_H;
    static constexpr underlying NO_FILE = underlying::NO_FILE;

   private:
    underlying file;
};

class Rank {
   public:
    enum class underlying { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, NO_RANK };

    constexpr Rank() : rank_(underlying::NO_RANK) {}
    constexpr Rank(underlying rank) : rank_(rank) {}
    constexpr Rank(int rank) : rank_(static_cast<underlying>(rank)) {}

    [[nodiscard]] constexpr underlying internal() const noexcept { return rank_; }

    constexpr bool operator==(const Rank& rhs) const noexcept { return rank_ == rhs.rank_; }
    constexpr bool operator!=(const Rank& rhs) const noexcept { return rank_ != rhs.rank_; }

    constexpr bool operator==(const underlying& rhs) const noexcept { return rank_ == rhs; }
    constexpr bool operator!=(const underlying& rhs) const noexcept { return rank_ != rhs; }

    constexpr bool operator>=(const Rank& rhs) const noexcept {
        return static_cast<int>(rank_) >= static_cast<int>(rhs.rank_);
    }
    constexpr bool operator<=(const Rank& rhs) const noexcept {
        return static_cast<int>(rank_) <= static_cast<int>(rhs.rank_);
    }

    constexpr operator int() const noexcept { return static_cast<int>(rank_); }

    constexpr std::uint64_t bb() const noexcept { return 0xffULL << (8 * static_cast<int>(rank_)); }

    [[nodiscard]] static constexpr bool back_rank(Rank r, Color color) noexcept {
        if (color == Color::WHITE) return r == Rank::RANK_1;
        return r == Rank::RANK_8;
    }

    [[nodiscard]] static constexpr Rank rank(Rank r, Color color) noexcept {
        if (color == Color::WHITE) return r;
        return Rank(static_cast<underlying>(static_cast<int>(RANK_8) - static_cast<int>(r)));
    }

    static constexpr underlying RANK_1  = underlying::RANK_1;
    static constexpr underlying RANK_2  = underlying::RANK_2;
    static constexpr underlying RANK_3  = underlying::RANK_3;
    static constexpr underlying RANK_4  = underlying::RANK_4;
    static constexpr underlying RANK_5  = underlying::RANK_5;
    static constexpr underlying RANK_6  = underlying::RANK_6;
    static constexpr underlying RANK_7  = underlying::RANK_7;
    static constexpr underlying RANK_8  = underlying::RANK_8;
    static constexpr underlying NO_RANK = underlying::NO_RANK;

   private:
    underlying rank_;
};

class Square {
   public:
    // clang-format off
    enum class underlying : uint8_t {
        SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
        SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
        SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
        SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
        SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
        SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
        SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
        SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
        NO_SQ
    };
    // clang-format on

    constexpr Square() : sq(underlying::NO_SQ) {}

    constexpr Square(int sq) : sq(static_cast<underlying>(sq)) { assert(sq <= 64 && sq >= 0); }
    constexpr Square(File file, Rank rank) : sq(static_cast<underlying>(file + rank * 8)) {}
    constexpr Square(Rank rank, File file) : sq(static_cast<underlying>(file + rank * 8)) {}
    constexpr Square(underlying sq) : sq(sq) {}
    constexpr Square(const char str[2]) : sq(static_cast<underlying>((str[0] - 'a') + (str[1] - '1') * 8)) {}

    constexpr Square operator^(const Square& s) const noexcept {
        return Square(static_cast<underlying>(static_cast<int>(sq) ^ s.index()));
    };

    constexpr bool operator==(const Square& rhs) const noexcept { return sq == rhs.sq; }

    constexpr bool operator!=(const Square& rhs) const noexcept { return sq != rhs.sq; }

    constexpr bool operator>(const Square& rhs) const noexcept {
        return static_cast<int>(sq) > static_cast<int>(rhs.sq);
    }

    constexpr bool operator>=(const Square& rhs) const noexcept {
        return static_cast<int>(sq) >= static_cast<int>(rhs.sq);
    }

    constexpr bool operator<(const Square& rhs) const noexcept {
        return static_cast<int>(sq) < static_cast<int>(rhs.sq);
    }

    constexpr bool operator<=(const Square& rhs) const noexcept {
        return static_cast<int>(sq) <= static_cast<int>(rhs.sq);
    }

    constexpr Square operator+(const Square& rhs) const noexcept {
        return Square(static_cast<underlying>(static_cast<int>(sq) + static_cast<int>(rhs.sq)));
    }

    constexpr Square operator-(const Square& rhs) const noexcept {
        return Square(static_cast<underlying>(static_cast<int>(sq) - static_cast<int>(rhs.sq)));
    }

    constexpr Square& operator++() noexcept {
        sq = static_cast<underlying>(static_cast<int>(sq) + 1);
        return *this;
    }

    constexpr Square operator++(int) noexcept {
        Square tmp(*this);
        operator++();
        return tmp;
    }

    constexpr Square& operator--() noexcept {
        sq = static_cast<underlying>(static_cast<int>(sq) - 1);
        return *this;
    }

    constexpr Square operator--(int) noexcept {
        Square tmp(*this);
        operator--();
        return tmp;
    }

    [[nodiscard]] constexpr int index() const noexcept { return static_cast<int>(sq); }

    [[nodiscard]] constexpr File file() const noexcept { return File(index() & 7); }
    [[nodiscard]] constexpr Rank rank() const noexcept { return Rank(index() >> 3); }

    /**
     * @brief Check if the square is light.
     * @return
     */
    [[nodiscard]] constexpr bool is_light() const noexcept {
        return (static_cast<std::int8_t>(sq) / 8 + static_cast<std::int8_t>(sq) % 8) % 2 == 0;
    }

    /**
     * @brief Check if the square is dark.
     * @return
     */
    [[nodiscard]] constexpr bool is_dark() const noexcept { return !is_light(); }

    /**
     * @brief Check if the square is vali.d
     * @return
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept { return static_cast<std::int8_t>(sq) < 64; }

    /**
     * @brief Check if the square is valid.
     * @param r
     * @param f
     * @return
     */
    [[nodiscard]] constexpr static bool is_valid(Rank r, File f) noexcept {
        return r >= Rank::RANK_1 && r <= Rank::RANK_8 && f >= File::FILE_A && f <= File::FILE_H;
    }

    /**
     * @brief Get the chebyshev distance between two squares.
     * @param sq
     * @param sq2
     * @return
     */
    [[nodiscard]] static int distance(Square sq, Square sq2) noexcept {
        return std::max(std::abs(sq.file() - sq2.file()), std::abs(sq.rank() - sq2.rank()));
    }

    /**
     * @brief Absolute value of sq - sq2.
     * @param sq
     * @param sq2
     * @return
     */
    [[nodiscard]] static int value_distance(Square sq, Square sq2) noexcept {
        return std::abs(sq.index() - sq2.index());
    }

    /**
     * @brief Check if the squares share the same color. I.e. if they are both light or dark.
     * @param sq
     * @param sq2
     * @return
     */
    [[nodiscard]] static constexpr bool same_color(Square sq, Square sq2) noexcept {
        return ((9 * (sq ^ sq2).index()) & 8) == 0;
    }

    /**
     * @brief Check if the square is on the back rank.
     * @param sq
     * @param color
     * @return
     */
    [[nodiscard]] static constexpr bool back_rank(Square sq, Color color) noexcept {
        if (color == Color::WHITE)
            return sq.rank() == Rank::RANK_1;
        else
            return sq.rank() == Rank::RANK_8;
    }

    /**
     * @brief Flips the square vertically.
     * @return
     */
    constexpr Square& flip() noexcept {
        sq = static_cast<underlying>(static_cast<int>(sq) ^ 56);
        return *this;
    }

    /**
     * @brief Flips the square vertically, depending on the color.
     * @param c
     * @return
     */
    [[nodiscard]] constexpr Square relative_square(Color c) const noexcept {
        return Square(static_cast<int>(sq) ^ (c * 56));
    }

    [[nodiscard]] constexpr int diagonal_of() const noexcept { return 7 + rank() - file(); }

    [[nodiscard]] constexpr int antidiagonal_of() const noexcept { return rank() + file(); }

    /**
     * @brief Get the en passant square. Should only be called for valid ep positions.
     * @return
     */
    [[nodiscard]] constexpr Square ep_square() const noexcept {
        assert(rank() == Rank::RANK_3     // capture
               || rank() == Rank::RANK_4  // push
               || rank() == Rank::RANK_5  // push
               || rank() == Rank::RANK_6  // capture
        );
        return Square(static_cast<int>(sq) ^ 8);
    }

    /**
     * @brief Get the destination square of the king after castling.
     * @param is_king_side
     * @param c
     * @return
     */
    [[nodiscard]] static constexpr Square castling_king_square(bool is_king_side, Color c) noexcept {
        return Square(is_king_side ? Square::underlying::SQ_G1 : Square::underlying::SQ_C1).relative_square(c);
    }

    /**
     * @brief Get the destination square of the rook after castling.
     * @param is_king_side
     * @param c
     * @return
     */
    [[nodiscard]] static constexpr Square castling_rook_square(bool is_king_side, Color c) noexcept {
        return Square(is_king_side ? Square::underlying::SQ_F1 : Square::underlying::SQ_D1).relative_square(c);
    }

    /**
     * @brief Maximum number of squares.
     * @return
     */
    [[nodiscard]] static constexpr int max() noexcept { return 64; }

   private:
    underlying sq;
};

enum class Direction : int8_t {
    NORTH      = 8,
    WEST       = -1,
    SOUTH      = -8,
    EAST       = 1,
    NORTH_EAST = 9,
    NORTH_WEST = 7,
    SOUTH_WEST = -9,
    SOUTH_EAST = -7
};

[[nodiscard]] constexpr Direction make_direction(Direction dir, Color c) noexcept {
    if (c == Color::BLACK) return static_cast<Direction>(-static_cast<int8_t>(dir));
    return dir;
}

constexpr Square operator+(Square sq, Direction dir) {
    return static_cast<Square>(sq.index() + static_cast<int8_t>(dir));
}

}  // namespace chess

namespace chess {

class Bitboard {
   public:
    constexpr Bitboard() : bits(0) {}
    constexpr Bitboard(std::uint64_t bits) : bits(bits) {}
    constexpr Bitboard(File file) : bits(0) {
        assert(file != File::NO_FILE);
        bits = 0x0101010101010101ULL << static_cast<int>(file.internal());
    }
    constexpr Bitboard(Rank rank) : bits(0) {
        assert(rank != Rank::NO_RANK);
        bits = 0xFFULL << (8 * static_cast<int>(rank.internal()));
    }

    explicit operator bool() const noexcept { return bits != 0; }

    constexpr Bitboard operator&&(bool rhs) const noexcept { return Bitboard(bits && rhs); }

    constexpr Bitboard operator&(std::uint64_t rhs) const noexcept { return Bitboard(bits & rhs); }
    constexpr Bitboard operator|(std::uint64_t rhs) const noexcept { return Bitboard(bits | rhs); }
    constexpr Bitboard operator^(std::uint64_t rhs) const noexcept { return Bitboard(bits ^ rhs); }
    constexpr Bitboard operator<<(std::uint64_t rhs) const noexcept { return Bitboard(bits << rhs); }
    constexpr Bitboard operator>>(std::uint64_t rhs) const noexcept { return Bitboard(bits >> rhs); }
    constexpr bool operator==(std::uint64_t rhs) const noexcept { return bits == rhs; }
    constexpr bool operator!=(std::uint64_t rhs) const noexcept { return bits != rhs; }

    constexpr Bitboard operator&(const Bitboard& rhs) const noexcept { return Bitboard(bits & rhs.bits); }
    constexpr Bitboard operator|(const Bitboard& rhs) const noexcept { return Bitboard(bits | rhs.bits); }
    constexpr Bitboard operator^(const Bitboard& rhs) const noexcept { return Bitboard(bits ^ rhs.bits); }
    constexpr Bitboard operator~() const noexcept { return Bitboard(~bits); }

    constexpr Bitboard& operator&=(const Bitboard& rhs) noexcept {
        bits &= rhs.bits;
        return *this;
    }

    constexpr Bitboard& operator|=(const Bitboard& rhs) noexcept {
        bits |= rhs.bits;
        return *this;
    }

    constexpr Bitboard& operator^=(const Bitboard& rhs) noexcept {
        bits ^= rhs.bits;
        return *this;
    }

    constexpr bool operator==(const Bitboard& rhs) const noexcept { return bits == rhs.bits; }
    constexpr bool operator!=(const Bitboard& rhs) const noexcept { return bits != rhs.bits; }
    constexpr bool operator||(const Bitboard& rhs) const noexcept { return bits || rhs.bits; }
    constexpr bool operator&&(const Bitboard& rhs) const noexcept { return bits && rhs.bits; }

    constexpr Bitboard& set(int index) noexcept {
        assert(index >= 0 && index < 64);
        bits |= (1ULL << index);
        return *this;
    }

    [[nodiscard]] constexpr bool check(int index) const noexcept {
        assert(index >= 0 && index < 64);
        return bits & (1ULL << index);
    }

    constexpr Bitboard& clear(int index) noexcept {
        assert(index >= 0 && index < 64);
        bits &= ~(1ULL << index);
        return *this;
    }

    constexpr Bitboard& clear() noexcept {
        bits = 0;
        return *this;
    }

    [[nodiscard]] static constexpr Bitboard fromSquare(int index) noexcept {
        assert(index >= 0 && index < 64);
        return Bitboard(1ULL << index);
    }

    [[nodiscard]] static constexpr Bitboard fromSquare(Square sq) noexcept {
        assert(sq.index() >= 0 && sq.index() < 64);
        return Bitboard(1ULL << sq.index());
    }

    [[nodiscard]] constexpr bool empty() const noexcept { return bits == 0; }

    [[nodiscard]]
    constexpr int lsb() const noexcept {
        assert(bits != 0);

        return __builtin_ctzll(bits);
    }

    [[nodiscard]]
    constexpr int msb() const noexcept {
        assert(bits != 0);

        return 63 ^ __builtin_clzll(bits);

    }

    [[nodiscard]]
    constexpr int count() const noexcept {
		return __builtin_popcountll(bits);
    }

    [[nodiscard]]

    constexpr std::uint8_t pop() noexcept {
        assert(bits != 0);
        std::uint8_t index = lsb();
        bits &= bits - 1;
        return index;
    }

    [[nodiscard]] constexpr std::uint64_t getBits() const noexcept { return bits; }

   private:
    std::uint64_t bits;
};

constexpr Bitboard operator&(std::uint64_t lhs, const Bitboard& rhs) { return rhs & lhs; }
constexpr Bitboard operator|(std::uint64_t lhs, const Bitboard& rhs) { return rhs | lhs; }
}  // namespace chess

namespace chess {
class Board;
}  // namespace chess

namespace chess {
class attacks {
    using U64 = std::uint64_t;
    struct Magic {
        U64 mask;
        U64 magic;
        Bitboard *attacks;
        U64 shift;

        U64 operator()(Bitboard b) const { return (((b & mask)).getBits() * magic) >> shift; }
    };

    // Slow function to calculate bishop attacks
    [[nodiscard]] static Bitboard bishopAttacks(Square sq, Bitboard occupied);

    // Slow function to calculate rook attacks
    [[nodiscard]] static Bitboard rookAttacks(Square sq, Bitboard occupied);

    // Initializes the magic bitboard tables for sliding pieces
    static void initSliders(Square sq, Magic table[], U64 magic,
                            const std::function<Bitboard(Square, Bitboard)> &attacks);

    // clang-format off
    // pre-calculated lookup table for pawn attacks
    static constexpr Bitboard PawnAttacks[2][64] = {
        // white pawn attacks
        { 0x200, 0x500, 0xa00, 0x1400,
        0x2800, 0x5000, 0xa000, 0x4000,
        0x20000, 0x50000, 0xa0000, 0x140000,
        0x280000, 0x500000, 0xa00000, 0x400000,
        0x2000000, 0x5000000, 0xa000000, 0x14000000,
        0x28000000, 0x50000000, 0xa0000000, 0x40000000,
        0x200000000, 0x500000000, 0xa00000000, 0x1400000000,
        0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000,
        0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000,
        0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000,
        0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000,
        0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000,
        0x200000000000000, 0x500000000000000, 0xa00000000000000, 0x1400000000000000,
        0x2800000000000000, 0x5000000000000000, 0xa000000000000000, 0x4000000000000000,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0 },

        // black pawn attacks
        { 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0,
            0x2, 0x5, 0xa, 0x14,
            0x28, 0x50, 0xa0, 0x40,
            0x200, 0x500, 0xa00, 0x1400,
            0x2800, 0x5000, 0xa000, 0x4000,
            0x20000, 0x50000, 0xa0000, 0x140000,
            0x280000, 0x500000, 0xa00000, 0x400000,
            0x2000000, 0x5000000, 0xa000000, 0x14000000,
            0x28000000, 0x50000000, 0xa0000000, 0x40000000,
            0x200000000, 0x500000000, 0xa00000000, 0x1400000000,
            0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000,
            0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000,
            0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000,
            0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000,
            0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000
        }
    };

    // clang-format on

    // pre-calculated lookup table for knight attacks
    static constexpr Bitboard KnightAttacks[64] = {
        0x0000000000020400, 0x0000000000050800, 0x00000000000A1100, 0x0000000000142200, 0x0000000000284400,
        0x0000000000508800, 0x0000000000A01000, 0x0000000000402000, 0x0000000002040004, 0x0000000005080008,
        0x000000000A110011, 0x0000000014220022, 0x0000000028440044, 0x0000000050880088, 0x00000000A0100010,
        0x0000000040200020, 0x0000000204000402, 0x0000000508000805, 0x0000000A1100110A, 0x0000001422002214,
        0x0000002844004428, 0x0000005088008850, 0x000000A0100010A0, 0x0000004020002040, 0x0000020400040200,
        0x0000050800080500, 0x00000A1100110A00, 0x0000142200221400, 0x0000284400442800, 0x0000508800885000,
        0x0000A0100010A000, 0x0000402000204000, 0x0002040004020000, 0x0005080008050000, 0x000A1100110A0000,
        0x0014220022140000, 0x0028440044280000, 0x0050880088500000, 0x00A0100010A00000, 0x0040200020400000,
        0x0204000402000000, 0x0508000805000000, 0x0A1100110A000000, 0x1422002214000000, 0x2844004428000000,
        0x5088008850000000, 0xA0100010A0000000, 0x4020002040000000, 0x0400040200000000, 0x0800080500000000,
        0x1100110A00000000, 0x2200221400000000, 0x4400442800000000, 0x8800885000000000, 0x100010A000000000,
        0x2000204000000000, 0x0004020000000000, 0x0008050000000000, 0x00110A0000000000, 0x0022140000000000,
        0x0044280000000000, 0x0088500000000000, 0x0010A00000000000, 0x0020400000000000};

    // pre-calculated lookup table for king attacks
    static constexpr Bitboard KingAttacks[64] = {
        0x0000000000000302, 0x0000000000000705, 0x0000000000000E0A, 0x0000000000001C14, 0x0000000000003828,
        0x0000000000007050, 0x000000000000E0A0, 0x000000000000C040, 0x0000000000030203, 0x0000000000070507,
        0x00000000000E0A0E, 0x00000000001C141C, 0x0000000000382838, 0x0000000000705070, 0x0000000000E0A0E0,
        0x0000000000C040C0, 0x0000000003020300, 0x0000000007050700, 0x000000000E0A0E00, 0x000000001C141C00,
        0x0000000038283800, 0x0000000070507000, 0x00000000E0A0E000, 0x00000000C040C000, 0x0000000302030000,
        0x0000000705070000, 0x0000000E0A0E0000, 0x0000001C141C0000, 0x0000003828380000, 0x0000007050700000,
        0x000000E0A0E00000, 0x000000C040C00000, 0x0000030203000000, 0x0000070507000000, 0x00000E0A0E000000,
        0x00001C141C000000, 0x0000382838000000, 0x0000705070000000, 0x0000E0A0E0000000, 0x0000C040C0000000,
        0x0003020300000000, 0x0007050700000000, 0x000E0A0E00000000, 0x001C141C00000000, 0x0038283800000000,
        0x0070507000000000, 0x00E0A0E000000000, 0x00C040C000000000, 0x0302030000000000, 0x0705070000000000,
        0x0E0A0E0000000000, 0x1C141C0000000000, 0x3828380000000000, 0x7050700000000000, 0xE0A0E00000000000,
        0xC040C00000000000, 0x0203000000000000, 0x0507000000000000, 0x0A0E000000000000, 0x141C000000000000,
        0x2838000000000000, 0x5070000000000000, 0xA0E0000000000000, 0x40C0000000000000};

    static constexpr U64 RookMagics[64] = {
        0x8a80104000800020ULL, 0x140002000100040ULL,  0x2801880a0017001ULL,  0x100081001000420ULL,
        0x200020010080420ULL,  0x3001c0002010008ULL,  0x8480008002000100ULL, 0x2080088004402900ULL,
        0x800098204000ULL,     0x2024401000200040ULL, 0x100802000801000ULL,  0x120800800801000ULL,
        0x208808088000400ULL,  0x2802200800400ULL,    0x2200800100020080ULL, 0x801000060821100ULL,
        0x80044006422000ULL,   0x100808020004000ULL,  0x12108a0010204200ULL, 0x140848010000802ULL,
        0x481828014002800ULL,  0x8094004002004100ULL, 0x4010040010010802ULL, 0x20008806104ULL,
        0x100400080208000ULL,  0x2040002120081000ULL, 0x21200680100081ULL,   0x20100080080080ULL,
        0x2000a00200410ULL,    0x20080800400ULL,      0x80088400100102ULL,   0x80004600042881ULL,
        0x4040008040800020ULL, 0x440003000200801ULL,  0x4200011004500ULL,    0x188020010100100ULL,
        0x14800401802800ULL,   0x2080040080800200ULL, 0x124080204001001ULL,  0x200046502000484ULL,
        0x480400080088020ULL,  0x1000422010034000ULL, 0x30200100110040ULL,   0x100021010009ULL,
        0x2002080100110004ULL, 0x202008004008002ULL,  0x20020004010100ULL,   0x2048440040820001ULL,
        0x101002200408200ULL,  0x40802000401080ULL,   0x4008142004410100ULL, 0x2060820c0120200ULL,
        0x1001004080100ULL,    0x20c020080040080ULL,  0x2935610830022400ULL, 0x44440041009200ULL,
        0x280001040802101ULL,  0x2100190040002085ULL, 0x80c0084100102001ULL, 0x4024081001000421ULL,
        0x20030a0244872ULL,    0x12001008414402ULL,   0x2006104900a0804ULL,  0x1004081002402ULL};

    static constexpr U64 BishopMagics[64] = {
        0x40040844404084ULL,   0x2004208a004208ULL,   0x10190041080202ULL,   0x108060845042010ULL,
        0x581104180800210ULL,  0x2112080446200010ULL, 0x1080820820060210ULL, 0x3c0808410220200ULL,
        0x4050404440404ULL,    0x21001420088ULL,      0x24d0080801082102ULL, 0x1020a0a020400ULL,
        0x40308200402ULL,      0x4011002100800ULL,    0x401484104104005ULL,  0x801010402020200ULL,
        0x400210c3880100ULL,   0x404022024108200ULL,  0x810018200204102ULL,  0x4002801a02003ULL,
        0x85040820080400ULL,   0x810102c808880400ULL, 0xe900410884800ULL,    0x8002020480840102ULL,
        0x220200865090201ULL,  0x2010100a02021202ULL, 0x152048408022401ULL,  0x20080002081110ULL,
        0x4001001021004000ULL, 0x800040400a011002ULL, 0xe4004081011002ULL,   0x1c004001012080ULL,
        0x8004200962a00220ULL, 0x8422100208500202ULL, 0x2000402200300c08ULL, 0x8646020080080080ULL,
        0x80020a0200100808ULL, 0x2010004880111000ULL, 0x623000a080011400ULL, 0x42008c0340209202ULL,
        0x209188240001000ULL,  0x400408a884001800ULL, 0x110400a6080400ULL,   0x1840060a44020800ULL,
        0x90080104000041ULL,   0x201011000808101ULL,  0x1a2208080504f080ULL, 0x8012020600211212ULL,
        0x500861011240000ULL,  0x180806108200800ULL,  0x4000020e01040044ULL, 0x300000261044000aULL,
        0x802241102020002ULL,  0x20906061210001ULL,   0x5a84841004010310ULL, 0x4010801011c04ULL,
        0xa010109502200ULL,    0x4a02012000ULL,       0x500201010098b028ULL, 0x8040002811040900ULL,
        0x28000010020204ULL,   0x6000020202d0240ULL,  0x8918844842082200ULL, 0x4010011029020020ULL};

    //static inline Bitboard RookAttacks[0x19000]  = {};
    static inline Bitboard VRookAttacks[64][64]  = {};
    static inline Bitboard HRookAttacks[64][64]  = {};
    static inline Bitboard BishopAttacks[0x1480] = {};

    //static inline Magic RookTable[64]   = {};
    static inline Magic BishopTable[64] = {};

   public:
    static constexpr Bitboard MASK_RANK[8] = {0xff,         0xff00,         0xff0000,         0xff000000,
                                              0xff00000000, 0xff0000000000, 0xff000000000000, 0xff00000000000000};

    static constexpr Bitboard MASK_FILE[8] = {
        0x101010101010101,  0x202020202020202,  0x404040404040404,  0x808080808080808,
        0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080,
    };

    /**
     * @brief  Shifts a bitboard in a given direction
     * @tparam direction
     * @param b
     * @return
     */
    template <Direction direction>
    [[nodiscard]] static constexpr Bitboard shift(const Bitboard b);

    /**
     * @brief
     * @tparam c
     * @param pawns
     * @return
     */
    template <Color::underlying c>
    [[nodiscard]] static Bitboard pawnLeftAttacks(const Bitboard pawns);

    /**
     * @brief Generate the right side pawn attacks.
     * @tparam c
     * @param pawns
     * @return
     */
    template <Color::underlying c>
    [[nodiscard]] static Bitboard pawnRightAttacks(const Bitboard pawns);

    /**
     * @brief Returns the pawn attacks for a given color and square
     * @param c
     * @param sq
     * @return
     */
    [[nodiscard]] static Bitboard pawn(Color c, Square sq) noexcept;

    /**
     * @brief Returns the knight attacks for a given square
     * @param sq
     * @return
     */
    [[nodiscard]] static Bitboard knight(Square sq) noexcept;

    /**
     * @brief Returns the bishop attacks for a given square
     * @param sq
     * @param occupied
     * @return
     */
    [[nodiscard]] static Bitboard bishop(Square sq, Bitboard occupied) noexcept;

    /**
     * @brief Returns the rook attacks for a given square
     * @param sq
     * @param occupied
     * @return
     */
    [[nodiscard]] static Bitboard rook(Square sq, Bitboard occupied) noexcept;

    /**
     * @brief Returns the queen attacks for a given square
     * @param sq
     * @param occupied
     * @return
     */
    [[nodiscard]] static Bitboard queen(Square sq, Bitboard occupied) noexcept;

    /**
     * @brief Returns the king attacks for a given square
     * @param sq
     * @return
     */
    [[nodiscard]] static Bitboard king(Square sq) noexcept;

    /**
     * @brief Returns the attacks for a given piece on a given square
     * @param board
     * @param color
     * @param square
     * @return
     */
    [[nodiscard]] static Bitboard attackers(const Board &board, Color color, Square square) noexcept;

    /**
     * @brief [Internal Usage] Initializes the attacks for the bishop and rook. Called once at startup.
     */
    static inline void initAttacks();
};
}  // namespace chess

#include <array>
#include <cctype>
#include <optional>



namespace chess::constants {

constexpr Bitboard DEFAULT_CHECKMASK = Bitboard(0xFFFFFFFFFFFFFFFFull);
constexpr auto STARTPOS              = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr auto MAX_MOVES             = 256;
}  // namespace chess::constants

namespace chess {

class PieceType {
   public:
    enum class underlying : std::uint8_t {
        PAWN,
        KNIGHT,
        BISHOP,
        ROOK,
        QUEEN,
        KING,
        NONE,
    };

    constexpr PieceType() : pt(underlying::NONE) {}
    constexpr PieceType(underlying pt) : pt(pt) {}
    constexpr explicit PieceType(char c) : pt(underlying::NONE) {
        switch (c) {
            case 'P':
                pt = underlying::PAWN;
                break;
            case 'N':
                pt = underlying::KNIGHT;
                break;
            case 'B':
                pt = underlying::BISHOP;
                break;
            case 'R':
                pt = underlying::ROOK;
                break;
            case 'Q':
                pt = underlying::QUEEN;
                break;
            case 'K':
                pt = underlying::KING;
                break;

            case 'p':
                pt = underlying::PAWN;
                break;
            case 'n':
                pt = underlying::KNIGHT;
                break;
            case 'b':
                pt = underlying::BISHOP;
                break;
            case 'r':
                pt = underlying::ROOK;
                break;
            case 'q':
                pt = underlying::QUEEN;
                break;
            case 'k':
                pt = underlying::KING;
                break;
            default:
                pt = underlying::NONE;
                break;
        }
    }

    constexpr bool operator==(const PieceType& rhs) const noexcept { return pt == rhs.pt; }
    constexpr bool operator!=(const PieceType& rhs) const noexcept { return pt != rhs.pt; }

    constexpr operator int() const noexcept { return static_cast<int>(pt); }

    [[nodiscard]] constexpr underlying internal() const noexcept { return pt; }

    static constexpr underlying PAWN   = underlying::PAWN;
    static constexpr underlying KNIGHT = underlying::KNIGHT;
    static constexpr underlying BISHOP = underlying::BISHOP;
    static constexpr underlying ROOK   = underlying::ROOK;
    static constexpr underlying QUEEN  = underlying::QUEEN;
    static constexpr underlying KING   = underlying::KING;
    static constexpr underlying NONE   = underlying::NONE;

   private:
    underlying pt;
};

class Piece {
   public:
    enum class underlying : std::uint8_t {
        WHITEPAWN,
        WHITEKNIGHT,
        WHITEBISHOP,
        WHITEROOK,
        WHITEQUEEN,
        WHITEKING,
        BLACKPAWN,
        BLACKKNIGHT,
        BLACKBISHOP,
        BLACKROOK,
        BLACKQUEEN,
        BLACKKING,
        NONE
    };

    constexpr Piece() : piece(underlying::NONE) {}
    constexpr Piece(underlying piece) : piece(piece) {}
    constexpr Piece(PieceType type, Color color)
        : piece(color == Color::NONE      ? Piece::NONE
                : type == PieceType::NONE ? Piece::NONE
                                          : static_cast<underlying>(static_cast<int>(color.internal()) * 6 + type)) {}
    constexpr Piece(Color color, PieceType type)
        : piece(color == Color::NONE      ? Piece::NONE
                : type == PieceType::NONE ? Piece::NONE
                                          : static_cast<underlying>(static_cast<int>(color.internal()) * 6 + type)) {}
    constexpr Piece(std::string_view p) : piece(underlying::NONE) { piece = convertCharToUnderlying(p[0]); }

    constexpr bool operator<(const Piece& rhs) const noexcept { return piece < rhs.piece; }
    constexpr bool operator>(const Piece& rhs) const noexcept { return piece > rhs.piece; }
    constexpr bool operator==(const Piece& rhs) const noexcept { return piece == rhs.piece; }
    constexpr bool operator!=(const Piece& rhs) const noexcept { return piece != rhs.piece; }

    constexpr bool operator==(const underlying& rhs) const noexcept { return piece == rhs; }
    constexpr bool operator!=(const underlying& rhs) const noexcept { return piece != rhs; }

    constexpr bool operator==(const PieceType& rhs) const noexcept { return type() == rhs; }
    constexpr bool operator!=(const PieceType& rhs) const noexcept { return type() != rhs; }

    char toChar() const
    {
        switch (piece) 
        {
        case WHITEPAWN:
            return 'P';
        case WHITEKNIGHT:
            return 'N';
        case WHITEBISHOP:
            return 'B';
        case WHITEROOK:
            return 'R';
        case WHITEQUEEN:
            return 'Q';
        case WHITEKING:
            return 'K';
            // black
        case BLACKPAWN:
            return 'p';
        case BLACKKNIGHT:
            return 'n';
        case BLACKBISHOP:
            return 'b';
        case BLACKROOK:
            return 'r';
        case BLACKQUEEN:
            return 'q';
        case BLACKKING:
            return 'k';
        default:
            return '.';
        }
    }

    constexpr operator int() const noexcept { return static_cast<int>(piece); }

    [[nodiscard]] constexpr PieceType type() const noexcept {
        if (piece == NONE) return PieceType::NONE;
        // return static_cast<PieceType::underlying>(int(piece) % 6);
        return static_cast<PieceType::underlying>(static_cast<int>(piece) > 5 ? static_cast<int>(piece) - 6
                                                                              : static_cast<int>(piece));
    }

    [[nodiscard]] constexpr Color color() const noexcept {
        if (piece == NONE) return Color::NONE;
        return static_cast<Color>(static_cast<int>(piece) / 6);
    }

    [[nodiscard]] constexpr underlying internal() const noexcept { return piece; }

    static constexpr underlying NONE        = underlying::NONE;
    static constexpr underlying WHITEPAWN   = underlying::WHITEPAWN;
    static constexpr underlying WHITEKNIGHT = underlying::WHITEKNIGHT;
    static constexpr underlying WHITEBISHOP = underlying::WHITEBISHOP;
    static constexpr underlying WHITEROOK   = underlying::WHITEROOK;
    static constexpr underlying WHITEQUEEN  = underlying::WHITEQUEEN;
    static constexpr underlying WHITEKING   = underlying::WHITEKING;
    static constexpr underlying BLACKPAWN   = underlying::BLACKPAWN;
    static constexpr underlying BLACKKNIGHT = underlying::BLACKKNIGHT;
    static constexpr underlying BLACKBISHOP = underlying::BLACKBISHOP;
    static constexpr underlying BLACKROOK   = underlying::BLACKROOK;
    static constexpr underlying BLACKQUEEN  = underlying::BLACKQUEEN;
    static constexpr underlying BLACKKING   = underlying::BLACKKING;

   private:
    underlying piece;

    [[nodiscard]] constexpr static underlying convertCharToUnderlying(char c) {
        switch (c) {
            case 'P':
                return WHITEPAWN;
            case 'N':
                return WHITEKNIGHT;
            case 'B':
                return WHITEBISHOP;
            case 'R':
                return WHITEROOK;
            case 'Q':
                return WHITEQUEEN;
            case 'K':
                return WHITEKING;
            case 'p':
                return BLACKPAWN;
            case 'n':
                return BLACKKNIGHT;
            case 'b':
                return BLACKBISHOP;
            case 'r':
                return BLACKROOK;
            case 'q':
                return BLACKQUEEN;
            case 'k':
                return BLACKKING;
            default:
                return NONE;
        }
    }
};
}  // namespace chess

namespace chess {

class Move {
   public:
    Move() = default;
    constexpr Move(std::uint16_t move) : move_(move), score_(0) {}

    /**
     * @brief Creates a move from a source and target square.
     * @tparam MoveType
     * @param source
     * @param target
     * @param pt leave this empty if it is not a promotion move, otherwise pass the piece type of the new piece.
     * @return
     */
    template <std::uint16_t MoveType = 0>
    [[nodiscard]] static constexpr Move make(Square source, Square target, PieceType pt = PieceType::KNIGHT) noexcept {
        assert(pt >= PieceType(PieceType::KNIGHT) && pt <= PieceType(PieceType::QUEEN));

        std::uint16_t bits_promotion = static_cast<std::uint16_t>(pt - PieceType(PieceType::KNIGHT));

        return Move(MoveType + (bits_promotion << 12) + (source.index() << 6) + target.index());
    }

    /**
     * @brief Get the source square of the move.
     * @return
     */
    [[nodiscard]] constexpr Square from() const noexcept { return static_cast<Square>((move_ >> 6) & 0x3F); }

    /**
     * @brief Get the target square of the move.
     * @return
     */
    [[nodiscard]] constexpr Square to() const noexcept { return static_cast<Square>(move_ & 0x3F); }

    /**
     * @brief Get the type of the move. Can be NORMAL, PROMOTION, ENPASSANT or CASTLING.
     * @return
     */
    [[nodiscard]] constexpr std::uint16_t typeOf() const noexcept {
        return static_cast<std::uint16_t>(move_ & (3 << 14));
    }

    /**
     * @brief Get the promotion piece of the move, should only be used if typeOf() returns PROMOTION.
     * @return
     */
    [[nodiscard]] constexpr PieceType promotionType() const noexcept {
        return static_cast<PieceType::underlying>(((move_ >> 12) & 3) + PieceType(PieceType::KNIGHT));
    }

    /**
     * @brief Set the score for a move. Useful if you later want to sort the moves.
     * @param score
     */
    constexpr void setScore(std::int16_t score) noexcept { score_ = score; }

    [[nodiscard]] constexpr std::uint16_t move() const noexcept { return move_; }
    [[nodiscard]] constexpr std::int16_t score() const noexcept { return score_; }

    constexpr bool operator==(const Move &rhs) const noexcept { return move_ == rhs.move_; }
    constexpr bool operator!=(const Move &rhs) const noexcept { return move_ != rhs.move_; }

    static constexpr std::uint16_t NO_MOVE   = 0;
    static constexpr std::uint16_t NULL_MOVE = 65;
    static constexpr std::uint16_t NORMAL    = 0;
    static constexpr std::uint16_t PROMOTION = 1 << 14;
    static constexpr std::uint16_t ENPASSANT = 2 << 14;
    static constexpr std::uint16_t CASTLING  = 3 << 14;

   private:
    std::uint16_t move_;
    std::int16_t score_;
};
}  // namespace chess



#include <cstddef>
//#include <iterator>
//edit: #include <stdexcept>


namespace chess {
class Movelist {
   public:
    using value_type      = Move;
    using size_type       = int;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;

    using iterator       = value_type*;
    using const_iterator = const value_type*;

    // Element access

    [[nodiscard]] constexpr reference at(size_type pos) {
#ifndef CHESS_NO_EXCEPTIONS
        if (pos >= size_) {
            throw std::out_of_range("Movelist::at: pos (which is " + std::to_string(pos) + ") >= size (which is " +
                                    std::to_string(size_) + ")");
        }
#endif
        return moves_[pos];
    }

    [[nodiscard]] constexpr const_reference at(size_type pos) const {
#ifndef CHESS_NO_EXCEPTIONS
        if (pos >= size_) {
            throw std::out_of_range("Movelist::at: pos (which is " + std::to_string(pos) + ") >= size (which is " +
                                    std::to_string(size_) + ")");
        }
#endif
        return moves_[pos];
    }

    [[nodiscard]] constexpr reference operator[](size_type pos) noexcept { return moves_[pos]; }
    [[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept { return moves_[pos]; }

    [[nodiscard]] constexpr reference front() noexcept { return moves_[0]; }
    [[nodiscard]] constexpr const_reference front() const noexcept { return moves_[0]; }

    [[nodiscard]] constexpr reference back() noexcept { return moves_[size_ - 1]; }
    [[nodiscard]] constexpr const_reference back() const noexcept { return moves_[size_ - 1]; }

    // Iterators

    [[nodiscard]] constexpr iterator begin() noexcept { return &moves_[0]; }
    [[nodiscard]] constexpr const_iterator begin() const noexcept { return &moves_[0]; }

    [[nodiscard]] constexpr iterator end() noexcept { return &moves_[0] + size_; }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return &moves_[0] + size_; }

    // Capacity

    /**
     * @brief Checks if the movelist is empty.
     * @return
     */
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

    /**
     * @brief Return the number of moves in the movelist.
     * @return
     */
    [[nodiscard]] constexpr size_type size() const noexcept { return size_; }

    // Modifiers

    /**
     * @brief Clears the movelist.
     */
    constexpr void clear() noexcept { size_ = 0; }

    /**
     * @brief Add a move to the end of the movelist.
     * @param move
     */
    constexpr void add(const_reference move) noexcept {
        assert(size_ < constants::MAX_MOVES);
        moves_[size_++] = move;
    }

    /**
     * @brief Add a move to the end of the movelist.
     * @param move
     */
    constexpr void add(value_type&& move) noexcept {
        assert(size_ < constants::MAX_MOVES);
        moves_[size_++] = move;
    }

    // Other

    /**
     * @brief Checks if a move is in the movelist, returns the index of the move if it is found, otherwise -1.
     * @param move
     * @return
     */
    [[nodiscard]] [[deprecated("Use std::find() instead.")]] constexpr size_type find(value_type move) const noexcept {
        for (size_type i = 0; i < size_; ++i) {
            if (moves_[i] == move) {
                return i;
            }
        }

        return -1;
    }

   private:
    std::array<value_type, constants::MAX_MOVES> moves_;
    size_type size_ = 0;
};
}  // namespace chess

namespace chess {
enum PieceGenType {
    PAWN   = 1,
    KNIGHT = 2,
    BISHOP = 4,
    ROOK   = 8,
    QUEEN  = 16,
    KING   = 32,
};

class Board;

class movegen {
   public:
    enum class MoveGenType : std::uint8_t { ALL, CAPTURE, QUIET };

    /**
     * @brief Generates all legal moves for a position.
     * @tparam mt
     * @param movelist
     * @param board
     * @param pieces
     */
    void static legalmoves(Movelist &movelist, const Board &board, MoveGenType mt = MoveGenType::ALL,
                           int pieces = PieceGenType::PAWN | PieceGenType::KNIGHT | PieceGenType::BISHOP |
                                        PieceGenType::ROOK | PieceGenType::QUEEN | PieceGenType::KING);

   private:
    static auto init_squares_between();
    static const std::array<std::array<Bitboard, 64>, 64> SQUARES_BETWEEN_BB;

    // Generate the checkmask. Returns a bitboard where the attacker path between the king and enemy piece is set.
    template <Color::underlying c>
    [[nodiscard]] static std::pair<Bitboard, int> checkMask(const Board &board, Square sq);

    // Generate the pin mask for horizontal and vertical pins. Returns a bitboard where the ray between the king and the
    // pinner is set.
    template <Color::underlying c>
    [[nodiscard]] static Bitboard pinMaskRooks(const Board &board, Square sq, Bitboard occ_enemy, Bitboard occ_us);

    // Generate the pin mask for diagonal pins. Returns a bitboard where the ray between the king and the pinner is set.
    template <Color::underlying c>
    [[nodiscard]] static Bitboard pinMaskBishops(const Board &board, Square sq, Bitboard occ_enemy, Bitboard occ_us);

    // Returns the squares that are attacked by the enemy
    template <Color::underlying c>
    [[nodiscard]] static Bitboard seenSquares(const Board &board, Bitboard enemy_empty);

    // Generate pawn moves.
    template <Color::underlying c>
    static void generatePawnMoves(const Board &board, MoveGenType mt, Movelist &moves, Bitboard pin_d, Bitboard pin_hv,
                                  Bitboard checkmask, Bitboard occ_enemy);

    [[nodiscard]] static std::array<Move, 2> generateEPMove(const Board &board, Bitboard checkmask, Bitboard pin_d,
                                                            Bitboard pawns_lr, Square ep, Color c);

    [[nodiscard]] static Bitboard generateKnightMoves(Square sq);

    [[nodiscard]] static Bitboard generateBishopMoves(Square sq, Bitboard pin_d, Bitboard occ_all);

    [[nodiscard]] static Bitboard generateRookMoves(Square sq, Bitboard pin_hv, Bitboard occ_all);

    [[nodiscard]] static Bitboard generateQueenMoves(Square sq, Bitboard pin_d, Bitboard pin_hv, Bitboard occ_all);

    [[nodiscard]] static Bitboard generateKingMoves(Square sq, Bitboard seen, Bitboard movable_square);

    template <Color::underlying c>
    [[nodiscard]] static Bitboard generateCastleMoves(const Board &board, Square sq, Bitboard seen, Bitboard pinHV);

    template <typename T>
    static void whileBitboardAdd(Movelist &movelist, Bitboard mask, T func);

    template <Color::underlying c>
    static void legalmoves(Movelist &movelist, const Board &board, MoveGenType mt, int pieces);

    template <Color::underlying c>
    static bool isEpSquareValid(const Board &board, Square ep);

    friend class Board;
};

}  // namespace chess


namespace chess {
class Zobrist {
    using U64                              = std::uint64_t;
    inline static U64 RANDOM_ARRAY[781];

    inline static std::array<U64, 16> castlingKey;

    static constexpr int MAP_HASH_PIECE[12] = {1, 3, 5, 7, 9, 11, 0, 2, 4, 6, 8, 10};

    [[nodiscard]] static U64 piece(Piece piece, Square square) noexcept {
        assert(piece < 12);
        return RANDOM_ARRAY[64 * MAP_HASH_PIECE[piece] + square.index()];
    }

    [[nodiscard]] static U64 enpassant(File file) noexcept {
        assert(int(file) < 8);
        return RANDOM_ARRAY[772 + file];
    }

    [[nodiscard]] static U64 castling(int castling) noexcept {
        assert(castling >= 0 && castling < 16);
        return castlingKey[castling];
    }

    [[nodiscard]] static U64 castlingIndex(int idx) noexcept {
        assert(idx >= 0 && idx < 4);
        return RANDOM_ARRAY[768 + idx];
    }

    [[nodiscard]] static U64 sideToMove() noexcept { return RANDOM_ARRAY[780]; }

   public:
    friend class Board;
	
	static void init()
	{
		auto generateCastlingKey = [](int index) -> U64 {
			constexpr int RANDOM_OFFSET = 768;
			constexpr int RANDOM_COUNT  = 4;

			U64 key = 0;

			for (int i = 0; i < RANDOM_COUNT; ++i) {
				if (index & (1 << i)) {
					key ^= RANDOM_ARRAY[RANDOM_OFFSET + i];
				}
			}

			return key;
		};

		for (int i = 0; i < 16; ++i) castlingKey[i] = generateCastlingKey(i);
		
		for(int i = 0; i < 781; ++i)
		{
			RANDOM_ARRAY[i] = random64();
		}
	}
};
}  // namespace chess

void nnue::add_accumulator(int p, int s);
void nnue::remove_accumulator(int p, int s);
void nnue::clear_accumulator();

namespace chess {

enum class GameResult { WIN, LOSE, DRAW, NONE };

enum class GameResultReason {
    CHECKMATE,
    STALEMATE,
    INSUFFICIENT_MATERIAL,
    FIFTY_MOVE_RULE,
    THREEFOLD_REPETITION,
    NONE
};

// A compact representation of the board in 24 bytes,
// does not include the half-move clock or full move number.
using PackedBoard = std::array<std::uint8_t, 24>;

class Board {
    using U64 = std::uint64_t;

   public:
    class CastlingRights {
       public:
        enum class Side : uint8_t { KING_SIDE, QUEEN_SIDE };

        void setCastlingRight(Color color, Side castle, File rook_file) {
            rooks[color][static_cast<int>(castle)] = rook_file;
        }

        void clear() {
            rooks[0].fill(File::NO_FILE);
            rooks[1].fill(File::NO_FILE);
        }

        int clear(Color color, Side castle) {
            rooks[color][static_cast<int>(castle)] = File::NO_FILE;
            return color * 2 + static_cast<int>(castle);
        }

        void clear(Color color) { rooks[color].fill(File::NO_FILE); }

        bool has(Color color, Side castle) const { return rooks[color][static_cast<int>(castle)] != File::NO_FILE; }

        bool has(Color color) const {
            return rooks[color][static_cast<int>(Side::KING_SIDE)] != File::NO_FILE ||
                   rooks[color][static_cast<int>(Side::QUEEN_SIDE)] != File::NO_FILE;
        }

        File getRookFile(Color color, Side castle) const { return rooks[color][static_cast<int>(castle)]; }

        int hashIndex() const {
            return has(Color::WHITE, Side::KING_SIDE) + 2 * has(Color::WHITE, Side::QUEEN_SIDE) +
                   4 * has(Color::BLACK, Side::KING_SIDE) + 8 * has(Color::BLACK, Side::QUEEN_SIDE);
        }

        bool isEmpty() const { return !has(Color::WHITE) && !has(Color::BLACK); }

        template <typename T>
        static constexpr Side closestSide(T sq, T pred) {
            return sq > pred ? Side::KING_SIDE : Side::QUEEN_SIDE;
        }

       private:
        // [color][side]
        std::array<std::array<File, 2>, 2> rooks;
    };

   private:
    struct State {
        U64 hash;
        CastlingRights castling;
        Square enpassant;
        uint8_t half_moves;
        Piece captured_piece;

        State() {}

        State(const U64 &hash, const CastlingRights &castling, const Square &enpassant, const uint8_t &half_moves,
              const Piece &captured_piece)
            : hash(hash),
              castling(castling),
              enpassant(enpassant),
              half_moves(half_moves),
              captured_piece(captured_piece) {}
    };

    enum class PrivateCtor { CREATE };

    // private constructor to avoid initialization
    Board(PrivateCtor) {}

   public:
    explicit Board(std::string_view fen = constants::STARTPOS) {
        setFenInternal(fen);
    }

    static Board fromFen(std::string_view fen) { return Board(fen); }

    /**
     * @brief Make a move on the board. The move must be legal otherwise the
     * behavior is undefined. EXACT can be set to true to only record
     * the enpassant square if the enemy can legally capture the pawn on their
     * next move.
     * @tparam EXACT (deleted by me)
     * @param move
     */
    void makeMove(const Move move) {
        const auto capture  = at(move.to()) != Piece::NONE && move.typeOf() != Move::CASTLING;
        const auto captured = at(move.to());
        const auto pt       = at<PieceType>(move.from());

        // Validate side to move
        assert((at(move.from()) < Piece::BLACKPAWN) == (stm_ == Color::WHITE));

        //prev_states_.emplace_back(key_, cr_, ep_sq_, hfm_, captured);
        new (&prev_states_[plies_++]) State(key_, cr_, ep_sq_, hfm_, captured);

        hfm_++;
        
        if (ep_sq_ != Square::underlying::NO_SQ) key_ ^= Zobrist::enpassant(ep_sq_.file());
        ep_sq_ = Square::underlying::NO_SQ;

        if (capture) {
            removePieceInternal(captured, move.to());

            hfm_ = 0;
            key_ ^= Zobrist::piece(captured, move.to());

            // remove castling rights if rook is captured
            if (captured.type() == PieceType::ROOK && Rank::back_rank(move.to().rank(), ~stm_)) {
                const auto king_sq = kingSq(~stm_);
                const auto file    = CastlingRights::closestSide(move.to(), king_sq);

                if (cr_.getRookFile(~stm_, file) == move.to().file()) {
                    key_ ^= Zobrist::castlingIndex(cr_.clear(~stm_, file));
                }
            }
        }

        // remove castling rights if king moves
        if (pt == PieceType::KING && cr_.has(stm_)) {
            key_ ^= Zobrist::castling(cr_.hashIndex());
            cr_.clear(stm_);
            key_ ^= Zobrist::castling(cr_.hashIndex());
        }
		else if (pt == PieceType::ROOK && Square::back_rank(move.from(), stm_)) 
		{
            const auto king_sq = kingSq(stm_);
            const auto file    = CastlingRights::closestSide(move.from(), king_sq);

            // remove castling rights if rook moves from back rank
            if (cr_.getRookFile(stm_, file) == move.from().file()) {
                key_ ^= Zobrist::castlingIndex(cr_.clear(stm_, file));
            }
        } 
		else if (pt == PieceType::PAWN)
		{
            hfm_ = 0;

            // double push
            if (Square::value_distance(move.to(), move.from()) == 16) 
			{
                // imaginary attacks from the ep square from the pawn which moved
                Bitboard ep_mask = attacks::pawn(stm_, move.to().ep_square());

                // add enpassant hash if enemy pawns are attacking the square
                if (static_cast<bool>(ep_mask & pieces(PieceType::PAWN, ~stm_))) 
				{
                    int found = -1;

                    // check if the enemy can legally capture the pawn on the next move
                    if constexpr (false) {
                        const auto piece = at(move.from());

                        found = 0;

                        removePieceInternal(piece, move.from());
                        placePieceInternal(piece, move.to());

                        stm_ = ~stm_;

                        bool valid;

                        if (stm_ == Color::WHITE) {
                            valid = movegen::isEpSquareValid<Color::WHITE>(*this, move.to().ep_square());
                        }
                        else {
                            valid = movegen::isEpSquareValid<Color::BLACK>(*this, move.to().ep_square());
                        }

                        if (valid) found = 1;

                        // undo
                        stm_ = ~stm_;

                        removePieceInternal(piece, move.to());
                        placePieceInternal(piece, move.from());
                    }

                    if (found != 0) {
                        assert(at(move.to().ep_square()) == Piece::NONE);
                        ep_sq_ = move.to().ep_square();
                        key_ ^= Zobrist::enpassant(move.to().ep_square().file());
                    }
                }
            }
        }

        if (move.typeOf() == Move::CASTLING) {
            assert(at<PieceType>(move.from()) == PieceType::KING);
            assert(at<PieceType>(move.to()) == PieceType::ROOK);

            const bool king_side = move.to() > move.from();
            const auto rookTo    = Square::castling_rook_square(king_side, stm_);
            const auto kingTo    = Square::castling_king_square(king_side, stm_);

            const auto king = at(move.from());
            const auto rook = at(move.to());

            removePieceInternal(king, move.from());
            removePieceInternal(rook, move.to());

            assert(king == Piece(PieceType::KING, stm_));
            assert(rook == Piece(PieceType::ROOK, stm_));

            placePieceInternal(king, kingTo);
            placePieceInternal(rook, rookTo);

            key_ ^= Zobrist::piece(king, move.from()) ^ Zobrist::piece(king, kingTo);
            key_ ^= Zobrist::piece(rook, move.to()) ^ Zobrist::piece(rook, rookTo);
        } else if (move.typeOf() == Move::PROMOTION) {
            const auto piece_pawn = Piece(PieceType::PAWN, stm_);
            const auto piece_prom = Piece(move.promotionType(), stm_);

            removePieceInternal(piece_pawn, move.from());
            placePieceInternal(piece_prom, move.to());

            key_ ^= Zobrist::piece(piece_pawn, move.from()) ^ Zobrist::piece(piece_prom, move.to());
        } else {
            assert(at(move.from()) != Piece::NONE);
            assert(at(move.to()) == Piece::NONE);

            const auto piece = at(move.from());

            removePieceInternal(piece, move.from());
            placePieceInternal(piece, move.to());

            key_ ^= Zobrist::piece(piece, move.from()) ^ Zobrist::piece(piece, move.to());
        }

        if (move.typeOf() == Move::ENPASSANT) {
            assert(at<PieceType>(move.to().ep_square()) == PieceType::PAWN);

            const auto piece = Piece(PieceType::PAWN, ~stm_);

            removePieceInternal(piece, move.to().ep_square());

            key_ ^= Zobrist::piece(piece, move.to().ep_square());
        }

        key_ ^= Zobrist::sideToMove();
        stm_ = ~stm_;
    }

    void unmakeMove(const Move move) {
        const auto& prev = prev_states_[--plies_];

        ep_sq_ = prev.enpassant;
        cr_    = prev.castling;
        hfm_   = prev.half_moves;
        stm_   = ~stm_;

        if (move.typeOf() == Move::CASTLING) {
            const bool king_side    = move.to() > move.from();
            const auto rook_from_sq = Square(king_side ? File::FILE_F : File::FILE_D, move.from().rank());
            const auto king_to_sq   = Square(king_side ? File::FILE_G : File::FILE_C, move.from().rank());

            assert(at<PieceType>(rook_from_sq) == PieceType::ROOK);
            assert(at<PieceType>(king_to_sq) == PieceType::KING);

            const auto rook = at(rook_from_sq);
            const auto king = at(king_to_sq);

            removePieceInternal(rook, rook_from_sq);
            removePieceInternal(king, king_to_sq);

            assert(king == Piece(PieceType::KING, stm_));
            assert(rook == Piece(PieceType::ROOK, stm_));

            placePieceInternal(king, move.from());
            placePieceInternal(rook, move.to());

            key_ = prev.hash;

            return;
        } else if (move.typeOf() == Move::PROMOTION) {
            const auto pawn  = Piece(PieceType::PAWN, stm_);
            const auto piece = at(move.to());

            assert(piece.type() == move.promotionType());
            assert(piece.type() != PieceType::PAWN);
            assert(piece.type() != PieceType::KING);
            assert(piece.type() != PieceType::NONE);

            removePieceInternal(piece, move.to());
            placePieceInternal(pawn, move.from());

            if (prev.captured_piece != Piece::NONE) {
                assert(at(move.to()) == Piece::NONE);
                placePieceInternal(prev.captured_piece, move.to());
            }

            key_ = prev.hash;
            return;
        } else {
            assert(at(move.to()) != Piece::NONE);
            assert(at(move.from()) == Piece::NONE);

            const auto piece = at(move.to());

            removePieceInternal(piece, move.to());
            placePieceInternal(piece, move.from());
        }

        if (move.typeOf() == Move::ENPASSANT) {
            const auto pawn   = Piece(PieceType::PAWN, ~stm_);
            const auto pawnTo = static_cast<Square>(ep_sq_ ^ 8);

            assert(at(pawnTo) == Piece::NONE);

            placePieceInternal(pawn, pawnTo);
        } else if (prev.captured_piece != Piece::NONE) {
            assert(at(move.to()) == Piece::NONE);

            placePieceInternal(prev.captured_piece, move.to());
        }

        key_ = prev.hash;
    }

    /**
     * @brief Make a null move. (Switches the side to move)
     */
    void makeNullMove() {
        new (&prev_states_[plies_++]) State(key_, cr_, ep_sq_, hfm_, Piece::NONE);

        key_ ^= Zobrist::sideToMove();
        if (ep_sq_ != Square::underlying::NO_SQ) key_ ^= Zobrist::enpassant(ep_sq_.file());
        ep_sq_ = Square::underlying::NO_SQ;

        stm_ = ~stm_;
    }

    /**
     * @brief Unmake a null move. (Switches the side to move)
     */
    void unmakeNullMove() {
        const auto& prev = prev_states_[--plies_];

        ep_sq_ = prev.enpassant;
        cr_    = prev.castling;
        hfm_   = prev.half_moves;
        key_   = prev.hash;

        stm_ = ~stm_;
    }

    /**
     * @brief Get the occupancy bitboard for the color.
     * @param color
     * @return
     */
    [[nodiscard]] Bitboard us(Color color) const { return occ_bb_[color]; }

    /**
     * @brief Get the occupancy bitboard for the opposite color.
     * @param color
     * @return
     */
    [[nodiscard]] Bitboard them(Color color) const { return us(~color); }

    /**
     * @brief Get the occupancy bitboard for both colors.
     * Faster than calling all() or us(Color::WHITE) | us(Color::BLACK).
     * @return
     */
    [[nodiscard]] Bitboard occ() const { return occ_bb_[0] | occ_bb_[1]; }

    /**
     * @brief Get the occupancy bitboard for all pieces, should be only used internally.
     * @return
     */
    [[nodiscard]] Bitboard all() const { return us(Color::WHITE) | us(Color::BLACK); }

    /**
     * @brief Returns the square of the king for a certain color
     * @param color
     * @return
     */
    [[nodiscard]] Square kingSq(Color color) const {
        assert(pieces(PieceType::KING, color) != Bitboard(0));
        return pieces(PieceType::KING, color).lsb();
    }

    /**
     * @brief Returns all pieces of a certain type and color
     * @param type
     * @param color
     * @return
     */
    [[nodiscard]] Bitboard pieces(PieceType type, Color color) const { return pieces_bb_[type] & occ_bb_[color]; }

    /**
     * @brief Returns all pieces of a certain type
     * @param type
     * @return
     */
    [[nodiscard]] Bitboard pieces(PieceType type) const {
        return pieces_bb_[type];
    }

    /**
     * @brief Returns either the piece or the piece type on a square
     * @tparam T
     * @param sq
     * @return
     */
    template <typename T = Piece>
    [[nodiscard]] T at(Square sq) const {
        assert(sq.index() < 64 && sq.index() >= 0);

        if constexpr (std::is_same_v<T, PieceType>) {
            return board_[sq.index()].type();
        } else {
            return board_[sq.index()];
        }
    }

    /**
     * @brief Checks if a move is a capture, enpassant moves are also considered captures.
     * @param move
     * @return
     */
    bool isCapture(const Move move) const {
        return (at(move.to()) != Piece::NONE && move.typeOf() != Move::CASTLING) || move.typeOf() == Move::ENPASSANT;
    }

    /**
     * @brief Get the current zobrist hash key of the board
     * @return
     */
    [[nodiscard]] U64 hash() const { return key_; }
    [[nodiscard]] Color sideToMove() const { return stm_; }
    [[nodiscard]] Square enpassantSq() const { return ep_sq_; }
    [[nodiscard]] CastlingRights castlingRights() const { return cr_; }
    [[nodiscard]] std::uint32_t halfMoveClock() const { return hfm_; }

    /**
     * @brief Checks if the current position is a repetition, set this to 1 if
     * you are writing a chess engine.
     * @param count
     * @return
     */
	
	[[nodiscard]] bool isRepetition() const {
        // We start the loop from the back and go forward in moves, at most to the
        // last move which reset the half-move counter because repetitions cant
        // be across half-moves.
        const auto size = plies_;

        for (int i = size - 2; i >= 0 && i >= size - hfm_ - 1; i -= 2) 
		{
            if (prev_states_[i].hash == key_) 
				return true;
        }

        return false;
    }

    /**
     * @brief Checks if the current position is a draw by 50 move rule.
     * Keep in mind that by the rules of chess, if the position has 50 half
     * moves it's not necessarily a draw, since checkmate has higher priority,
     * call getHalfMoveDrawType,
     * to determine whether the position is a draw or checkmate.
     * @return
     */
    [[nodiscard]] bool isHalfMoveDraw() const { return hfm_ >= 100; }

    /**
     * @brief Only call this function if isHalfMoveDraw() returns true.
     * @return
     */
    [[nodiscard]] std::pair<GameResultReason, GameResult> getHalfMoveDrawType() const {
        Movelist movelist;
        movegen::legalmoves(movelist, *this);

        if (movelist.empty() && inCheck()) {
            return {GameResultReason::CHECKMATE, GameResult::LOSE};
        }

        return {GameResultReason::FIFTY_MOVE_RULE, GameResult::DRAW};
    }

    /**
     * @brief Basic check if the current position is a draw by insufficient material.
     * @return
     */
    [[nodiscard]] bool isInsufficientMaterial() const {
        const auto count = occ().count();

        if (count > 4) return false;

        // only kings, draw
        if (count == 2) return true;

        // only bishop + knight, cant mate
        if (count == 3) {
            if (pieces(PieceType::BISHOP, Color::WHITE) || pieces(PieceType::BISHOP, Color::BLACK)) return true;
            if (pieces(PieceType::KNIGHT, Color::WHITE) || pieces(PieceType::KNIGHT, Color::BLACK)) return true;
        }

        // same colored bishops, cant mate
        if (count == 4) {
            if (pieces(PieceType::BISHOP, Color::WHITE) && pieces(PieceType::BISHOP, Color::BLACK) &&
                Square::same_color(pieces(PieceType::BISHOP, Color::WHITE).lsb(),
                                   pieces(PieceType::BISHOP, Color::BLACK).lsb()))
                return true;

            // one side with two bishops which have the same color
            auto white_bishops = pieces(PieceType::BISHOP, Color::WHITE);
            auto black_bishops = pieces(PieceType::BISHOP, Color::BLACK);

            if (white_bishops.count() == 2) {
                if (Square::same_color(white_bishops.lsb(), white_bishops.msb())) return true;
            } else if (black_bishops.count() == 2) {
                if (Square::same_color(black_bishops.lsb(), black_bishops.msb())) return true;
            }
        }

        return false;
    }

    /**
     * @brief Checks if a square is attacked by the given color.
     * @param square
     * @param color
     * @return
     */
    [[nodiscard]] bool isAttacked(Square square, Color color) const {
        // cheap checks first
        if (attacks::pawn(~color, square) & pieces(PieceType::PAWN, color)) return true;
        if (attacks::knight(square) & pieces(PieceType::KNIGHT, color)) return true;
        if (attacks::king(square) & pieces(PieceType::KING, color)) return true;

        if (attacks::bishop(square, occ()) & (pieces(PieceType::BISHOP, color) | pieces(PieceType::QUEEN, color)))
            return true;

        if (attacks::rook(square, occ()) & (pieces(PieceType::ROOK, color) | pieces(PieceType::QUEEN, color)))
            return true;

        return false;
    }

    /**
     * @brief Checks if the current side to move is in check
     * @return
     */
    [[nodiscard]] bool inCheck() const { return isAttacked(kingSq(stm_), ~stm_); }

    /**
     * @brief Checks if the given color has at least 1 piece thats not pawn and not king
     * @param color
     * @return
     */
    [[nodiscard]] bool hasNonPawnMaterial(Color color) const {
        return bool(pieces(PieceType::KNIGHT, color) | pieces(PieceType::BISHOP, color) |
                    pieces(PieceType::ROOK, color) | pieces(PieceType::QUEEN, color));
    }

    /**
     * @brief Calculates the zobrist hash key of the board, expensive! Prefer using hash().
     * @return
     */
    [[nodiscard]] U64 zobrist() const {
        U64 hash_key = 0ULL;

        auto wPieces = us(Color::WHITE);
        auto bPieces = us(Color::BLACK);

        while (wPieces.getBits()) {
            const Square sq = wPieces.pop();
            hash_key ^= Zobrist::piece(at(sq), sq);
        }

        while (bPieces.getBits()) {
            const Square sq = bPieces.pop();
            hash_key ^= Zobrist::piece(at(sq), sq);
        }

        U64 ep_hash = 0ULL;
        if (ep_sq_ != Square::underlying::NO_SQ) ep_hash ^= Zobrist::enpassant(ep_sq_.file());

        U64 stm_hash = 0ULL;
        if (stm_ == Color::WHITE) stm_hash ^= Zobrist::sideToMove();

        U64 castling_hash = 0ULL;
        castling_hash ^= Zobrist::castling(cr_.hashIndex());

        return hash_key ^ ep_hash ^ stm_hash ^ castling_hash;
    }

    void compact()
    {
        for (int i = 0; i < hfm_; ++i)
            prev_states_[i] = prev_states_[i + plies_ - hfm_];
        plies_ = hfm_;
    }

   protected:
    State prev_states_[192];

    std::array<Bitboard, 6> pieces_bb_ = {};
    std::array<Bitboard, 2> occ_bb_    = {};
    std::array<Piece, 64> board_       = {};

    U64 key_           = 0ULL;
    CastlingRights cr_ = {};
    uint16_t plies_    = 0;
    Color stm_         = Color::WHITE;
    Square ep_sq_      = Square::underlying::NO_SQ;
    uint8_t hfm_       = 0;

   private:
    void removePieceInternal(Piece piece, Square sq) {
        assert(board_[sq.index()] == piece && piece != Piece::NONE);

        nnue::remove_accumulator(static_cast<int>(piece), sq.index());

        auto type  = piece.type();
        auto color = piece.color();
        auto index = sq.index();

        assert(type != PieceType::NONE);
        assert(color != Color::NONE);
        assert(index >= 0 && index < 64);

        pieces_bb_[type].clear(index);
        occ_bb_[color].clear(index);
        board_[index] = Piece::NONE;
    }

    void placePieceInternal(Piece piece, Square sq) {
        assert(board_[sq.index()] == Piece::NONE);

        nnue::add_accumulator(static_cast<int>(piece), sq.index());

        auto type  = piece.type();
        auto color = piece.color();
        auto index = sq.index();

        assert(type != PieceType::NONE);
        assert(color != Color::NONE);
        assert(index >= 0 && index < 64);

        pieces_bb_[type].set(index);
        occ_bb_[color].set(index);
        board_[index] = piece;
    }

    void setFenInternal(std::string_view fen) {
        occ_bb_.fill(0ULL);
        pieces_bb_.fill(0ULL);
        board_.fill(Piece::NONE);

        nnue::clear_accumulator();

        // find leading whitespaces and remove them
        while (fen[0] == ' ') fen.remove_prefix(1);

        const auto params     = split_string_view<6>(fen);
        const auto position   = params[0].has_value() ? *params[0] : "";
        const auto move_right = params[1].has_value() ? *params[1] : "w";
        const auto castling   = params[2].has_value() ? *params[2] : "-";
        const auto en_passant = params[3].has_value() ? *params[3] : "-";
        const auto half_move  = params[4].has_value() ? *params[4] : "0";
        const auto full_move  = params[5].has_value() ? *params[5] : "1";

        static auto parseStringViewToInt = [](std::string_view sv) -> std::optional<int> {
            if (!sv.empty() && sv.back() == ';') sv.remove_suffix(1);
            size_t pos;
            int value = std::stoi(std::string(sv), &pos);
            if (pos == sv.size()) return value;
            return std::nullopt;
        };

        // Half move clock
        hfm_ = parseStringViewToInt(half_move).value_or(0);

        // Full move number
        plies_ = parseStringViewToInt(full_move).value_or(1);

        plies_ = 0;
        ep_sq_ = en_passant == "-" ? Square::underlying::NO_SQ : Square(en_passant.data());
        stm_   = (move_right == "w") ? Color::WHITE : Color::BLACK;
        key_   = 0ULL;
        cr_.clear();

        if (stm_ == Color::BLACK) {
            //plies_++;
        } else {
            key_ ^= Zobrist::sideToMove();
        }

        auto square = 56;
        for (char curr : position) {
            if (isdigit(curr)) {
                square += (curr - '0');
            } else if (curr == '/') {
                square -= 16;
            } else {
                auto p = Piece(std::string_view(&curr, 1));
                
                placePieceInternal(p, Square(square));
                
                key_ ^= Zobrist::piece(p, Square(square));
                ++square;
            }
        }

        for (char i : castling) {
            if (i == '-') break;

            const auto king_side  = CastlingRights::Side::KING_SIDE;
            const auto queen_side = CastlingRights::Side::QUEEN_SIDE;

            
			if (i == 'K') cr_.setCastlingRight(Color::WHITE, king_side, File::FILE_H);
			if (i == 'Q') cr_.setCastlingRight(Color::WHITE, queen_side, File::FILE_A);
			if (i == 'k') cr_.setCastlingRight(Color::BLACK, king_side, File::FILE_H);
			if (i == 'q') cr_.setCastlingRight(Color::BLACK, queen_side, File::FILE_A);
        }

        // check if ep square itself is valid
        if (ep_sq_ != Square::underlying::NO_SQ && !((ep_sq_.rank() == Rank::RANK_3 && stm_ == Color::BLACK) ||
                                                     (ep_sq_.rank() == Rank::RANK_6 && stm_ == Color::WHITE))) {
            ep_sq_ = Square::underlying::NO_SQ;
        }

        // check if ep square is valid, i.e. if there is a pawn that can capture it
        if (ep_sq_ != Square::underlying::NO_SQ) {
            bool valid;

            if (stm_ == Color::WHITE) {
                valid = movegen::isEpSquareValid<Color::WHITE>(*this, ep_sq_);
            } else {
                valid = movegen::isEpSquareValid<Color::BLACK>(*this, ep_sq_);
            }

            if (!valid)
                ep_sq_ = Square::underlying::NO_SQ;
            else
                key_ ^= Zobrist::enpassant(ep_sq_.file());
        }

        key_ ^= Zobrist::castling(cr_.hashIndex());

        if (key_ != zobrist())
            print("ZOBRIST ERROR\n", 15);
    }

    template <int N>
    std::array<std::optional<std::string_view>, N> static split_string_view(std::string_view fen,
                                                                            char delimiter = ' ') {
        std::array<std::optional<std::string_view>, N> arr = {};

        std::size_t start = 0;
        std::size_t end   = 0;

        for (std::size_t i = 0; i < N; i++) {
            end = fen.find(delimiter, start);
            if (end == std::string::npos) {
                arr[i] = fen.substr(start);
                break;
            }
            arr[i] = fen.substr(start, end - start);
            start  = end + 1;
        }

        return arr;
    }
};

}  // namespace  chess

namespace chess {

template <Direction direction>
[[nodiscard]] inline constexpr Bitboard attacks::shift(const Bitboard b) {
    switch (direction) {
        case Direction::NORTH:
            return b << 8;
        case Direction::SOUTH:
            return b >> 8;
        case Direction::NORTH_WEST:
            return (b & ~MASK_FILE[0]) << 7;
        case Direction::WEST:
            return (b & ~MASK_FILE[0]) >> 1;
        case Direction::SOUTH_WEST:
            return (b & ~MASK_FILE[0]) >> 9;
        case Direction::NORTH_EAST:
            return (b & ~MASK_FILE[7]) << 9;
        case Direction::EAST:
            return (b & ~MASK_FILE[7]) << 1;
        case Direction::SOUTH_EAST:
            return (b & ~MASK_FILE[7]) >> 7;
    }

        // c++23
#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
    std::unreachable();
#endif

    assert(false);

    return {};
}

template <Color::underlying c>
[[nodiscard]] inline Bitboard attacks::pawnLeftAttacks(const Bitboard pawns) {
    return c == Color::WHITE ? (pawns << 7) & ~MASK_FILE[static_cast<int>(File::FILE_H)]
                             : (pawns >> 7) & ~MASK_FILE[static_cast<int>(File::FILE_A)];
}

template <Color::underlying c>
[[nodiscard]] inline Bitboard attacks::pawnRightAttacks(const Bitboard pawns) {
    return c == Color::WHITE ? (pawns << 9) & ~MASK_FILE[static_cast<int>(File::FILE_A)]
                             : (pawns >> 9) & ~MASK_FILE[static_cast<int>(File::FILE_H)];
}

[[nodiscard]] inline Bitboard attacks::pawn(Color c, Square sq) noexcept { return PawnAttacks[c][sq.index()]; }

[[nodiscard]] inline Bitboard attacks::knight(Square sq) noexcept { return KnightAttacks[sq.index()]; }

[[nodiscard]] inline Bitboard attacks::bishop(Square sq, Bitboard occupied) noexcept {
    return BishopTable[sq.index()].attacks[BishopTable[sq.index()](occupied)];
}

[[nodiscard]] inline Bitboard attacks::rook(Square sq, Bitboard occupied) noexcept {
    uint64_t vlookup = _pext_u64(occupied.getBits(), 0x01010101010100ULL << static_cast<int>(sq.file()));
    Bitboard ret = VRookAttacks[sq.index()][vlookup];
    uint64_t hlookup = _pext_u64(occupied.getBits(), 126ULL << static_cast<int>(sq.rank()) * 8);
    ret |= HRookAttacks[sq.index()][hlookup];
    return ret;
}

[[nodiscard]] inline Bitboard attacks::queen(Square sq, Bitboard occupied) noexcept {
    return bishop(sq, occupied) | rook(sq, occupied);
}

[[nodiscard]] inline Bitboard attacks::king(Square sq) noexcept { return KingAttacks[sq.index()]; }

[[nodiscard]] inline Bitboard attacks::attackers(const Board &board, Color color, Square square) noexcept {
    const auto queens   = board.pieces(PieceType::QUEEN, color);
    const auto occupied = board.occ();

    // using the fact that if we can attack PieceType from square, they can attack us back
    auto atks = (pawn(~color, square) & board.pieces(PieceType::PAWN, color));
    atks |= (knight(square) & board.pieces(PieceType::KNIGHT, color));
    atks |= (bishop(square, occupied) & (board.pieces(PieceType::BISHOP, color) | queens));
    atks |= (rook(square, occupied) & (board.pieces(PieceType::ROOK, color) | queens));
    atks |= (king(square) & board.pieces(PieceType::KING, color));

    return atks & occupied;
}

[[nodiscard]] inline Bitboard attacks::bishopAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0ULL;

    int r, f;

    int br = sq.rank();
    int bf = sq.file();

    for (r = br + 1, f = bf + 1; Square::is_valid(static_cast<Rank>(r), static_cast<File>(f)); r++, f++) {
        auto s = Square(static_cast<Rank>(r), static_cast<File>(f)).index();
        attacks.set(s);
        if (occupied.check(s)) break;
    }

    for (r = br - 1, f = bf + 1; Square::is_valid(static_cast<Rank>(r), static_cast<File>(f)); r--, f++) {
        auto s = Square(static_cast<Rank>(r), static_cast<File>(f)).index();
        attacks.set(s);
        if (occupied.check(s)) break;
    }

    for (r = br + 1, f = bf - 1; Square::is_valid(static_cast<Rank>(r), static_cast<File>(f)); r++, f--) {
        auto s = Square(static_cast<Rank>(r), static_cast<File>(f)).index();
        attacks.set(s);
        if (occupied.check(s)) break;
    }

    for (r = br - 1, f = bf - 1; Square::is_valid(static_cast<Rank>(r), static_cast<File>(f)); r--, f--) {
        auto s = Square(static_cast<Rank>(r), static_cast<File>(f)).index();
        attacks.set(s);
        if (occupied.check(s)) break;
    }

    return attacks;
}

[[nodiscard]] inline Bitboard attacks::rookAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0ULL;

    int r, f;

    int rr = sq.rank();
    int rf = sq.file();

    for (r = rr + 1; Square::is_valid(static_cast<Rank>(r), static_cast<File>(rf)); r++) {
        auto s = Square(static_cast<Rank>(r), static_cast<File>(rf)).index();
        attacks.set(s);
        if (occupied.check(s)) break;
    }

    for (r = rr - 1; Square::is_valid(static_cast<Rank>(r), static_cast<File>(rf)); r--) {
        auto s = Square(static_cast<Rank>(r), static_cast<File>(rf)).index();
        attacks.set(s);
        if (occupied.check(s)) break;
    }

    for (f = rf + 1; Square::is_valid(static_cast<Rank>(rr), static_cast<File>(f)); f++) {
        auto s = Square(static_cast<Rank>(rr), static_cast<File>(f)).index();
        attacks.set(s);
        if (occupied.check(s)) break;
    }

    for (f = rf - 1; Square::is_valid(static_cast<Rank>(rr), static_cast<File>(f)); f--) {
        auto s = Square(static_cast<Rank>(rr), static_cast<File>(f)).index();
        attacks.set(s);
        if (occupied.check(s)) break;
    }

    return attacks;
}

inline void attacks::initSliders(Square sq, Magic table[], U64 magic,
                                 const std::function<Bitboard(Square, Bitboard)> &attacks) {
    // The edges of the board are not considered for the attacks
    // i.e. for the sq h7 edges will be a1-h1, a1-a8, a8-h8, ignoring the edge of the current square
    const Bitboard edges = ((Bitboard(Rank::RANK_1) | Bitboard(Rank::RANK_8)) & ~Bitboard(sq.rank())) |
                           ((Bitboard(File::FILE_A) | Bitboard(File::FILE_H)) & ~Bitboard(sq.file()));

    U64 occ = 0ULL;

    auto &table_sq = table[sq.index()];

    table_sq.magic = magic;
    table_sq.mask  = (attacks(sq, occ) & ~edges).getBits();
    table_sq.shift = 64 - Bitboard(table_sq.mask).count();

    if (sq < 64 - 1) {
        table[sq.index() + 1].attacks = table_sq.attacks + (1ull << Bitboard(table_sq.mask).count());
    }

    do {
        table_sq.attacks[table_sq(occ)] = attacks(sq, occ);
        occ                             = (occ - table_sq.mask) & table_sq.mask;
    } while (occ);
}

inline void attacks::initAttacks() {
    BishopTable[0].attacks = BishopAttacks;

    for (int i = 0; i < 64; i++) {
        Square s = static_cast<Square>(i);
        initSliders(s, BishopTable, BishopMagics[i], bishopAttacks);
        
        for (U64 j = 0; j < 64; j++)
        {
            HRookAttacks[i][j] = rookAttacks(s, j * 2ULL * 0x0101010101010101ULL) & Bitboard(s.rank());
            VRookAttacks[i][j] = rookAttacks(s, 255ULL * (((j & 1) << 8) + ((j & 2) << 15) + ((j & 4) << 22) + ((j & 8) << 29) + ((j & 16) << 36) + ((j & 32) << 43))) & Bitboard(s.file());
        }
    }
}
}  // namespace chess



namespace chess {

inline auto movegen::init_squares_between() {
    std::array<std::array<Bitboard, 64>, 64> squares_between_bb{};
    Bitboard sqs = 0;

    for (Square sq1 = 0; sq1 < 64; ++sq1) {
        for (Square sq2 = 0; sq2 < 64; ++sq2) {
            sqs = Bitboard::fromSquare(sq1) | Bitboard::fromSquare(sq2);
            if (sq1 == sq2)
                squares_between_bb[sq1.index()][sq2.index()].clear();
            else if (sq1.file() == sq2.file() || sq1.rank() == sq2.rank())
                squares_between_bb[sq1.index()][sq2.index()] = attacks::rook(sq1, sqs) & attacks::rook(sq2, sqs);
            else if (sq1.diagonal_of() == sq2.diagonal_of() || sq1.antidiagonal_of() == sq2.antidiagonal_of())
                squares_between_bb[sq1.index()][sq2.index()] = attacks::bishop(sq1, sqs) & attacks::bishop(sq2, sqs);
        }
    }

    return squares_between_bb;
}

template <Color::underlying c>
[[nodiscard]] inline std::pair<Bitboard, int> movegen::checkMask(const Board &board, Square sq) {
    const auto opp_knight = board.pieces(PieceType::KNIGHT, ~c);
    const auto opp_bishop = board.pieces(PieceType::BISHOP, ~c);
    const auto opp_rook   = board.pieces(PieceType::ROOK, ~c);
    const auto opp_queen  = board.pieces(PieceType::QUEEN, ~c);

    const auto opp_pawns = board.pieces(PieceType::PAWN, ~c);

    int checks = 0;

    // check for knight checks
    Bitboard knight_attacks = attacks::knight(sq) & opp_knight;
    checks += bool(knight_attacks);

    Bitboard mask = knight_attacks;

    // check for pawn checks
    Bitboard pawn_attacks = attacks::pawn(board.sideToMove(), sq) & opp_pawns;
    mask |= pawn_attacks;
    checks += bool(pawn_attacks);

    // check for bishop checks
    Bitboard bishop_attacks = attacks::bishop(sq, board.occ()) & (opp_bishop | opp_queen);

    if (bishop_attacks) {
        const auto index = bishop_attacks.lsb();

        mask |= SQUARES_BETWEEN_BB[sq.index()][index] | Bitboard::fromSquare(index);
        checks++;
    }

    Bitboard rook_attacks = attacks::rook(sq, board.occ()) & (opp_rook | opp_queen);

    if (rook_attacks) {
        if (rook_attacks.count() > 1) {
            checks = 2;
            return {mask, checks};
        }

        const auto index = rook_attacks.lsb();

        mask |= SQUARES_BETWEEN_BB[sq.index()][index] | Bitboard::fromSquare(index);
        checks++;
    }

    if (!mask) {
        return {constants::DEFAULT_CHECKMASK, checks};
    }

    return {mask, checks};
}

template <Color::underlying c>
[[nodiscard]] inline Bitboard movegen::pinMaskRooks(const Board &board, Square sq, Bitboard occ_opp, Bitboard occ_us) {
    const auto opp_rook  = board.pieces(PieceType::ROOK, ~c);
    const auto opp_queen = board.pieces(PieceType::QUEEN, ~c);

    Bitboard rook_attacks = attacks::rook(sq, occ_opp) & (opp_rook | opp_queen);
    Bitboard pin_hv       = 0;

    while (rook_attacks) {
        const auto index = rook_attacks.pop();

        const Bitboard possible_pin = SQUARES_BETWEEN_BB[sq.index()][index] | Bitboard::fromSquare(index);
        if ((possible_pin & occ_us).count() == 1) pin_hv |= possible_pin;
    }

    return pin_hv;
}

template <Color::underlying c>
[[nodiscard]] inline Bitboard movegen::pinMaskBishops(const Board &board, Square sq, Bitboard occ_opp,
                                                      Bitboard occ_us) {
    const auto opp_bishop = board.pieces(PieceType::BISHOP, ~c);
    const auto opp_queen  = board.pieces(PieceType::QUEEN, ~c);

    Bitboard bishop_attacks = attacks::bishop(sq, occ_opp) & (opp_bishop | opp_queen);
    Bitboard pin_diag       = 0;

    while (bishop_attacks) {
        const auto index = bishop_attacks.pop();

        const Bitboard possible_pin = SQUARES_BETWEEN_BB[sq.index()][index] | Bitboard::fromSquare(index);
        if ((possible_pin & occ_us).count() == 1) pin_diag |= possible_pin;
    }

    return pin_diag;
}

template <Color::underlying c>
[[nodiscard]] inline Bitboard movegen::seenSquares(const Board &board, Bitboard enemy_empty) {
    auto king_sq          = board.kingSq(~c);
    Bitboard map_king_atk = attacks::king(king_sq) & enemy_empty;

    if (map_king_atk == Bitboard(0ull)) {
        return 0ull;
    }

    auto occ     = board.occ() & ~Bitboard::fromSquare(king_sq);
    auto queens  = board.pieces(PieceType::QUEEN, c);
    auto pawns   = board.pieces(PieceType::PAWN, c);
    auto knights = board.pieces(PieceType::KNIGHT, c);
    auto bishops = board.pieces(PieceType::BISHOP, c) | queens;
    auto rooks   = board.pieces(PieceType::ROOK, c) | queens;

    Bitboard seen = attacks::pawnLeftAttacks<c>(pawns) | attacks::pawnRightAttacks<c>(pawns);

    while (knights) {
        const auto index = knights.pop();
        seen |= attacks::knight(index);
    }

    while (bishops) {
        const auto index = bishops.pop();
        seen |= attacks::bishop(index, occ);
    }

    while (rooks) {
        const auto index = rooks.pop();
        seen |= attacks::rook(index, occ);
    }

    const Square index = board.kingSq(c);
    seen |= attacks::king(index);

    return seen;
}

template <Color::underlying c>
inline void movegen::generatePawnMoves(const Board &board, MoveGenType mt, Movelist &moves, Bitboard pin_d, Bitboard pin_hv,
                                       Bitboard checkmask, Bitboard occ_opp) {
    // flipped for black

    constexpr auto UP         = make_direction(Direction::NORTH, c);
    constexpr auto DOWN       = make_direction(Direction::SOUTH, c);
    constexpr auto DOWN_LEFT  = make_direction(Direction::SOUTH_WEST, c);
    constexpr auto DOWN_RIGHT = make_direction(Direction::SOUTH_EAST, c);
    constexpr auto UP_LEFT    = make_direction(Direction::NORTH_WEST, c);
    constexpr auto UP_RIGHT   = make_direction(Direction::NORTH_EAST, c);

    constexpr auto RANK_B_PROMO     = Rank::rank(Rank::RANK_7, c).bb();
    constexpr auto RANK_PROMO       = Rank::rank(Rank::RANK_8, c).bb();
    constexpr auto DOUBLE_PUSH_RANK = Rank::rank(Rank::RANK_3, c).bb();

    const auto pawns = board.pieces(PieceType::PAWN, c);

    // These pawns can maybe take Left or Right
    const Bitboard pawns_lr          = pawns & ~pin_hv;
    const Bitboard unpinned_pawns_lr = pawns_lr & ~pin_d;
    const Bitboard pinned_pawns_lr   = pawns_lr & pin_d;

    auto l_pawns = attacks::shift<UP_LEFT>(unpinned_pawns_lr) | (attacks::shift<UP_LEFT>(pinned_pawns_lr) & pin_d);
    auto r_pawns = attacks::shift<UP_RIGHT>(unpinned_pawns_lr) | (attacks::shift<UP_RIGHT>(pinned_pawns_lr) & pin_d);

    // Prune moves that don't capture a piece and are not on the checkmask.
    l_pawns &= occ_opp & checkmask;
    r_pawns &= occ_opp & checkmask;

    // These pawns can walk Forward
    const auto pawns_hv = pawns & ~pin_d;

    const auto pawns_pinned_hv   = pawns_hv & pin_hv;
    const auto pawns_unpinned_hv = pawns_hv & ~pin_hv;

    // Prune moves that are blocked by a piece
    const auto single_push_unpinned = attacks::shift<UP>(pawns_unpinned_hv) & ~board.occ();
    const auto single_push_pinned   = attacks::shift<UP>(pawns_pinned_hv) & pin_hv & ~board.occ();

    // Prune moves that are not on the checkmask.
    Bitboard single_push = (single_push_unpinned | single_push_pinned) & checkmask;

    Bitboard double_push = ((attacks::shift<UP>(single_push_unpinned & DOUBLE_PUSH_RANK) & ~board.occ()) |
                            (attacks::shift<UP>(single_push_pinned & DOUBLE_PUSH_RANK) & ~board.occ())) &
                           checkmask;

    if (pawns & RANK_B_PROMO) {
        Bitboard promo_left  = l_pawns & RANK_PROMO;
        Bitboard promo_right = r_pawns & RANK_PROMO;
        Bitboard promo_push  = single_push & RANK_PROMO;

        // Skip capturing promotions if we are only generating quiet moves.
        // Generates at ALL and CAPTURE
        while (mt != MoveGenType::QUIET && promo_left) {
            const auto index = promo_left.pop();
            moves.add(Move::make<Move::PROMOTION>(index + DOWN_RIGHT, index, PieceType::QUEEN));
            moves.add(Move::make<Move::PROMOTION>(index + DOWN_RIGHT, index, PieceType::ROOK));
            moves.add(Move::make<Move::PROMOTION>(index + DOWN_RIGHT, index, PieceType::BISHOP));
            moves.add(Move::make<Move::PROMOTION>(index + DOWN_RIGHT, index, PieceType::KNIGHT));
        }

        // Skip capturing promotions if we are only generating quiet moves.
        // Generates at ALL and CAPTURE
        while (mt != MoveGenType::QUIET && promo_right) {
            const auto index = promo_right.pop();
            moves.add(Move::make<Move::PROMOTION>(index + DOWN_LEFT, index, PieceType::QUEEN));
            moves.add(Move::make<Move::PROMOTION>(index + DOWN_LEFT, index, PieceType::ROOK));
            moves.add(Move::make<Move::PROMOTION>(index + DOWN_LEFT, index, PieceType::BISHOP));
            moves.add(Move::make<Move::PROMOTION>(index + DOWN_LEFT, index, PieceType::KNIGHT));
        }

        // Skip quiet promotions if we are only generating captures.
        // Generates at ALL and QUIET
        while (mt != MoveGenType::CAPTURE && promo_push) {
            const auto index = promo_push.pop();
            moves.add(Move::make<Move::PROMOTION>(index + DOWN, index, PieceType::QUEEN));
            moves.add(Move::make<Move::PROMOTION>(index + DOWN, index, PieceType::ROOK));
            moves.add(Move::make<Move::PROMOTION>(index + DOWN, index, PieceType::BISHOP));
            moves.add(Move::make<Move::PROMOTION>(index + DOWN, index, PieceType::KNIGHT));
        }
    }

    single_push &= ~RANK_PROMO;
    l_pawns &= ~RANK_PROMO;
    r_pawns &= ~RANK_PROMO;

    while (mt != MoveGenType::QUIET && l_pawns) {
        const auto index = l_pawns.pop();
        moves.add(Move::make<Move::NORMAL>(index + DOWN_RIGHT, index));
    }

    while (mt != MoveGenType::QUIET && r_pawns) {
        const auto index = r_pawns.pop();
        moves.add(Move::make<Move::NORMAL>(index + DOWN_LEFT, index));
    }

    while (mt != MoveGenType::CAPTURE && single_push) {
        const auto index = single_push.pop();
        moves.add(Move::make<Move::NORMAL>(index + DOWN, index));
    }

    while (mt != MoveGenType::CAPTURE && double_push) {
        const auto index = double_push.pop();
        moves.add(Move::make<Move::NORMAL>(index + DOWN + DOWN, index));
    }

    if (mt == MoveGenType::QUIET) return;

    const Square ep = board.enpassantSq();

    if (ep != Square::underlying::NO_SQ) {
        auto m = generateEPMove(board, checkmask, pin_d, pawns_lr, ep, c);

        for (const auto &move : m) {
            if (move != Move::NO_MOVE) moves.add(move);
        }
    }
}

[[nodiscard]] inline std::array<Move, 2> movegen::generateEPMove(const Board &board, Bitboard checkmask, Bitboard pin_d,
                                                                 Bitboard pawns_lr, Square ep, Color c) {
    assert((ep.rank() == Rank::RANK_3 && board.sideToMove() == Color::BLACK) ||
           (ep.rank() == Rank::RANK_6 && board.sideToMove() == Color::WHITE));

    std::array<Move, 2> moves = {Move::NO_MOVE, Move::NO_MOVE};
    auto i                    = 0;

    const auto DOWN     = make_direction(Direction::SOUTH, c);
    const auto epPawnSq = ep + DOWN;

    /*
     In case the en passant square and the enemy pawn
     that just moved are not on the checkmask
     en passant is not available.
    */
    if ((checkmask & (Bitboard::fromSquare(epPawnSq) | Bitboard::fromSquare(ep))) == Bitboard(0)) return moves;

    const Square kSQ              = board.kingSq(c);
    const Bitboard kingMask       = Bitboard::fromSquare(kSQ) & epPawnSq.rank().bb();
    const Bitboard enemyQueenRook = board.pieces(PieceType::ROOK, ~c) | board.pieces(PieceType::QUEEN, ~c);

    auto epBB = attacks::pawn(~c, ep) & pawns_lr;

    // For one en passant square two pawns could potentially take there.
    while (epBB) {
        const auto from = epBB.pop();
        const auto to   = ep;

        /*
         If the pawn is pinned but the en passant square is not on the
         pin mask then the move is illegal.
        */
        if ((Bitboard::fromSquare(from) & pin_d) && !(pin_d & Bitboard::fromSquare(ep))) continue;

        const auto connectingPawns = Bitboard::fromSquare(epPawnSq) | Bitboard::fromSquare(from);

        /*
         7k/4p3/8/2KP3r/8/8/8/8 b - - 0 1
         If e7e5 there will be a potential ep square for us on e6.
         However, we cannot take en passant because that would put our king
         in check. For this scenario we check if there's an enemy rook/queen
         that would give check if the two pawns were removed.
         If that's the case then the move is illegal and we can break immediately.
        */
        const auto isPossiblePin = kingMask && enemyQueenRook;

        if (isPossiblePin && (attacks::rook(kSQ, board.occ() & ~connectingPawns) & enemyQueenRook) != Bitboard(0))
            break;

        moves[i++] = Move::make<Move::ENPASSANT>(from, to);
    }

    return moves;
}

[[nodiscard]] inline Bitboard movegen::generateKnightMoves(Square sq) { return attacks::knight(sq); }

[[nodiscard]] inline Bitboard movegen::generateBishopMoves(Square sq, Bitboard pin_d, Bitboard occ_all) {
    // The Bishop is pinned diagonally thus can only move diagonally.
    if (pin_d & Bitboard::fromSquare(sq)) return attacks::bishop(sq, occ_all) & pin_d;
    return attacks::bishop(sq, occ_all);
}

[[nodiscard]] inline Bitboard movegen::generateRookMoves(Square sq, Bitboard pin_hv, Bitboard occ_all) {
    // The Rook is pinned horizontally thus can only move horizontally.
    if (pin_hv & Bitboard::fromSquare(sq)) return attacks::rook(sq, occ_all) & pin_hv;
    return attacks::rook(sq, occ_all);
}

[[nodiscard]] inline Bitboard movegen::generateQueenMoves(Square sq, Bitboard pin_d, Bitboard pin_hv,
                                                          Bitboard occ_all) {
    Bitboard moves = 0ULL;

    if (pin_d & Bitboard::fromSquare(sq))
        moves |= attacks::bishop(sq, occ_all) & pin_d;
    else if (pin_hv & Bitboard::fromSquare(sq))
        moves |= attacks::rook(sq, occ_all) & pin_hv;
    else {
        moves |= attacks::rook(sq, occ_all);
        moves |= attacks::bishop(sq, occ_all);
    }

    return moves;
}

[[nodiscard]] inline Bitboard movegen::generateKingMoves(Square sq, Bitboard seen, Bitboard movable_square) {
    return attacks::king(sq) & movable_square & ~seen;
}

template <Color::underlying c>
[[nodiscard]] inline Bitboard movegen::generateCastleMoves(const Board &board, Square sq, Bitboard seen,
                                                           Bitboard pin_hv) {
    if (!Square::back_rank(sq, c) || !board.castlingRights().has(c)) return 0ull;

    const auto rights = board.castlingRights();

    Bitboard moves = 0ull;

    for (const auto side : {Board::CastlingRights::Side::KING_SIDE, Board::CastlingRights::Side::QUEEN_SIDE}) {
        if (!rights.has(c, side)) continue;

        const auto end_king_sq = Square::castling_king_square(side == Board::CastlingRights::Side::KING_SIDE, c);
        const auto end_rook_sq = Square::castling_rook_square(side == Board::CastlingRights::Side::KING_SIDE, c);

        const auto from_rook_sq = Square(rights.getRookFile(c, side), sq.rank());

        const Bitboard not_occ_path       = SQUARES_BETWEEN_BB[sq.index()][from_rook_sq.index()];
        const Bitboard not_attacked_path  = SQUARES_BETWEEN_BB[sq.index()][end_king_sq.index()];
        const Bitboard empty_not_attacked = ~seen & ~(board.occ() & Bitboard(~Bitboard::fromSquare(from_rook_sq)));
        const Bitboard withoutRook        = board.occ() & Bitboard(~Bitboard::fromSquare(from_rook_sq));
        const Bitboard withoutKing        = board.occ() & Bitboard(~Bitboard::fromSquare(sq));

        if ((not_attacked_path & empty_not_attacked) == not_attacked_path &&
            ((not_occ_path & ~board.occ()) == not_occ_path) &&
            !(Bitboard::fromSquare(from_rook_sq) & pin_hv.getBits() & sq.rank().bb()) &&
            !(Bitboard::fromSquare(end_rook_sq) & (withoutRook & withoutKing).getBits()) &&
            !(Bitboard::fromSquare(end_king_sq) &
              (seen | (withoutRook & Bitboard(~Bitboard::fromSquare(sq)))).getBits())) {
            moves |= Bitboard::fromSquare(from_rook_sq);
        }
    }

    return moves;
}

template <typename T>
inline void movegen::whileBitboardAdd(Movelist &movelist, Bitboard mask, T func) {
    while (mask) {
        const Square from = mask.pop();
        auto moves        = func(from);
        while (moves) {
            const Square to = moves.pop();
            movelist.add(Move::make<Move::NORMAL>(from, to));
        }
    }
}

template <Color::underlying c>
inline void movegen::legalmoves(Movelist &movelist, const Board &board, movegen::MoveGenType mt, int pieces) {
    /*
     The size of the movelist might not
     be 0! This is done on purpose since it enables
     you to append new move types to any movelist.
    */
    auto king_sq = board.kingSq(c);

    Bitboard occ_us  = board.us(c);
    Bitboard occ_opp = board.us(~c);
    Bitboard occ_all = occ_us | occ_opp;

    Bitboard opp_empty = ~occ_us;

    const auto [checkmask, checks] = checkMask<c>(board, king_sq);
    const auto pin_hv              = pinMaskRooks<c>(board, king_sq, occ_opp, occ_us);
    const auto pin_d               = pinMaskBishops<c>(board, king_sq, occ_opp, occ_us);

    assert(checks <= 2);

    // Moves have to be on the checkmask
    Bitboard movable_square;

    // Slider, Knights and King moves can only go to enemy or empty squares.
    if (mt == MoveGenType::ALL)
        movable_square = opp_empty;
    else if (mt == MoveGenType::CAPTURE)
        movable_square = occ_opp;
    else  // QUIET moves
        movable_square = ~occ_all;

    if (pieces & PieceGenType::KING) {
        Bitboard seen = seenSquares<~c>(board, opp_empty);

        whileBitboardAdd(movelist, Bitboard::fromSquare(king_sq),
                         [&](Square sq) { return generateKingMoves(sq, seen, movable_square); });

        if (checks == 0 && mt != MoveGenType::CAPTURE) {
            Bitboard moves_bb = generateCastleMoves<c>(board, king_sq, seen, pin_hv);

            while (moves_bb) {
                Square to = moves_bb.pop();
                movelist.add(Move::make<Move::CASTLING>(king_sq, to));
            }
        }
    }

    movable_square &= checkmask;

    // Early return for double check as described earlier
    if (checks == 2) return;

    // Add the moves to the movelist.
    if (pieces & PieceGenType::PAWN) {
        generatePawnMoves<c>(board, mt, movelist, pin_d, pin_hv, checkmask, occ_opp);
    }

    if (pieces & PieceGenType::KNIGHT) {
        // Prune knights that are pinned since these cannot move.
        Bitboard knights_mask = board.pieces(PieceType::KNIGHT, c) & ~(pin_d | pin_hv);

        whileBitboardAdd(movelist, knights_mask, [&](Square sq) { return generateKnightMoves(sq) & movable_square; });
    }

    if (pieces & PieceGenType::BISHOP) {
        // Prune horizontally pinned bishops
        Bitboard bishops_mask = board.pieces(PieceType::BISHOP, c) & ~pin_hv;

        whileBitboardAdd(movelist, bishops_mask,
                         [&](Square sq) { return generateBishopMoves(sq, pin_d, occ_all) & movable_square; });
    }

    if (pieces & PieceGenType::ROOK) {
        //  Prune diagonally pinned rooks
        Bitboard rooks_mask = board.pieces(PieceType::ROOK, c) & ~pin_d;

        whileBitboardAdd(movelist, rooks_mask,
                         [&](Square sq) { return generateRookMoves(sq, pin_hv, occ_all) & movable_square; });
    }

    if (pieces & PieceGenType::QUEEN) {
        // Prune double pinned queens
        Bitboard queens_mask = board.pieces(PieceType::QUEEN, c) & ~(pin_d & pin_hv);

        whileBitboardAdd(movelist, queens_mask,
                         [&](Square sq) { return generateQueenMoves(sq, pin_d, pin_hv, occ_all) & movable_square; });
    }
}

inline void movegen::legalmoves(Movelist &movelist, const Board &board, movegen::MoveGenType mt, int pieces) {
    if (board.sideToMove() == Color::WHITE)
        legalmoves<Color::WHITE>(movelist, board, mt, pieces);
    else
        legalmoves<Color::BLACK>(movelist, board, mt, pieces);
}

template <Color::underlying c>
inline bool movegen::isEpSquareValid(const Board &board, Square ep) {
    const auto stm = board.sideToMove();

    Bitboard occ_us  = board.us(stm);
    Bitboard occ_opp = board.us(~stm);
    auto king_sq     = board.kingSq(stm);

    const auto [checkmask, checks] = movegen::checkMask<c>(board, king_sq);
    const auto pin_hv              = movegen::pinMaskRooks<c>(board, king_sq, occ_opp, occ_us);
    const auto pin_d               = movegen::pinMaskBishops<c>(board, king_sq, occ_opp, occ_us);

    const auto pawns    = board.pieces(PieceType::PAWN, stm);
    const auto pawns_lr = pawns & ~pin_hv;
    const auto m        = movegen::generateEPMove(board, checkmask, pin_d, pawns_lr, ep, stm);
    bool found          = false;

    for (const auto &move : m) {
        if (move != Move::NO_MOVE) {
            found = true;
            break;
        }
    }

    return found;
}

inline const std::array<std::array<Bitboard, 64>, 64> movegen::SQUARES_BETWEEN_BB = [] {
    attacks::initAttacks();
    return movegen::init_squares_between();
}();

}  // namespace chess

namespace chess {
class uci {
   public:
    /**
     * @brief Converts an internal move to a UCI string
     * @param move
     * @return
     */
    static void moveToUci(const Move &move) noexcept(false) {
        // Get the from and to squares
        Square from_sq = move.from();
        Square to_sq   = move.to();

        // If the move is not a castling move and is a king moving more than one square,
        // update the to square to be the correct square for a regular castling move
        if (move.typeOf() == Move::CASTLING) {
            to_sq = Square(to_sq > from_sq ? File::FILE_G : File::FILE_C, from_sq.rank());
        }

		char out[6];
		
		out[0] = 'a' + from_sq.file();
		out[1] = '1' + from_sq.rank();
		out[2] = 'a' + to_sq.file();
		out[3] = '1' + to_sq.rank();
		
		if (move.typeOf() == Move::PROMOTION)
		{
			out[4] = "nbrq"[static_cast<int>(move.promotionType()) - static_cast<int>(PieceType::underlying::KNIGHT)];
		}
		
		print(out, move.typeOf() == Move::PROMOTION ? 5 : 4);
		print("\n", 1);
    }

    /**
     * @brief Converts a UCI string to an internal move.
     * @param board
     * @param uci
     * @return
     */
    [[nodiscard]] static Move uciToMove(const Board &board, const char * uci) noexcept(false) {
        Square source = Square(uci);
        Square target = Square(uci+2);

        if (!source.is_valid() || !target.is_valid()) {
			print_err("Bad move\n", 9);
			print_err(uci, 64);
            exit(-1);
        }

        PieceType piece = board.at(source).type();

        // convert to king captures rook
        if (piece == PieceType::KING && Square::distance(target, source) == 2) {
            target = Square(target > source ? File::FILE_H : File::FILE_A, source.rank());
            return Move::make<Move::CASTLING>(source, target);
        }

        // en passant
        if (piece == PieceType::PAWN && target == board.enpassantSq()) {
            return Move::make<Move::ENPASSANT>(source, target);
        }

        // promotion
        if (piece == PieceType::PAWN && uci[4] > 'a' && Square::back_rank(target, ~board.sideToMove())) {
            auto promotion = PieceType(uci[4]);

            if (promotion != PieceType::QUEEN && promotion != PieceType::ROOK && promotion != PieceType::BISHOP &&
                promotion != PieceType::KNIGHT) {
				print("Bad move\n", 9);
                return Move::NO_MOVE;
            }

            return Move::make<Move::PROMOTION>(source, target, promotion);
        }

        return Move::make<Move::NORMAL>(source, target);
    }
};

#ifdef DEBUGGING
const char* move_str(Move m)
{
    static char out[6];
    if (m == Move::NO_MOVE)
    {
        out[0] = 'X';
        out[1] = 0;
        return out;
    }

    out[0] = 'a' + m.from().file();
    out[1] = '1' + m.from().rank();
    out[2] = 'a' + m.to().file();
    out[3] = '1' + m.to().rank();

    if (m.typeOf() == Move::PROMOTION)
    {
        out[4] = "nbrq"[static_cast<int>(m.promotionType()) - static_cast<int>(PieceType::underlying::KNIGHT)];
    }

    out[m.typeOf() == Move::PROMOTION ? 5 : 4] = 0;

    return out;
}
#endif


}  // namespace chess

#endif
