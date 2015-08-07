#include <cstdint>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <string>
#include <array>
#include <vector>
#include <memory>
#include <chrono>
#include <queue>
#include <stack>

auto pre_load (std::istream& is, std::size_t n = 0u) {
    static const auto bad_format =
        std::runtime_error ("Bad input format");
    typedef std::array<std::uint8_t, 81u> puzzle_grid_type;
    static const auto load = [] (const std::string& line) {
        puzzle_grid_type out;
        std::transform (
            std::begin (line), std::end (line),
            std::begin (out), [] (auto x) {
            return std::uint8_t (x == '.' ? 0u : x - '0');
        });
        return out;
    };

    std::string line;  
    if (n > 0u) {
        std::vector<puzzle_grid_type> out;
        out.reserve (n);
        for (auto i = 0u; std::getline (is, line) && i < n; ++i) {
            if (line.size () != 81u)
                throw bad_format;
            out.emplace_back (load (line));
        }
        return out;
    }
    std::vector<puzzle_grid_type> out;
    while (std::getline (is, line)) {
        if (line.size () != 81u)
            throw bad_format;
        out.emplace_back (load (line));
    }
    return out;
}

template <typename _Grid>
auto print (const _Grid& grid) {
    for (auto j = 0u; j < 9u; ++j) {
        if (j % 3 == 0 && j != 0)
            std::cout << "-------+-------+------\n";
        for (auto i = 0u; i < 9u; ++i) {
            std::cout << (i % 3 == 0 && i != 0 ? " | " : " ");
            if (unsigned (grid [i + j*9u]) == 0) {
                std::cout << ".";
            } else {
                std::cout << unsigned (grid [i + j*9u]);
            }
        }
        std::cout << "\n";
    }
}

struct sudoku_grid {    
    struct cell_type {
        static const auto INVALID_ALL = std::uint16_t (0x1ff);

        cell_type ()
            :_value (0u), _mask (0u)
        {}

        cell_type (std::uint8_t v)
            :_value (v), _mask (v != 0 ? INVALID_ALL : 0) 
        {}

        cell_type& operator = (std::uint8_t v) { 
            return *this = cell_type (v); 
        }

        auto invalid () const { 
            return _mask; 
        }

        auto value () const { 
            return std::uint8_t (_value); 
        }

        auto invalid (std::uint16_t v) { 
            return _mask = v; 
        }

        auto value (std::uint8_t v) { 
            _mask = (v != 0 ? INVALID_ALL : 0);
            return _value = v; 
        }

        auto is_set () const { 
            return value () != 0u; 
        }

        auto empty () const { 
            return !is_set () && invalid () == INVALID_ALL; 
        }

        auto& invalidate (std::uint8_t v) { 
            if (v < 1 || v > 9)
                return *this;
            _mask |= (1 << (v - 1u)); 
            return *this; 
        }

        auto& invalidate_set (std::uint16_t v) { 
            _mask |= (v & INVALID_ALL); 
            return *this; 
        }

        auto is_valid (std::uint8_t v) const { 
            if (v < 1 || v > 9)
                return false;
            if (is_set ())
                return value () == v;
            auto j = _mask & (1u << (v - 1u));
            return !(j);
        }

        cell_type& operator = (const cell_type&) = default;
        cell_type (const cell_type&) = default;

        operator unsigned () const {
            return value ();
        }
     
    private:
        std::uint8_t _value;
        std::uint16_t _mask;
    };
    
    static inline auto map (std::uint32_t i, std::uint32_t j) {
        static const std::uint32_t mapping [9u] [9u] = {
            { 0,  1,  2,  9, 10, 11, 18, 19, 20},
            { 3,  4,  5, 12, 13, 14, 21, 22, 23},
            { 6,  7,  8, 15, 16, 17, 24, 25, 26},
            {27, 28, 29, 36, 37, 38, 45, 46, 47},
            {30, 31, 32, 39, 40, 41, 48, 49, 50},
            {33, 34, 35, 42, 43, 44, 51, 52, 53},
            {54, 55, 56, 63, 64, 65, 72, 73, 74},
            {57, 58, 59, 66, 67, 68, 75, 76, 77},
            {60, 61, 62, 69, 70, 71, 78, 79, 80}
        };    
        return mapping [i] [j];
    }

    auto&& cell (std::uint32_t i)                           { return grid [i]; }
    auto&& cell (std::uint32_t i) const                     { return grid [i]; }
    auto&& cell (std::uint32_t i, std::uint32_t j)          { return grid [map (i, j)]; }
    auto&& cell (std::uint32_t i, std::uint32_t j) const    { return grid [map (i, j)]; }
    auto is_good () const { return _is_good; }
    auto good (bool b) { _is_good = b; }


    auto is_valid () const {
        for (auto i = 0u; i < 9u; ++i) {
            unsigned b [3u][9u] = {
                {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
                {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
                {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}
            };
            for (auto j = 0u; j < 9u; ++j) {
                if ((cell (j*9u + i).value () && ++b [0u] [cell (j*9u + i).value () - 1u] > 1u) ||
                    (cell (i*9u + j).value () && ++b [1u] [cell (i*9u + j).value () - 1u] > 1u) ||
                    (cell (i, j    ).value () && ++b [2u] [cell (i, j    ).value () - 1u] > 1u)) 
                {
                    return false;
                }
            }
        }
        return true;
    }
protected:
    auto prune () {
        sudoku_grid::cell_type row [9u], col [9u], blk [9u];
        for (auto i = 0u; i < 9u; ++i) {
            for (auto j = 0u; j < 9u; ++j) {
                row [i].invalidate (cell (i*9u + j).value ());
                col [i].invalidate (cell (j*9u + i).value ());
                blk [i].invalidate (cell (i, j).value ());
            }
        }

        for (auto i = 0u; i < 9u; ++i) {
            for (auto j = 0u; j < 9u; ++j) {
                auto& rcell = cell (i*9u + j);
                rcell.invalidate_set (row [i].invalid ());
                rcell.invalidate_set (col [j].invalid ());               
                rcell.invalidate_set (blk [i - i%3u + j/3u].invalid ());
            }
        }
    }
public:

    auto mutate (std::uint32_t id, std::uint8_t v) const {
        auto new_grid = *this;
        new_grid.cell (id).value (v);
        auto x = id%9u;
        auto y = id/9u;
        auto k = y - y%3u + x/3u;

        for (auto i = 0u; i < 9u; ++i) {
            auto _empty =
                new_grid.cell (y*9u + i).invalidate (v).empty () ||
                new_grid.cell (i*9u + x).invalidate (v).empty () ||
                new_grid.cell (k, i).invalidate (v).empty ();
            if (_empty) {
                new_grid.good (false);
                break;
            }
        }
        return new_grid;
    }

    sudoku_grid (const std::array<std::uint8_t, 81u>& in) {
        std::copy (std::begin (in), std::end (in), std::begin (grid));
        prune ();
    }

    sudoku_grid () {
        std::fill (std::begin (grid), std::end (grid), 0u);
    }

    sudoku_grid (const sudoku_grid&) = default;

    sudoku_grid& operator = (const sudoku_grid&) = default;

    sudoku_grid& operator = (const std::array<std::uint8_t, 81u>& in) {
        return *this = sudoku_grid (in);
    }

    auto&& operator [] (std::uint32_t id)         { return cell (id); }
    auto&& operator [] (std::uint32_t id) const   { return cell (id); }

    operator bool () const { return is_good (); }

private:
    std::array <cell_type, 81u> grid;
    bool _is_good = true;
};

bool solve (const sudoku_grid& grid, sudoku_grid& gout, std::uint32_t next = 0u) {
    auto i = next;
    while (i < 81u && grid.cell (i).is_set ()) ++i;
    if (i >= 81u) {
        gout = grid;
        return true;
    }
    auto& _cell = grid.cell (i);
    for (auto j = 1u; j <= 9; ++j) {
        if (!_cell.is_valid (j))
            continue;
        auto new_grid = grid.mutate (i, j);
        if (!new_grid.is_good ())
            continue;
        if (solve (new_grid, gout, i + 1u))
            return true;
    }
    return false;
}



int main () try {
    //#define TEST
    #ifndef TEST
    auto puzzles = pre_load (std::cin, 500);
    auto out = sudoku_grid ();
    auto t0 = std::chrono::high_resolution_clock::now ();
    for (const auto& in: puzzles) {
        if (!solve (in, out) || !out.is_valid ()) {
            throw std::runtime_error ("Solution not found");
        }        
    }
    auto t1 = std::chrono::high_resolution_clock::now ();
    auto t = std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count () / 1000.0;
    std::cout << t << "s\n";
    #else
    void run_tests ();
    run_tests ();
    #endif

    return 0;
}
catch (const std::exception& e) {
    std::cout << e.what () << "\n";
    return -1;    
}

void test_cell () {
    sudoku_grid::cell_type ct;
    assert (ct.value () == 0u);
    assert (ct.is_set () == false);
    assert (ct.empty () == false);
    assert (ct.is_valid (1u) == true);
    assert (ct.is_valid (2u) == true);
    assert (ct.is_valid (3u) == true);
    assert (ct.is_valid (4u) == true);
    assert (ct.is_valid (5u) == true);
    assert (ct.is_valid (6u) == true);
    assert (ct.is_valid (7u) == true);
    assert (ct.is_valid (8u) == true);
    assert (ct.is_valid (9u) == true);
    ct = 9;
    assert (ct.value () == 9u);
    assert (ct.is_set () == true);
    assert (ct.empty () == false);
    assert (ct.is_valid (1u) == false);
    assert (ct.is_valid (2u) == false);
    assert (ct.is_valid (3u) == false);
    assert (ct.is_valid (4u) == false);
    assert (ct.is_valid (5u) == false);
    assert (ct.is_valid (6u) == false);
    assert (ct.is_valid (7u) == false);
    assert (ct.is_valid (8u) == false);
    assert (ct.is_valid (9u) == true);
    ct.value (8);
    assert (ct.value () == 8u);
    assert (ct.is_set () == true);
    assert (ct.empty () == false);
    assert (ct.is_valid (1u) == false);
    assert (ct.is_valid (2u) == false);
    assert (ct.is_valid (3u) == false);
    assert (ct.is_valid (4u) == false);
    assert (ct.is_valid (5u) == false);
    assert (ct.is_valid (6u) == false);
    assert (ct.is_valid (7u) == false);
    assert (ct.is_valid (8u) == true);
    assert (ct.is_valid (9u) == false);
    ct.value (0);
    assert (ct.value () == 0u);
    assert (ct.is_set () == false);
    assert (ct.empty () == false);
    assert (ct.is_valid (1u) == true);
    assert (ct.is_valid (2u) == true);
    assert (ct.is_valid (3u) == true);
    assert (ct.is_valid (4u) == true);
    assert (ct.is_valid (5u) == true);
    assert (ct.is_valid (6u) == true);
    assert (ct.is_valid (7u) == true);
    assert (ct.is_valid (8u) == true);
    assert (ct.is_valid (9u) == true);
    ct.invalidate (5u);
    assert (ct.value () == 0u);
    assert (ct.is_set () == false);
    assert (ct.empty () == false);
    assert (ct.is_valid (1u) == true);
    assert (ct.is_valid (2u) == true);
    assert (ct.is_valid (3u) == true);
    assert (ct.is_valid (4u) == true);
    assert (ct.is_valid (5u) == false);
    assert (ct.is_valid (6u) == true);
    assert (ct.is_valid (7u) == true);
    assert (ct.is_valid (8u) == true);
    assert (ct.is_valid (9u) == true);
    ct.invalidate (6u);
    assert (ct.value () == 0u);
    assert (ct.is_set () == false);
    assert (ct.empty () == false);
    assert (ct.is_valid (1u) == true);
    assert (ct.is_valid (2u) == true);
    assert (ct.is_valid (3u) == true);
    assert (ct.is_valid (4u) == true);
    assert (ct.is_valid (5u) == false);
    assert (ct.is_valid (6u) == false);
    assert (ct.is_valid (7u) == true);
    assert (ct.is_valid (8u) == true);
    assert (ct.is_valid (9u) == true);
    ct.invalidate (7u);
    assert (ct.value () == 0u);
    assert (ct.is_set () == false);
    assert (ct.empty () == false);
    assert (ct.is_valid (1u) == true);
    assert (ct.is_valid (2u) == true);
    assert (ct.is_valid (3u) == true);
    assert (ct.is_valid (4u) == true);
    assert (ct.is_valid (5u) == false);
    assert (ct.is_valid (6u) == false);
    assert (ct.is_valid (7u) == false);
    assert (ct.is_valid (8u) == true);
    assert (ct.is_valid (9u) == true);
    ct.invalidate (8u);
    assert (ct.value () == 0u);
    assert (ct.is_set () == false);
    assert (ct.empty () == false);
    assert (ct.is_valid (1u) == true);
    assert (ct.is_valid (2u) == true);
    assert (ct.is_valid (3u) == true);
    assert (ct.is_valid (4u) == true);
    assert (ct.is_valid (5u) == false);
    assert (ct.is_valid (6u) == false);
    assert (ct.is_valid (7u) == false);
    assert (ct.is_valid (8u) == false);
    assert (ct.is_valid (9u) == true);
    assert (ct.invalid () == 0x0f0);
    ct.invalidate_set (ct.invalid () | 0xf);
    assert (ct.value () == 0u);
    assert (ct.is_set () == false);
    assert (ct.empty () == false);
    assert (ct.is_valid (1u) == false);
    assert (ct.is_valid (2u) == false);
    assert (ct.is_valid (3u) == false);
    assert (ct.is_valid (4u) == false);
    assert (ct.is_valid (5u) == false);
    assert (ct.is_valid (6u) == false);
    assert (ct.is_valid (7u) == false);
    assert (ct.is_valid (8u) == false);
    assert (ct.is_valid (9u) == true);
    assert (ct.invalid () == 0x0ff);
    ct.invalidate (9u);
    assert (ct.value () == 0u);
    assert (ct.is_set () == false);
    assert (ct.empty () == true);
    assert (ct.is_valid (1u) == false);
    assert (ct.is_valid (2u) == false);
    assert (ct.is_valid (3u) == false);
    assert (ct.is_valid (4u) == false);
    assert (ct.is_valid (5u) == false);
    assert (ct.is_valid (6u) == false);
    assert (ct.is_valid (7u) == false);
    assert (ct.is_valid (8u) == false);
    assert (ct.is_valid (9u) == false);
    assert (ct.invalid () == ct.INVALID_ALL);
}

void run_tests () {
    test_cell ();
}

