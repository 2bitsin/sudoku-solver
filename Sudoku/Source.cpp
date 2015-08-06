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
        cell_type               (): _value (0u) {}
        cell_type               (std::uint8_t v):_value (v), _mask (v == 0 ? 0 : INVALID_ALL) {}
        cell_type& operator =   (std::uint8_t v)        { return *this = cell_type (v); }
        auto invalid            () const                { return _mask; }
        auto value              () const                { return _value; }
        auto invalid            (std::uint16_t v)       { return _mask = v; }
        auto value              (std::uint8_t v)        { return _mask = v == 0 ? 0u : INVALID_ALL, _value = v; }
        auto is_set             () const                { return value () != 0u; }
        auto empty              () const                { return !is_set () && invalid () == INVALID_ALL; }
        auto& invalidate        (std::uint8_t v)        { if (v >= 1 && v <= 9) _mask |= (1 << (v - 1u)); return *this; }
        auto& invalidate_set    (std::uint16_t v)       { _mask |= (v & INVALID_ALL); return *this; }
        auto is_valid (std::uint8_t v) const            { return (v >= 1 && v <= 9) && ((is_set () && value () == v) || (_mask & (1 << (v - 1u)))); }

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
                blk [i].invalidate (cell (map (i, j)).value ());
            }
        }

        for (auto i = 0u; i < 9u; ++i) {
            for (auto j = 0u; j < 9u; ++j) {
                auto& rcell = cell (j, i);
                rcell.invalidate_set (row [j].invalid ());
                rcell.invalidate_set (col [i].invalid ());
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
    for (auto j = 0u; j < 9; ++j) {
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
    auto puzzles = pre_load (std::cin, 50);
    auto out = sudoku_grid ();
    for (const auto& in: puzzles) {
        print (in);
        std::cout << "\n";
        if (!solve (in, out)) {
            std::cout << "Solution not found!\n";
            continue;
        }
        print (out);
        std::cout << "\n";
    }

    return 0;
}
catch (const std::exception& e) {
    std::cout << e.what () << "\n";
    return -1;    
}




/*
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

struct grid {
    typedef std::uint8_t value_type;
    typedef std::uint16_t meta_type;
    typedef std::array<value_type, 81u> value_grid_type;
    typedef std::array<meta_type, 81u> meta_grid_type;   

    grid (): 
        value {
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        meta {
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        good {true}
    {}


    friend std::istream& operator >> (std::istream& in, grid& grid) {
        std::string line;
        std::getline (in, line);
        if (line.size () != 81u)
            throw std::runtime_error ("Invalid input");
        for (auto i = 0u; i < 81u; ++i) {
            grid.meta [i] = std::uint16_t (0);
            grid.value [i] = std::uint8_t (line [i] == '.' ? 0 : line [i] - '0');
        }
        grid.good = true;

        return in;
    }

    friend std::ostream& operator << (std::ostream& out, const grid& g) {
        for (auto y = 0; y < 3u; ++y) {
            if (y != 0) std::cout <<
                " ------+-------+------\n";
            for (auto i = 0; i < 3u; ++i) {
                for (auto x = 0; x < 3u; ++x) {
                    if (x != 0) std::cout << " |";
                    for (auto j = 0; j < 3u; ++j) {
                        auto& v = g.value [(y*3 + i)*9 + (x*3 + j)];
                        if (v != 0) std::cout << " " << int (v);
                        else std::cout << " .";
                    }
                }
                std::cout << "\n";
            }
        }
        return out;
    }
    grid (std::istream& in): grid () {
        in >> *this;
    }

    grid (const grid&) = default;
    grid& operator = (const grid&) = default;

    value_grid_type value;
    meta_grid_type meta;
    bool good;
};




template <typename _Ctype>
auto collect (const _Ctype& in) {
    std::uint16_t mask = 0u;
    for (auto i = 0u; i < 9u; ++i) 
        mask |= (1u << in (i));    
    return mask >> 1u;
}

auto collect_row (const grid& g, std::uint32_t row) {
    auto& value = g.value;
    return collect ([&value, row] (auto i) {
        return value [i + row*9u];
    });
}

auto collect_col (const grid& g, std::uint32_t col) {
    auto& value = g.value;
    return collect ([&value, col] (auto i) {
        return value [i*9u + col];
    });
}

auto collect_cel (const grid& g, std::uint32_t idx) {
    auto& value = g.value;
    return collect ([&value, idx] (auto i) {
        return value [mapping [idx][i]];
    });
}

auto prune_all (grid& g) {
    std::uint16_t rows [9u], cols [9u], cels [9u];
    for (auto i = 0u; i < 9u; ++i) {
        rows [i] = collect_row (g, i);
        cols [i] = collect_col (g, i);
        cels [i] = collect_cel (g, i);
    }

    for (auto j = 0u; j < 9u; ++j) {
        for (auto i = 0u; i < 9u; ++i) {
            auto k = (j/3u)*3u + (i/3u);
            g.meta [j*9u + i] = cels [k]|rows [j]|cols [i];            
        }
    }
}

auto mutate (const grid& g, std::uint32_t idx, std::uint16_t v) {
    auto new_g = g;    
    new_g.value [idx] = grid::value_type (v);
    auto x = idx % 9u;
    auto y = idx / 9u;
    auto k = (y/3u)*3u + (x/3u);
    v = (1u << (v - 1u));
    for (auto i = 0u; i < 9u; ++i) {
        auto mrow = i*9+x;
        auto mcol = y*9+i;
        auto mcel = mapping [k] [i];
        new_g.meta [mrow] |= v;
        new_g.meta [mcol] |= v;
        new_g.meta [mcel] |= v;
        if ((!new_g.value [mrow] && new_g.meta [mrow] == 0x1ff) ||
            (!new_g.value [mcol] && new_g.meta [mcol] == 0x1ff) ||
            (!new_g.value [mcel] && new_g.meta [mcel] == 0x1ff))
        {
            new_g.good = false;
            break;
        }        
    }
    return new_g;
}

bool solve (const grid& g, grid& out, unsigned next = 0u) {        
    auto i = next;
    while (i < 81u && g.value [i] > 0u)
        ++i;
    if (i >= 81u) {
        out = g;
        return true;
    }

    auto m = g.meta [i];        
    for (auto j = 0u; j < 9; ++j) {
        if (m & (1u << j))
            continue;
        auto ng = mutate (g, i, j + 1u);
        if (!ng.good) 
            continue;
        if (solve (ng, out, i + 1u))
            return true;
    }
    return false;
}

bool validate (const grid& g) {    
    for (auto i = 0u; i < 9u; ++i) {        
        unsigned brow [9u] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
        unsigned bcol [9u] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
        unsigned bcel [9u] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
        for (auto j = 0u; j < 9u; ++j) {
            if (
                ++bcol [g.value [j * 9 + i] - 1u] > 1u ||
                ++brow [g.value [i * 9 + j] - 1u] > 1u ||
                ++bcel [g.value [mapping [i] [j]] - 1u] > 1u)
            {
                return false;
            }
        }                    
    }
    return true;
}


int main (int, char**) try {
    int i = 50;
    auto t0 = std::chrono::high_resolution_clock::now ();
    while (std::cin && i > 0) {
        auto root = grid (std::cin);     
        auto solt = grid ();
        prune_all (root);
        std::cout << root;
        std::cout << "-----\n";
        if (solve (root, solt)) {
            std::cout << solt;
            if (!validate (solt))
                std::cout << "Invalid solution";
        }
        else {
            std::cout << "No solution\n";
        }
        std::cout << "==========\n";
        --i;
    }
    auto t1 = std::chrono::high_resolution_clock::now ();
    std::cout << "\nTime : " << std::chrono::duration_cast<std::chrono::milliseconds> (t1 - t0).count () / 1000.0 << "\n";
    return 0;
}
catch (const std::exception& e) {
    std::cout << e.what () << "\n";
    return -1;
}
*/